# vizmote
ESP8266 sketch for local network control of Wiz platform lighting

## Usage

Assumes a button attached to Pin 5, and an LED on Pin 0 (Adafruit Feather huzzah).

Long Click - Registration mode. Scans the local network with mask `192.168.1.255` looking for lights in Plant Growth mode. Any lights in that mode are enrolled for remote control.

Single Click - Toggle the state of enrolled lights.
