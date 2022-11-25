/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>
#include <Ethernet.h>
#include "ESPAsyncUDP.h"

const char* ssid = "kazoosh";
const char* password = "k42005h!!!";
const int localUdpPort = 2390;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

AsyncUDP udp;

// Fastled WS2811
// #define NUM_LEDS 52
#define NUM_LEDS 15
#define DATA_PIN 2

CRGB leds[NUM_LEDS];
CRGB diffs[NUM_LEDS]; // diffs from current collor (for wobble effect)
int randomPerLed[NUM_LEDS];
// generate an moderatly unique id ;) for this light
int uniqueId = random(1, 30000);

int maxDelayNextColorUpdate = 10000; // default value for maximum delay to set next colot (10s) .. dely will be set randomly 

int blendSpeed = 10; // speed for color transition 
int activateColorFade = 1; // 0 to disable color fade
int sparklePropability = 1; // between 0 and 9
int sparkleMaxValue = 200;
int colorFadePropability = 1;
int colorFadeMaxValue = 75;

CRGB currentColor( CRGB::Black);
CRGB nextColor( CRGB::Black);
CRGB nextColorAfterDelay (CRGB::Black);
unsigned long nextColorUpdate = 0;

void loop() {
  EVERY_N_MILLISECONDS(50) { 

      blendToNextColor(blendSpeed);
      updateLEDS();

      // random color fade
      if(random(1000) < colorFadePropability && currentColor.r == 0 && currentColor.g == 0 && currentColor.b == 0){
        int randombrightness = random(colorFadeMaxValue);
        nextColor = CRGB(randombrightness, randombrightness, randombrightness);
     }
  }
  if(nextColorUpdate && (millis() > nextColorUpdate) ) {
    nextColor = nextColorAfterDelay;
    nextColorUpdate = 0;
  }
}

// blends current color to target color
void blendToNextColor(int speed) {
   currentColor.r = blendToNextColorValue(currentColor.r, nextColor.r, speed);
   currentColor.g = blendToNextColorValue(currentColor.g, nextColor.g, speed);
   currentColor.b = blendToNextColorValue(currentColor.b, nextColor.b, speed);

   // go back to black after color was shown
   if(currentColor.r == nextColor.r && currentColor.g == nextColor.g && currentColor.b == nextColor.b){
    nextColor = CRGB::Black;
   }
}

// brings from value closer to to value on every call (blend from current to target)
int blendToNextColorValue(int current, int target, int speed) {
  if(current != target){
    current = (10 * current + ((target - current) / abs(target - current))) * speed) /10;
  } 
  return current;
}


void updateLEDS()
{  

    for( int i = 0; i < NUM_LEDS; i++) {

       //sparkle 
       if( random16() < 50 * sparklePropability) {
          leds[ random16(NUM_LEDS) ] += CRGB(sparkleMaxValue,sparkleMaxValue,sparkleMaxValue);
        }

        int amplitude = min(15, max(currentColor.r + currentColor.g + currentColor.b ,0));
        
        leds[i].r = beatsin8(randomPerLed[i], _max(0,currentColor.r -amplitude), _min(255,currentColor.r +amplitude));
        leds[i].g = beatsin8(randomPerLed[i], _max(0,currentColor.g -amplitude), _min(255,currentColor.g +amplitude));
        leds[i].b = beatsin8(randomPerLed[i], _max(0,currentColor.b -amplitude), _min(255,currentColor.b +amplitude));
    }
    
    FastLED.show(); 
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/* 
 *  possible messages: 
 *  
 *    color_255,0,0 // rgb values r,g,b ([0-255],[0-255],[0-255])
 *    
 *    randomDelay_10000 // maximum delay to set color in ms
 *   
 *    sparkle_2,100   // sparkle sparklePropability, sparkleMaxValue ([0-9],[0-255])
 *  
 *    blendSpeed_3 // speed of color blending ([0-9], 0 means no blending)
 *    
 *    colorFade_1,75 //  colorFadePropability, colorFadeMaxValue ([0-9],[0-255])
 *    
*/

// process incomming udp message
void processMessage(String udpMessage) {

    String messageType = getValue(udpMessage,'_',0)
    String message = getValue(udpMessage,'_',1)
    
    if (messageType === 'color') {
      int r = getValue(message,',',0).toInt();
      int g = getValue(message,',',1).toInt();
      int b = getValue(message,',',2).toInt();
      nextColorAfterDelay = CRGB(r,g,b);
      nextColorUpdate = millis() + random(maxDelayNextColorUpdate); // random delayto turn to next color
    }

    if (messageType === 'sparkle') {
      sparklePropability = getValue(message,',',0).toInt();
      sparkleMaxValue = getValue(message,',',1).toInt();
    }

    if (messageType === 'blendSpeed') {
      blendSpeed = message.toInt();
    }

    if (messageType === 'randomDelay') {
      maxDelayNextColorUpdate = message.toInt();
    }
    if (messageType === 'colorFade') {
      colorFadePropability = getValue(message,',',0).toInt();
      colorFadeMaxValue = getValue(message,',',1).toInt();
    }

    
}


void setupUDP() {
  if(udp.listen(localUdpPort)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            String message = String( (char*) packet.data());
            processMessage(message)
        });
    }
}


void setupWifi() {
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

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS);

  // array defining random sin amplitudes for each led
  for(int i=0; i<NUM_LEDS; i++) {
    randomPerLed[i] = 30 + random(0,10);
  }

  Serial.begin(115200);
  delay(10);

  setupWifi();
  setupUDP();
}
