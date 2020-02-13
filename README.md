# esp8266-fridge
ESP8266-based simple fridge temperature controller with Web GUI

This Arduino sketch for ESP8266 platform implements a simple temperature controller that drives fridge power on/off states based on temperature measured by DS18B20 digital thermometer readings and temperature/hysteresis parameters configured via a Web GUI. The controller can be configured to work as cooler and heater.
It uses WifiManager to connect to Wifi networks so no hardcoded credentials are stored in the source code.

Basic view:

![Basic view](https://github.com/eltomek/esp8266-fridge/blob/master/Screen1.png)

ThingSpeak IoT platform is supported so you can plot temperature values like this:

![Data plot](https://github.com/eltomek/esp8266-fridge/blob/master/Screen2.png)

## Hardware used:
1. Wemos D1 mini board (or its clone)
2. Electromagnetic 10A relay board (one can use SSR instead)
3. DS180B20 digital thermometer
4. AC plug, socket, AC connection cables and some case

## Software prerequisites:
All libraries are available through Arduino IDE Library Manager

1. ArduinoJson
2. DallasTemperature
3. OneWire
4. ThingSpeak
5. WifiManager

## Setup instructions:
Wemos D1 mini example:
* connect the Data/DQ line of our DS18B20 to the D4 (GPIO2) pin
* connect the relay control pin to the D2 (GPIO4) pin (it can be described as IN)
* connect the power supply and ground of the relay (5V) and DS18B20 (3.3V)
* optionally, put a 4.7kOhm resistor between the Data/DQ line and the thermometer power supply

### Compiling software:
Use the Arduino IDE (https://www.arduino.cc/en/main/software), add esp8266 support (https://github.com/esp8266/Arduino/blob/master/README.md#installing-with- boards-manager), install the necessary libraries (https://github.com/eltomek/esp8266-fridge#software-prerequisites) using the Library Manager in the Arduino IDE.

If your esp8266 board's  WiFi network has not previously been configured it will display it will enter the Access Point mode. Open the page at http://192.168.4.1 and configure the network connections (more info: https://github.com/tzapu/WiFiManager#how-it-works).
