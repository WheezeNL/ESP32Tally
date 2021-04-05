// This is a M5 Stack Atom based Tally Light made by Wheeze_NL.
// Based on the work of SKAARHOJ, Juerd, F0x, and many others.

// !!! Change the SkaarHoj libraries, Changes are in the Readme.
// https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/tree/master/ArduinoLibs
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

// https://github.com/Juerd/ESP-WiFiSettings/
#include <WiFiSettings.h>

// Onderstaande allemaal te vinden via arduino repo.
#include <FastLED.h>
#include <SPIFFS.h>
#include <WiFi.h>

const int buttonpin = 39;   // Pin where the button is connected
const int ledpin = 27;      // Pin where the Adressable LED's are connected
const int numleds = 25;     // How many LED's are connected to ledpin?

CRGB leds[numleds];
uint8_t hue = 0;

// Define the IP address of your ATEM switcher
IPAddress switcherIp(172, 16, 1, 100);    // Init of IPAddress variable for your ATEM Switcher, actual value is setup in WebGUI
ATEMstd AtemSwitcher; 

// Settings which will be changed through WiFiSettings
int cameraNumber = 0;
bool blShowPreview = false;
bool blAtemControl = false;
bool blAtemSetPreview = false;

unsigned long lastAtemUpdate = millis();
unsigned long lastAtemUpdatePush = millis();

int currentProgram = 0;
int currentPreview = 0;
bool commandSend = false; // Used as trigger to update the display after controlling the Switcher from the ESP.

#define IDLE 0                // Camera is not selected on switcher
#define PROGRAM 1                // Camera is program on switcher (live)
#define PREVIEW 2                // Camera is preview on switcher
#define ERROR 9                  // For use when we should reset?

int state = IDLE;
//int newState = IDLE; set in loop()

const bool _ = false;
const bool X = true;
const bool sprites[] = {
  _, X, X, _,
  X, _, _, X,
  X, _, _, X,
  X, _, _, X,
  _, X, X, _,

  _, _, X, _,
  _, X, X, _,
  _, _, X, _,
  _, _, X, _,
  _, X, X, X,

  _, X, X, _,
  X, _, _, X,
  _, _, X, _,
  _, X, _, _,
  X, X, X, X,

  X, X, X, _,
  _, _, _, X,
  _, X, X, _,
  _, _, _, X,
  X, X, X, _,
  
  _, _, X, _,
  _, X, X, _,
  X, _, X, _,
  X, X, X, X,
  _, _, X, _,

  X, X, X, X,
  X, _, _, _,
  _, X, X, _,
  _, _, _, X,
  X, X, X, _,

  _, X, X, _,
  X, _, _, _,
  X, X, X, _,
  X, _, _, X,
  _, X, X, _,

  X, X, X, X,
  _, _, _, X,
  _, _, X, _,
  _, X, _, _,
  _, X, _, _,

  _, X, X, _,
  X, _, _, X,
  _, X, X, _,
  X, _, _, X,
  _, X, X, _,

  _, X, X, _,
  X, _, _, X,
  _, X, X, X,
  _, _, _, X,
  _, X, X, _,

  _, X, X, _,
  X, _, _, X,
  X, X, X, X,
  X, _, _, X,
  X, _, _, X,

  X, X, X, _,
  X, _, _, X,
  X, X, X, _,
  X, _, _, X,
  X, X, X, _,

  _, X, X, _,
  X, _, _, X,
  X, _, _, _,
  X, _, _, X,
  _, X, X, _,

  X, X, X, _,
  X, _, _, X,
  X, _, _, X,
  X, _, _, X,
  X, X, X, _,

  X, X, X, X,
  X, _, _, _,
  X, X, X, _,
  X, _, _, _,
  X, X, X, X,

  X, X, X, X,
  X, _, _, _,
  X, X, X, _,
  X, _, _, _,
  X, _, _, _
};

void showNumber(int num) {
  int overflow = 0;
  while (num > 15) {
    num = num-16;
    overflow += 1;
  }

  if (overflow > 5) {overflow = 5;}

  for (int y=0; y<5; y++) {
    leds[20-y*5] = CRGB::Black;
    if (overflow > 0) {
      leds[20-y*5] = CHSV((hue+128)%255, 255, 255); // inverse of number hue
      overflow -= 1;
    }
    for (int x=0; x<4; x++) {
      leds[y*5+1+x] = sprites[num*5*4 + y*4 + x] ? CHSV((hue+y*5+x)%255, 255, 255) : CHSV(0, 0, 0);
    }
  }
  FastLED.show();
}

void changeState(int newstate) {
  switch (newstate)
  {
    case IDLE:
      showNumber(cameraNumber);
      break;
    case PROGRAM:
      fill_solid(leds, numleds, CRGB(255,0,0));  // fill red
      FastLED.show();
      break;
    case PREVIEW:
      if (blShowPreview){
        fill_solid(leds, numleds, CRGB(0,255,0));  // fill green, #ToDo make optional with the blShowPreview bool.
        FastLED.show();
      } else {
        showNumber(cameraNumber);
      }
      break;
    case ERROR:
      leds[0] = CRGB::Blue;
      FastLED.show();
      Serial.println("newState == ERROR, what's up?");
      break;
  }
  state = newstate;
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);
  pinMode(buttonpin, INPUT);

  FastLED.addLeds < WS2812B, ledpin, GRB > (leds, numleds);
  FastLED.setBrightness(20);

  WiFiSettings.heading("ATEM Switcher Settings", true);
  String switchIpstring = WiFiSettings.string("switcher_Ip", 7, 15, "192.168.1.121", "Atem Switcher IP"); // Variable only used during setup, so declared here.
  cameraNumber = WiFiSettings.integer("cameraID", 1, 9, 1, "Which input to follow");  // Select which input on the switched to follow.
  WiFiSettings.heading("Other Settings", true);
  blShowPreview = WiFiSettings.checkbox("show_Preview", false, "Show Preview Status?");  // Choose if you want to show the preview-status, or just the status when camera is program (live).
  WiFiSettings.warning("Be carefull when giving this unit control-rights, as it may cause unintended switching to live in uncontrolled environments!", true);
  blAtemControl = WiFiSettings.checkbox("allow_Atem_Control", false, "Allow Remote Control?");  // Do you want to control the switcher from the ESP?
  blAtemSetPreview = WiFiSettings.checkbox("allow_Atem_Set_Preview", false, "Set previous Program as Preview?");  // Do you want to set the preview-input of the switcher, when you set this camera-input to program?

  switcherIp.fromString(switchIpstring);  // Set the saved IP as the object needed by the library to connect.

  //WiFiSettings.onSuccess  = []() { green(); };  // What to do on succesfull connection to wifi? make screen a weird color perhaps?
  //WiFiSettings.onFailure  = []() { red(); };    // What to do on unsuccessfull connection to wifi?
  WiFiSettings.onWaitLoop = []() {
    static CHSV color(0, 255, 255);
    color.hue++;
    FastLED.showColor(color);
    if (! digitalRead(buttonpin)) WiFiSettings.portal();
    return 50;
  };
  WiFiSettings.onPortalWaitLoop = []() {
    static CHSV color(0, 255, 255);
    color.saturation--;
    FastLED.showColor(color);
  };
  WiFiSettings.connect();

  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();
  
}

void loop() {

  if ( (millis() - lastAtemUpdate) > 50 || commandSend ) {  // If lastAtemUpdate happened more then x miliseconds ago, AtemSwitcher.runLoop() will update the info in the AtemSwitcher. Might fail after 49.7 days.
    
    // Check for packets, respond to them etc. Keeping the connection alive!
    AtemSwitcher.runLoop();    
    lastAtemUpdate = millis();
    currentProgram = AtemSwitcher.getProgramInput();
    currentPreview = AtemSwitcher.getPreviewInput();
    commandSend = false;
    int newState = 0;
    if (!AtemSwitcher.isConnected()) {
      newState = ERROR; // (not connected?)
    } else if ( currentProgram == cameraNumber ) {
      newState = PROGRAM;
    } else if ( currentPreview == cameraNumber ) {
      newState = PREVIEW;
    } else {
      newState = IDLE;
    }

    if ( newState != state ) {
      Serial.println("state changed"); // Testing
      changeState(newState);
      }
  }

  if ( !digitalRead(buttonpin)) {
    if ((millis() - lastAtemUpdatePush) > 500 && AtemSwitcher.isConnected() && blAtemControl && AtemSwitcher.getProgramInput() != cameraNumber) {
      fill_solid(leds, numleds, CRGB(0,0,255));  // fill Blue
      FastLED.show();
      //void changeProgramInput(uint16_t inputNumber);
      if (blAtemSetPreview) {
        AtemSwitcher.changePreviewInput(AtemSwitcher.getProgramInput());
      }
      AtemSwitcher.changeProgramInput(cameraNumber);
      // Check for packets, respond to them etc. Keeping the connection alive!
      AtemSwitcher.runLoop();
      Serial.println("Button pushed, changed input and runLoop?"); // Testing
      lastAtemUpdatePush = millis();
    } else if ((millis() - lastAtemUpdatePush) > 500 && blAtemControl && AtemSwitcher.isConnected() == false) {
      Serial.println("Button Pushed, but not connected to ATEM");
      fill_solid(leds, numleds, CRGB(0,0,255));  // fill Blue
      FastLED.show();
      lastAtemUpdatePush = millis();
    } else if ((millis() - lastAtemUpdatePush) > 500 && blAtemControl == false) {
      Serial.println("Button Pushed, but not allowed in GUI");
      lastAtemUpdatePush = millis();
    }
  }
  
  hue += 1;
}
