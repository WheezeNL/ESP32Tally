# ESP32Tally
A "Tallylight" based on the ESP32.

## What?
A Tallylight is a light on or near a camera, which indicates if this camera is Live (Actual) and with this project also which camera is Preview.

It connects directly on ATEM-Videoswitchers thanks to the work of Skaarhoj, no other running software is needed.
The plan is to extend this project to be compatible with OBS.

## Setup:
The very basic setup is done in a web-gui, based on the WiFi-Settings project bij Juerd.
After initial flashing, the leds on the M5Atom will change colors, indicating that it is in setup-mode.
To get to the setup-mode later: Press and hold the front-button of the M5Atom, reset with the button on the side, and then release the front-button when the led's rotate colors.

Your ESP will be in AccessPoint mode, to which you can connect with a phone.
The Web-Interface is available on http://192.168.4.1

## Etc.
Developed on M5Atom, but without using their libraries so it could work on other ESP32-based boards, and probably even ESP8266.

The Skaarhoj libraries are no longer updated, but the parts of the protocol we use in this project seem to still be working in the current ATEM Switchers 8.6 Update, 18 Feb 2021.
