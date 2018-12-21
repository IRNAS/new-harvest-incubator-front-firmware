# new-harvest-incubator-front-firmware
New Harvest Incubator firmware.

Control the Incubator and construct mobile app and web-interface. In addition to added libraries you need:

* Arduino IDE to run the main incubator_NewHarvest.ino sketch. 
* Suport for Adafruit ESP32 Feather, follow the instructions: https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide
* Add OneWire library in Library manager inside Aruino IDE.

To control the incubator run the incubator_NewHarvest.ino sketch. In the main sketch Blynk app is defined along with the web-interface. 

For detailed description of the control algorithm, read the readme section in the Incubator.h file. 