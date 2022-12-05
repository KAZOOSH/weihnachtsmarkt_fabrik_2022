#include <FastLED.h>
#include <AS5045.h>
#include <Easing.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>


//*********************************
//   HARDWARE INTERFACE
//*********************************

// LED config
#define CSpin   14 // wmos pin1
#define CLKpin  12 // wmos pin2
#define DOpin   13 // wmos pin3

#define NUM_LEDS 60
#define DATA_PIN 2 // wmos pin4
CRGB leds[NUM_LEDS];
uint8_t hue = 0;
#define D_LEDS 1

// rotation sensor
AS5045 enc (CSpin, CLKpin, DOpin) ;

// wifi
const char *ssid = "kazoosh";
const char *pass = "k42005h!!!";
AsyncUDP udp;

//*********************************
//   STATE MANAGEMENT VARIABLES
//*********************************
enum state {IDLE, ROTATING, SENDING,END};

// time control
state currentState = IDLE;
unsigned long currentTime = 0;
unsigned long stateBegin = 0;

//wheel position
float angle = 0;

// velocity 
float velocity = 0;
const int vNVals = 10;
int vPointer = 0;
float vHistory[vNVals];
unsigned long dVMeasure = 30;
unsigned long lastVMeasure = 0;

// *******
// states
// *******

// idle
const unsigned long delayAniIdle = 45;
const unsigned long delayIdle = 1000;
unsigned long lastT;
const float vEndIdle = 10;

// rotating
EasingFunc<Ease::Linear> eRotation;
const float vMax = 25;

// sending
bool isSendColor = false;
CHSV colorSend;
float sendAngle = 0.0;


void setup() {
  // setup easing
  eRotation.duration(4); // nleds
  eRotation.scale(255);
  
  // start serial
  Serial.begin(115200);

  if (!enc.begin ())
    Serial.println ("Error setting up AS5045") ;

  // start leds
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
  LEDS.setBrightness(84);
    
  // Get WiFi going
  connectWifi();

  // init velocity array
  for (int i=0; i< vNVals; ++i){
    vHistory[i] = 0;
  }
}

void connectWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi Failed");
    while (1)
    {
      delay(1000);
    }
  }
}

void sendColorValue(int r,int g, int b){
  String builtString = String(r) + "," + String(g) + "," + String(b);
  Serial.println(builtString);
  // Send UDP Broadcast to 255.255.255.255 (default broadcast addr), Port 2255
  udp.broadcastTo(builtString.c_str(), 2390);
}

void updateVelocity(){
  // update in fixed timespan
  if( currentTime - lastVMeasure > dVMeasure){
    // add current angle to ringbuffer
    vHistory[vPointer] = angle;

    // set v to 0 and create median array
    velocity = 0;
    float vMedian[vNVals];

    // add angle diferences to median array
    for (int i=0; i< vNVals; ++i){
      int pos = (vPointer + i)%vNVals;

      int pos2 = pos-1;
      if (pos2 <0) pos2 = vNVals-1;
      
      float vCurrent = vHistory[pos]-vHistory[pos2];
      vMedian[i] = vCurrent;
      
      velocity += vCurrent;
    }

    // kick out highest and lowest value
    int iMax =0;
    int iMin=0;
    for (int i=0; i< vNVals; ++i){
      if(vMedian[i] < vMedian[iMin]) iMin = i;
      if(vMedian[i] > vMedian[iMax]) iMax = i;
    }
    // calculate mean as velocity
    for (int i=0; i< vNVals; ++i){
      if (i!= iMin && i!= iMax){
        velocity += vMedian[i];
      }
    }
    velocity/= (vNVals-2);
  
    vPointer=(vPointer+1)%vNVals;
    lastVMeasure = currentTime;
  
    //Serial.println(velocity);
  }
}

int getBrightnessForLed(int led,float percentage){
  // calculates brightness for leds on ring animations
  int n = NUM_LEDS/2;
  if (led>n){
    led = NUM_LEDS -led;
  }  
  float begin = n*percentage;
  if (led>begin){
    return 0;
  }
  int ret = max(0.0,min(eRotation.get(begin-led),255.0));
  return ret;
}


void changeState(state newState){
  currentState = newState;
  stateBegin = currentTime;
}

// proceed idle state
void idle(){
  float stepAngle = 255.0/float(NUM_LEDS);
  float startAngle = map(angle,0,360,0,255);

  // if rotation fast enough switch state
  if(velocity > vEndIdle){
    changeState(ROTATING);
  }else{
    // proceed with colors moving around wheel
    unsigned long tAll = delayIdle + NUM_LEDS*delayAniIdle;
    unsigned long t = currentTime%tAll;
    if (t< delayIdle){
      CRGB color = CRGB(0,0,0);
      for (int i=0; i<NUM_LEDS;i+=1){
        leds[i] = color;
        
      }
    }else{
      t -= delayIdle;
      float nLed =  NUM_LEDS - float(t)/float(delayAniIdle);
      CRGB black = CRGB(0,0,0);
      
      for (int i=0; i<NUM_LEDS;i+=1){
        int currentAngle = int(startAngle +i*stepAngle)%255;

        double d = double(i - nLed);
        if (d < 0) {
          d = 10;
        }
        d = min(10.0,d);
        d = 10 - d;
        d/=10;
        float percentage =d;
        float brightness = getBrightnessForLed(i,percentage);

        // fade out upper leds
        if (i < 20){
          brightness *= i*0.05;
        }
        
        CRGB color = CHSV( currentAngle, 255, brightness);
        leds[i] = color;
      }
    }
  }
}

// proceed rotating state
void rotating(){
  if (velocity != 0){
    // color wheel animation
    float startAngle = map(angle,0,360,0,255);
    float stepAngle = 255.0/float(NUM_LEDS);
  
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
      int currentAngle = int(startAngle +i*stepAngle)%255;
      CRGB color = CHSV( currentAngle, 255, 255);
      leds[i] = color;
    }
  }else{
    // end state and set send color
    float startAngle = map(angle,0,360,0,255);
    colorSend = CHSV( startAngle, 255, 255);
    sendAngle = startAngle;
    
    isSendColor = false;
    changeState(SENDING);
  }
}
// proceed sending state
void sending(){
  // current time
  unsigned int t = currentTime - stateBegin;
  // times for animation steps in ms
  unsigned int t0 = 3000; // fade out colors
  unsigned int t1 = 2000; // fade in selected color
  unsigned int t2 = 3000; // pulsing
  unsigned int t3 = 2000; // fade out selected color
  unsigned int t4 = 5000; // inactive time

  if(!isSendColor){
    CRGB col =colorSend;
    sendColorValue(col[0],col[1],col[2]);
    isSendColor = true;
  }
  if (t< t0){
    float percentage = 1.0 - float(t)/float(t0);

  float stepAngle = 255.0/float(NUM_LEDS);
  
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
      int currentAngle = int(sendAngle +i*stepAngle)%255;

      int v = getBrightnessForLed(i,percentage);
      CHSV col = CHSV( currentAngle, 255, v);
      leds[i] = col;
    }
  }

  else if (t < t1+t0){
    t-=t0;
    float percentage = float(t)/float(t1);
    //Serial.println(percentage);
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
      CHSV col = colorSend;
      int v = getBrightnessForLed(i,percentage);
      col.value = v;
      leds[i] = col;
    }
  }else if (t<t1+t2+t0){
    float p = float(map(t-t1,0,t2,0,M_PI*2*10000))/10000.0;
    float v = (cos(p)+1)*128;
    CHSV col = colorSend;
    col.value = v;
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
     leds[i] = col;
    }
  }else if (t<t1+t2+t3+t0){
    t= t -t0 -t1 -t2;
    float percentage = float(t)/float(t3);
    CHSV col = colorSend;
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
      col.value = getBrightnessForLed(i,1.0-percentage);
      leds[i] = col;
    }
  }else if (t<t1+t2+t3+t4+t0){
    for (int i=0; i<NUM_LEDS;i+=D_LEDS){
      leds[i] = CRGB::Black;
    }
  }else{
    changeState(END);
  }
}

void ending(){
  if(currentTime - stateBegin > 2000){
    changeState(IDLE);
  }
}


void loop() {
  lastT = currentTime;
  currentTime = millis();
  angle = enc.read () * 0.08789;

  updateVelocity();

  switch (currentState) {
    case IDLE: 
      idle();
      break;
    case ROTATING:  
      rotating();
      break;
    case SENDING:  
      sending();
      break;
     case END:  
      ending();
      break;
  }
  FastLED.show();
  delay(1);
}
