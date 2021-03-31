// This is a M5 Stack Atom based Tally Light.
// Based on the work of SKAARHOJ, Juerd, F0x, and many others.

// Download these from here
// https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/tree/master/ArduinoLibs
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

/*
!!! Change the SkaarHoj libraries !!!
for now look at the changes under "Modify the Skaarhoj library files for use with ESP32" on https://oneguyoneblog.com/2020/06/13/tally-light-esp32-for-blackmagic-atem-switcher/
*/

// Download this one from
// https://github.com/Juerd/ESP-WiFiSettings/
#include <WiFiSettings.h>

// Onderstaande allemaal te vinden via arduino repo.
#include <FastLED.h>
#include <SPIFFS.h>
#include <WiFi.h>

const int buttonpin = 39;
const int ledpin = 27;
const int numleds = 25;
CRGB leds[numleds];
uint8_t hue = 0;

// Define the IP address of your ATEM switcher
// In de setup vervangen we deze door de via portal ingestelde waarde!
IPAddress switcherIp(10, 4, 2, 121);
ATEMstd AtemSwitcher;

int cameraNumber = 0;
bool allowAtemControl = false;
bool allowAtemSetPreview = false;

unsigned long lastAtemUpdate = millis();
unsigned long lastAtemUpdatePush = millis();
int ProgramTally = 0;
int PreviewTally = 0;
int PreviewTallyPrevious = 0;
int ProgramTallyPrevious = 0;

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
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);
  pinMode(buttonpin, INPUT);

  FastLED.addLeds < WS2812B, ledpin, GRB > (leds, numleds);
  FastLED.setBrightness(20);

  String switchIpstring = WiFiSettings.string("switcher_Ip", 7, 15, "192.168.1.121", "Atem Switcher IP");

  switcherIp.fromString(switchIpstring);

  cameraNumber = WiFiSettings.integer("cameraID", 1, 9, 1, "Follow Camera Input");

  allowAtemControl = WiFiSettings.checkbox("allow_Atem_Control", false, "Allow Atem Control");
  allowAtemSetPreview = WiFiSettings.checkbox("allow_Atem_Set_Preview", false, "Atem Set Preview on Control");
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

  if ( (millis() - lastAtemUpdate) > 100 ) {  // If lastAtemUpdate happened more then x seconds ago, AtemSwitcher.runLoop() will update the info in the AtemSwitcher. Might fail after 49.7 days.
    
    // Check for packets, respond to them etc. Keeping the connection alive!
    AtemSwitcher.runLoop();    
  
    lastAtemUpdate = millis();
  }
  
  // Get current state of this camera: is it live or preview? (Might have changed after pushing the button as well)
  ProgramTally = AtemSwitcher.getProgramTally(cameraNumber);
  PreviewTally = AtemSwitcher.getPreviewTally(cameraNumber);

  // Check if this new state is diffrent from the last state. If it has changed, we'll update the display and LED's
  if ((ProgramTallyPrevious != ProgramTally) || (PreviewTallyPrevious != PreviewTally)) {

    if ((ProgramTally) ) { // If our camera == Program (live)
      //leds[0] = CRGB::Red;
      fill_solid(leds, numleds, CRGB(255,0,0));  // fill red
      FastLED.show();
    } else if (PreviewTally) { // If our camera == Preview
      //leds[0] = CRGB::Green;
      fill_solid(leds, numleds, CRGB(0,255,0));  // fill green
      FastLED.show();
    } else if (!PreviewTally || !ProgramTally) { // If our camera != preview or program: do something else.
      showNumber(cameraNumber);
    }
    Serial.println("Preview or Program changed"); // Testing

    // set previous state to current.
    ProgramTallyPrevious = ProgramTally;
    PreviewTallyPrevious = PreviewTally;
  }

  if ( !digitalRead(buttonpin)) {
    if ((millis() - lastAtemUpdatePush) > 500 && AtemSwitcher.isConnected() && allowAtemControl && AtemSwitcher.getProgramInput() != cameraNumber) {
      fill_solid(leds, numleds, CRGB(0,0,255));  // fill Blue
      FastLED.show();
      //void changeProgramInput(uint16_t inputNumber);
      if (allowAtemSetPreview) {
        AtemSwitcher.changePreviewInput(AtemSwitcher.getProgramInput());
      }
      AtemSwitcher.changeProgramInput(cameraNumber);
      // Check for packets, respond to them etc. Keeping the connection alive!
      AtemSwitcher.runLoop(); // ToDo Does this command have to be here?
      Serial.println("Button pushed, changed input and runLoop?"); // Testing
      lastAtemUpdatePush = millis();
    } else if ((millis() - lastAtemUpdatePush) > 500 && allowAtemControl && AtemSwitcher.isConnected() == false) {
      Serial.println("Button Pushed, but not connected to ATEM");
      fill_solid(leds, numleds, CRGB(0,0,255));  // fill Blue
      FastLED.show();
      lastAtemUpdatePush = millis();
    } else if ((millis() - lastAtemUpdatePush) > 500 && allowAtemControl == false) {
      Serial.println("Button Pushed, but not allowed in GUI");
      lastAtemUpdatePush = millis();
    }
  }
  
  hue += 1;
  // FastLED.show(); // Unneccesary, but why not.
  // delay(25); // Delay zou niet nodig moeten zijn.
}
