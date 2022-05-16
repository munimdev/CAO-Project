#include <WiFi.h>

const char* ssid = "MY_SSID";
const char* password = "MY_PASSWORD";

// Set web server port number to 80
WiFiServer server(80);

void initWiFi() {
  WiFi.mode(WIFI_STA); //setting wifi mode to station so esp can connect to router
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); //.begin uses credentials to connect to the router
  while (WiFi.status() != WL_CONNECTED) { //waiting until esp32 is connected to router
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //printing the local ip address
  server.begin(); //starting a local server on the ip assigned by router
}

void setup() {
  Serial.begin(115200);
  initWiFi(); //calls init wifi function to connect to wifi
  Serial.print("RRSI: "); 
  Serial.println(WiFi.RSSI()); //prints rssi, which is basically the network strength
}

void loop() {
  
}