
This project is an easy-to-use test setup for the Analog Devices' AD9850 chip: a DDS Synthesizer.

A PC (Windows) software does control a Teensy microcontroller with parameters like frequency and phase.
  The PC software controls the Teensy controller through a HID USB connection. You don't need a driver :-)
  Parametrized settings can be stored on the microcontollers' EEPROM ..
    and you can connect an USB power bank to power the microcontroller and AD9850,
    that removes some noise on the output.

The Teensy microcontroller is connected to the AD9850 test board and controls it.

ATTENTION: take care not to oversteer other devices with the output signal!


HARDWARE PARTLIST:

* 1 x AD9850 Test board TE439:
https://www.amazon.de/gp/product/B01HHLQQ4I/

* 1 x Teensy 3.1 with pins (you should be able to other another Teensy model with enough pins)
https://www.pjrc.com/store/teensy31.html
http://www.watterott.com/de/Teensy-USB-Board-v31-MK20DX256VLH7-mit-Pins
for overview, see https://www.pjrc.com/teensy/index.html

* 2 x small Breadboards
http://www.pollin.de/shop/dt/MDM2OTg0OTk-
http://www.watterott.com/de/Breadboard-klein-selbstklebend

* some Jumper Wirers M/M
http://www.watterott.com/de/Jumper-Wires-MM-200mm

* optional BNC Adapter
http://www.ebay.de/itm/172548061062
http://www.ebay.de/itm/192066626283

* optional: 30 dB attenuator
to allow connection to a receiver's input


SOFTWARE PREREQUISITES for Development/Microcontroller Upload:
* Arduino 1.8.2 / Genuino with Teensyduino 1.36
https://www.pjrc.com/teensy/td_download.html


SOFTWARE PREREQUISITES for Development/PC Control:
* Visual Studio Express 2013 for Windows Desktop - or higher version.
With minor modification you should also be able to use other compiler.
https://www.microsoft.com/en-us/download/details.aspx?id=44914



Some Links / References:
* http://www.analog.com/media/en/technical-documentation/data-sheets/AD9850.pdf
* http://jontio.zapto.org/hda1/JDDS.html
* https://electronicfreakblog.wordpress.com/2014/01/29/dds-signalgenerator-mit-ad9850/
* http://www.nmea.de/schaltung/sch-dds9850.html


LICENSE:
MIT, (c)2017 Hayati Ayguen <h_ayguen@web.de>

exceptions:
- Teensy RawHID source files
  see http://www.pjrc.com/teensy/rawhid.html

- Arduino Controller Library for AD9850 Module/Shield
  see www.arduino-projekte.de
  http://www.arduino-projekte.de/index.php?n=7
  http://www.arduino-projekte.de/download.php?id=28

  for compilation/upload you need to copy the files
    AH_AD8950.cpp and AH_AD8950.h from AH_AD9850_20121026.zip
    into the subfolder ad9850_arduino
      besides the Arduino project file ad9850_arduino.ino.

