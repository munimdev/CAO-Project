#include <Arduino.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Keypad.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

//define esp32 pins
#define MOTION_SENSOR 27
#define DOOR 26

//wifi setup
#define WIFI_SSID "C-133"
#define WIFI_PASSWORD "changeme2018"

//Adafruit.io Setup
#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883              
#define AIO_USERNAME   "munimzafar"
#define AIO_KEY        "aio_SLIJ73QBlkU1P74I7GZ1h9vVNLgm"

//keypad setup
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns
#define PIN1 17
#define PIN2 16
#define PIN3 4
#define PIN4 2
#define PIN5 5
#define PIN6 18
#define PIN7 19
#define PIN8 21

char keys[ROW_NUM][COLUMN_NUM] = { //define keypad layout
  {'1', '2', '3','A'},
  {'4', '5', '6','B'},
  {'7', '8', '9','C'},
  {'*', '0', '#','D'},
};
byte pin_rows[ROW_NUM] = {PIN5,PIN6,PIN7,PIN8}; 
byte pin_column[COLUMN_NUM] = {PIN1,PIN2,PIN3,PIN4}; 
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
char key; //stores the inputted key value
String password = String(random(1000, 9999)); // password for mailbox
String input_password;

const char* webServerUsername = "munimzafar";
const char* webServerPassword = "caoproject";
const char* inputParameter = "doorState";
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Variable to store the HTTP request
String header;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup Adafruit feeds called to subscribe to door feed and email feed
Adafruit_MQTT_Subscribe Door = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/led"); 
Adafruit_MQTT_Publish DoorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/led");
Adafruit_MQTT_Publish MailBoxFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mailbox");

boolean mailboxAlert = false; //global bool variable that tells whether an email should be sent or not
boolean doorStatus = false; //tells whether door is currently open or not
boolean emailSent = false;

const long doorTimeoutTime = 15000; //15 seconds
unsigned long doorOpeningTime = 0;
unsigned long currentClockTime = millis();

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>CAO Project - Munim Zafar</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-0evHe/X+R7YkIZDRvuzKMRqM+OrBnVFBL6DOitfPri4tjfHxaWutUpFmBp4vmVor" crossorigin="anonymous">
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap.bundle.min.js" integrity="sha384-pprn3073KE6tl6bjs2QrFaJGz5/SUsLqktiwsUTF55Jfv3qYSDhgCecCxMW52nD2" crossorigin="anonymous"></script>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    body {padding-bottom: 10px; box-sizing: border-box;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .logout-button {position: absolute; top: 10px; right: 10px;}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
    <div><h2>Package Delivery Alert System</h2></div>
    <div class="logout-button"><button type="button" class="btn btn-outline-dark" onclick="logoutButton()">Logout</button></div>
  <div class="d-flex align-items-center justify-content-center">
    <div class="col-1">
      <div class="row d-flex align-items-center justify-content-center">Trunk Status</div>
      <div class="row d-flex align-items-center justify-content-center"><span id="doorState">%STATE%</span></div>
    </div>
    <div class="col-1">
      %BUTTONPLACEHOLDER%
    </div>
  </div>
  <div class="d-flex align-items-center justify-content-center">
    <div class="row d-flex align-items-center justify-content-center"><span id="keypad-password">Current keypad password: %PASSWORD%</span></div>
  </div>
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?doorState=1", true); 
    document.getElementById("doorState").innerHTML = "<span style='color: green;'>Open</span>";  
  }
  else { 
    xhr.open("GET", "/update?doorState=0", true); 
    document.getElementById("doorState").innerHTML = "<span style='color: red;'>Closed</span>";      
  }
  xhr.send();
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
</script>
</body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
</body>
</html>
)rawliteral";

String outputState(){
  if(doorStatus){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  if (var == "STATE"){
    if(doorStatus){
      return "<span style='color: green;'>Open</span>";
    }
    else {
      return "<span style='color: red;'>Closed</span>";
    }
  }
  if(var == "PASSWORD") {
    return password.c_str();    
  }
  return String();
}

// Checks if motion was detected inside the package trunk
void IRAM_ATTR detectPackageDelivery() {
  if(doorStatus == true) {
    mailboxAlert = true;
  }
}

void openPackageTrunk(bool openedByOwner) {
  doorStatus = true;
  doorOpeningTime = millis();
  digitalWrite(DOOR, HIGH);
  emailSent = openedByOwner; //set emailSent to true so that the owner does not trigger the email when taking out the package
  if(openedByOwner) {
    input_password = "";
    Serial.println("Trunk opened by owner");
  }
}

void closePackageTrunk() {
  doorStatus = false;
  emailSent = false;
  mailboxAlert = false;
  input_password = "";
  password = String(random(1000, 9999)); // regenerate the password
  digitalWrite(DOOR, LOW);
  Serial.println("Trunk is now closed");
}



void MQTT_connect(); //function declaration. This function connects to the MQTT service to communicate with the Adafruit API

void setup() {
  Serial.begin(115200);
  input_password.reserve(32); // max 32 characters for input

  Serial.print(String("Connecting to "+String(WIFI_SSID)));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println(String("IP address: "+WiFi.localIP()));
  Serial.println();

  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(MOTION_SENSOR, INPUT_PULLUP);
  
  pinMode(DOOR, OUTPUT);
  // Set DOOR to LOW
  digitalWrite(DOOR, LOW);
  
  //Set the motion sensor output pin as an interrupt and assign the interrupt function
  //the interrupt function is called on RISING mode, meaning when output goes from LOW to HIGH
  attachInterrupt(digitalPinToInterrupt(MOTION_SENSOR), detectPackageDelivery, RISING);
  
  // Setup MQTT subscription for Door feed to monitor ON or OFF status
  mqtt.subscribe(&Door);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(webServerUsername, webServerPassword))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
    Serial.println("Just received request.");
  });
    
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?doorState=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(webServerUsername, webServerPassword))
      return request->requestAuthentication();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?doorState=<inputMessage>
    if (request->hasParam(inputParameter)) {
      inputMessage = request->getParam(inputParameter)->value();
      inputParam = inputParameter;
      if(inputMessage.toInt()) {
        openPackageTrunk(true);
      }
      else {
        closePackageTrunk();
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}

void loop() {
  if(doorStatus == false) { //only get keypad input if the door is closed
    key = keypad.getKey();
  }

  //get current time
  currentClockTime = millis();
  if(doorStatus && (currentClockTime - doorOpeningTime > doorTimeoutTime)) {
    closePackageTrunk(); //close package time if package trunk has remained open for more than its timeout time
  }
  else if(doorStatus && mailboxAlert && !emailSent) { //if door is open and motion is detected, send email
    digitalWrite(DOOR, HIGH);
    DoorFeed.publish(1);
    if(MailBoxFeed.publish(1)) {
      Serial.println("Sending email");
      emailSent = true;
      mailboxAlert = false;
    }
    MailBoxFeed.publish(0);
  }

  if (key) {
    if (key == '*') {
      input_password = ""; // clear input password
    } else if (key == '#') { //check input password
      Serial.print("Keypad password inputted: ");
      Serial.println(input_password.toInt());
      if (password == input_password) {
        Serial.println("Trunk opened through keypad.");
        openPackageTrunk(false);
      } else {
        Serial.println("Invalid password attempt.");
      }
      input_password = ""; // clear input password
    } else {
      input_password += key; // append new character to input password string
    }
  }

  MQTT_connect(); //connect to the API service if not connected
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if(! mqtt.ping()) { // ping the API server to keep the mqtt connection alive
      mqtt.disconnect();
    }
    if (subscription == &Door ) { //monitor door status through Adafruit subscription
      int voiceCommand = atoi((char *)Door.lastread);

      if(voiceCommand == true && doorStatus == false) { //if door is opened
        openPackageTrunk(true);
      }
      else if(voiceCommand == false && doorStatus == true) { //if door is closed
        closePackageTrunk();
      }
    }
  }
}

void MQTT_connect() { //connects to the MQTT server
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT...");

  uint8_t retries = 5;
  
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}