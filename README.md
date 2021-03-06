# ESP32Tally
A "Tallylight" based on the ESP32.

## What?
A Tallylight is a light on or near a camera, which indicates if this camera is Live (Actual) and with this project also which camera is Preview.

It connects directly on ATEM-Videoswitchers thanks to the work of Skaarhoj, no other running software is needed.  
The plan is to extend this project to be compatible with OBS.  
The configuration is done in a web-gui, based on the WiFi-Settings project bij Juerd.

## Setup:
After initial flashing, the leds on the M5Atom will change colors, indicating that it is in setup-mode.  
To get to the setup-mode later: Press and hold the front-button of the M5Atom, reset with the button on the side, and then release the front-button when the led's rotate colors.  
Your ESP will be in AccessPoint mode, to which you can connect with a phone.  
The Web-Interface is available on http://192.168.4.1

## Etc.
Developed on M5Atom, but without using their libraries so it could work on other ESP32-based boards, and probably even ESP8266.

The Skaarhoj libraries are no longer updated, but the parts of the protocol we use in this project seem to still be working in the current ATEM Switchers 8.6 Update, 18 Feb 2021.  

## Modify the Skaarhoj library files for use with ESP32. (Source: oneguyoneblog.com)

For the Skaarhoj libraries to work on and ESP32, a few changes need to be made!  

The library is compatible with Arduino (with Ethernet shield) and the ESP8266 (WiFi), but not with the ESP32. This is easily remedied by making a total of 3 changes to the two files ATEMbase.cpp and ATEMbase.h.

In libraries/ATEMbase/ATEMbase.cpp, around line 50:

Search for
```
	// Set up Udp communication object:
#ifdef ESP8266
WiFiUDP Udp;
#else
EthernetUDP Udp;
#endif
```
and replace by:
```
	// Set up Udp communication object:
WiFiUDP Udp;
```
 

In the second file, libraries/ATEMbase/ATEMbase.h, around line 35:
```
#ifdef ESP8266
#include <WifiUDP.h>
#else
#include <EthernetUdp.h>
#endif
```
replace this with the following line (mind the uppercase and lowercase):
```
 #include <WiFiUdp.h>
```

The second change in this file, around line 60, look for this snippet:
```
#ifdef ESP8266
WiFiUDP _Udp;
#else
EthernetUDP _Udp;	// UDP object for communication, see constructor.
#endif
```
and replace it with:
```
WiFiUDP _Udp;
```
You can now use the ATEM library in your ESP32 projects.
