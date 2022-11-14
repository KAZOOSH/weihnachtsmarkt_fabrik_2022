#include <ESP8266WiFi.h>
#include "ESPAsyncUDP.h"
const int localUdpPort = 2390;
const char * ssid = "kazoosh";
const char * password = "k42005h!!!";

AsyncUDP udp;

void setup() {


  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
    delay(5000);
    String color = String(random(255)) + ',' + String(random(255)) + ',' + String(random(255));
    Serial.println("broadcast" + color);
    udp.broadcastTo(color.c_str(), localUdpPort);
}
