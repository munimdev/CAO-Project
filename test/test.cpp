#include <WiFi.h>
#include <Arduino.h>
#include <ESP_Mail_Client.h>
#include <Keypad.h>

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
WiFiServer server(80); // start wifi server on port 80

// Variable to store the HTTP request
String header;

boolean mailboxAlert = false; //global bool variable that tells whether an email should be sent or not
boolean doorStatus = false; //tells whether door is currently open or not

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

const long doorTimeoutTime = 15000;
unsigned long doorOpeningTime = 0;
unsigned long currentClockTime = millis();

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "caoprojectesp32@gmail.com"
#define AUTHOR_PASSWORD "iicjzexnzetvnket"

/* Recipient's email*/
#define RECIPIENT_EMAIL "munimzafar100@gmail.com"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

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
  {'1', '2', '3','='},
  {'4', '5', '6','>'},
  {'7', '8', '9','<'},
  {'*', '0', '#','+'},
};

byte pin_rows[ROW_NUM] = {pin5,pin6,pin7,pin8}; 
byte pin_column[COLUMN_NUM] = {pin1,pin2,pin3,pin4}; 

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password = "7890"; // password for mailbox
String input_password;

void sendEmail() {
  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Munim Zafar", RECIPIENT_EMAIL);

  /*Send HTML message*/
  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  Serial.println("Motion detected. Turning LED on");
  // digitalWrite(LED, HIGH);
  mailboxAlert = true; //momentarily set the alert variable to true, when motion sensor changes from LOW to HIGH
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

  server.begin();
}

void loop() {
  char key = keypad.getKey();
  WiFiClient client = server.available();   // Listen for incoming clients

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

  if(password != input_password) {
    if (key) {
      Serial.println(key);

      if (key == '*') {
        input_password = ""; // clear input password
      } else if (key == '#') {
        
      } else {
          Serial.println("The password is incorrect, ACCESS DENIED!");
      }

      input_password = ""; // clear input password
    } else {
        input_password += key; // append new character to input password string
      }
  }
  else if(password == input_password && key == '#') {
    Serial.println("The password is correct, ACCESS GRANTED!");
    doorOpeningTime = millis();
    
    if(mailboxAlert) {
      digitalWrite(LED, HIGH);
      doorOpeningTime = millis();
      doorStatus = true;
      sendEmail();
      mailboxAlert = false;
    }

    unsigned long doorOpeningTime = 0;
    currentClockTime = millis();
  }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}