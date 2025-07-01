# vsx919ah-controller

Pioneer VSX919ah Wifi controller

An ESP32 sketch to control a VSX919ah AV Receiver via the 'IR in' port.

Connects to the receiver with a 3.5mm jack and the ESP32 hosts a webpage to allow network remote control. Unlike control via the 'SR in' port, the IR remote will still work.

I power the ESP32 via the receivers USB port, which allows me to detect wheter the AVR is turned on and use that info to have KODI automatically switch audio outputs from the TV to the AVR and vice-versa. As such, the webUI does not contain navigation buttons, only input and some control buttons, but extra buttons can be added.

IR codes were obtained from a aftermarket AXD7534 remote and a Logitech Harmony remote using an ESP32 with a IR receiver diode.

This sketch needs to be compiled with the 2.0.14 version of the esp32 board data in the Arduino IDE.

To wire up the 3.5mm mono jack, connect the sleeve to ground and the tip to the GPIO port designated in the sketch.