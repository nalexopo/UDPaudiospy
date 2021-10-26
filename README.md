# UDPaudiospy
An esp32 WiFi UDP stream Microphone and a python backend for spying a room straight to your computer speakers.

## Hardware

Adafruit HUZZAH32 – ESP32 Feather Board
Adafruit I2S MEMS Microphone Breakout - SPH0645LM4H

                      Connections

|  Esp32 HUZZAH Silkscreen	| SPH0645LM4H  Breakout 	    |
|---	                      |---	                        |
|  3V 	                    | 3V   	                      |
|  GND 	                    | GND   	                    |
|  SCK/5 	                  | BCLK   	                    |
|  MISO/19 	                | DOUT  	                    |
|  21 	                    | LRCL  	                    |

## Software

Software is copy pasta and collage of different idf examples, some codes found on the internet and filling in the blanks myself. 
Thanks to all people out there making public your work.

1. Make blank IDF project and copy paste main
2. Make sure you enter your WIFI SSID and PASSWORD in the proper #defines inside esp32 code. (XXX fields)
3. Add your computer ip and port both in python backend and esp32 code. (XXX fields)
4. Build and download esp32 firmware
5. Run Python Backend

PS make sure that your Python can run on your network without firewall blocking it.

