#include <Arduino.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Keypad.h>
#include <WiFi.h>

#define MOTION_SENSOR 27
#define LED 26

//wifi setup
#define WIFI_SSID "C-133"
#define WIFI_PASSWORD "changeme2018"

//Adafruit.io Setup
#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883              
#define AIO_USERNAME   "munimzafar"
#define AIO_KEY        "aio_gKoC768Lzp2k8jChtPa5TkwakHxs"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe Door = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/led"); 
Adafruit_MQTT_Publish DoorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/led");
Adafruit_MQTT_Publish MailBoxFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mailbox");

WiFiServer server(80); // start wifi server on port 80

// Variable to store the HTTP request
String header;

boolean mailboxAlert = false; //global bool variable that tells whether an email should be sent or not
boolean doorStatus = false; //tells whether door is currently open or not
boolean mailBoxSent = false;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

const long doorTimeoutTime = 15000; //15 seconds
unsigned long doorOpeningTime = 0;
unsigned long currentClockTime = millis();

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // three columns

#define pin1 17
#define pin2 16
#define pin3 4
#define pin4 2
#define pin5 5
#define pin6 18
#define pin7 19
#define pin8 21

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3','A'},
  {'4', '5', '6','B'},
  {'7', '8', '9','C'},
  {'*', '0', '#','D'},
};

// byte pin_rows[ROW_NUM] = {pin5,pin6,pin7,pin8}; 
// byte pin_column[COLUMN_NUM] = {pin1,pin2,pin3,pin4}; 
byte pin_rows[ROW_NUM] = {pin5,pin6,pin7,pin8}; 
byte pin_column[COLUMN_NUM] = {pin1,pin2,pin3,pin4}; 

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password = "7890"; // password for mailbox
String input_password;

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  if(doorStatus == true) {
    mailboxAlert = true;
  }
}

void MQTT_connect();

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
  
  pinMode(LED, OUTPUT);
  // Set LED to LOW
  digitalWrite(LED, LOW);
  
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(MOTION_SENSOR), detectsMovement, RISING);
  
  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&Door);

  server.begin();
}

void loop() {
  char key = keypad.getKey();
  WiFiClient client = server.available();   // Listen for incoming clients
  MQTT_connect();
  currentClockTime = millis();
  if(doorStatus && (currentClockTime - doorOpeningTime > doorTimeoutTime)) {
    doorStatus = false;
    mailBoxSent = false;
    digitalWrite(LED, LOW);
    Serial.println("Door is now closed");
    input_password = ""; //reset input password
  }
  else if(doorStatus && mailboxAlert && !mailBoxSent) {
    digitalWrite(LED, HIGH);
    DoorFeed.publish(1);
    if(MailBoxFeed.publish(1)) {
      Serial.println("Sending email");
      mailBoxSent = true;
      mailboxAlert = false;
    }
    MailBoxFeed.publish(0);
    Serial.println("mailbox alert is "+bool(mailboxAlert));
  }

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Mailbox Alert Ssystem</h1>");    
            client.println("<p><a href=\"/email\"><button class=\"button\">Send Email</button></a></p>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  if (key) {
    Serial.println(key);

    if (key == '*') {
      input_password = ""; // clear input password
    } else if (key == '#') {
      if (password == input_password) {
        Serial.println("The password is correct, ACCESS GRANTED!");
        doorStatus = true;
        digitalWrite(LED, HIGH);
        doorOpeningTime = millis();
      } else {
        Serial.println("The password is incorrect, ACCESS DENIED!");
      }

      input_password = ""; // clear input password
    } else {
      input_password += key; // append new character to input password string
    }
  }
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    // ping the server to keep the mqtt connection alive
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    if (subscription == &Door ) {
      Serial.print(F("Got: "));
      Serial.println((char *)Door.lastread);
      int Light1_State = atoi((char *)Door.lastread);
      if(doorStatus == false) {
        doorStatus = true;
        input_password="";
        doorOpeningTime = millis();
      }
      digitalWrite(LED, Light1_State);
    }
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

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