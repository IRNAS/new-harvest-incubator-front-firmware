# new-harvest-incubator-front-firmware
New Harvest Incubator firmware.

Control the Incubator and construct mobile app and web-interface. In addition to the added libraries you will need:

* Arduino IDE to run the main incubator_NewHarvest.ino sketch. 
* Suport for Adafruit ESP32 Feather, follow the instructions: https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide .
* Add OneWire library from the Library manager inside Aruino IDE.

To control the incubator run the [incubator_NewHarvest.ino](https://github.com/IRNAS/new-harvest-incubator-front-firmware/blob/master/incubator_NewHarvest.ino) sketch. In the main sketch Blynk app is defined along with the web-interface. 

For detailed description of the control algorithm, read the readme section in the [Incubator.h](https://github.com/IRNAS/new-harvest-incubator-front-firmware/blob/master/Incubator.h) file. 

For the user manual, see [Instructions](https://github.com/IRNAS/new-harvest-incubator-front-firmware/blob/master/Instructions.pdf).
