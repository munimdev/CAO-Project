#include <WiFi.h>

const char* ssid = "C-133";
const char* password = "changeme2018";

void initWiFi() {
  WiFi.mode(WIFI_STA); //setting wifi mode to station so esp can connect to router
  WiFi.begin(ssid, password); //.begin uses credentials to connect to the router
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) { //checking if esp32 is connected and waiting
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP()); //printing the local ip address
}

void setup() {
  Serial.begin(115200);
  initWiFi(); //calls init wifi function to connect to wifi
  Serial.print("RRSI: "); 
  Serial.println(WiFi.RSSI()); //prints rssi, which is basically the network strength
}

void loop() {
  
}