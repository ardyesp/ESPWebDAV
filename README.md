# WebDAV Server and a 3D Printer
This project is a WiFi WebDAV server using ESP8266 SoC. It maintains the filesystem on an SD card.

Supports the basic WebDav operations - *PROPFIND*, *GET*, *PUT*, *DELETE*, *MKCOL*, *MOVE* etc.

Once the WebDAV server is running on the ESP8266, a WebDAV client like Windows can access the filesystem on the SD card just like a cloud drive. The drive can also be mounted like a networked drive, and allows copying/pasting/deleting files on SD card remotely.

### 3D Printer

I am using this setup as a networked drive for 3D Printer running Marlin. Following circuit with ESP8266 and a MicroSD adapter is fabricated on a PCB. A full size SD card adapter is glued to one end and provides access to all SPI data lines from printer. ESP8266 code avoids accessing micro SD card, when Marlin (printer's firmware) is reading/writing to it (detected using Chip Select line).

GCode can be directly uploaded from the slicer (Cura) to this remote drive, thereby simplifying the workflow. 


![Printer Hookup Diagram](PrinterHookup2.jpg)

## Dependencies:
1. [ESP8266 Arduino Core version 2.4](https://github.com/esp8266/Arduino)
2. [SdFat library](https://github.com/greiman/SdFat)
  

## Use:
Compile and upload the program to an ESP8266 module. ESP12-E was used for development and testing.
Connect the SPI bus lines to SD card.

ESP Module|SD Card
---|---
GPIO13|MOSI   
GPIO12|MISO   
GPIO14|SCK    
GPIO4|CS   
GPIO5|CS Sense   

The card should be formatted for Fat16 or Fat32

To access the drive from Windows, type ```\\esp_hostname_or_ip\DavWWWRoot``` at the Run prompt, or use Map Network Drive menu in Windows Explorer.

## References
Marlin Firmware - [http://marlinfw.org/](http://marlinfw.org/)   

Cura Slicer - [https://ultimaker.com/en/products/ultimaker-cura-software](https://ultimaker.com/en/products/ultimaker-cura-software)   

3D Printer LCD and SD Card Interface - [http://reprap.org/wiki/RepRapDiscount_Full_Graphic_Smart_Controller](http://reprap.org/wiki/RepRapDiscount_Full_Graphic_Smart_Controller)   

LCD Schematics - [http://reprap.org/mediawiki/images/7/79/LCD_connect_SCHDOC.pdf](http://reprap.org/mediawiki/images/7/79/LCD_connect_SCHDOC.pdf)   



