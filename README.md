# esp8266-fridge
ESP8266-based simple fridge temperature controller with Web GUI

This Arduino sketch for ESP8266 platform implements a simple temperature controller that drives fridge power on/off states based on temperature measured by DS18B20 digital thermometer readings and temperature/hysteresis parameters configured via a Web GUI.
It uses WifiManager to connect to Wifi networks so no hardcoded credentials are stored in the source code.

Hardware used:
==============
1. Wemos D1 mini board (or its clone)
2. Electromagnetic 10A relay board (one can use SSR instead)
3. DS180B20 digital thermometer
4. AC plug, socket, AC connection cables and some case

Software prerequisites:
======================
All libraries are available through Arduino IDE Library Manager

1. ArduinoJson
2. DallasTemperature
3. OneWire
4. ThingSpeak
5. WifiManager
