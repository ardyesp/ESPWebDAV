# ESPWebDAV
A WiFi WebDAV server using ESP8266. Maintains the filesystem on an SD card.

Supports the basic WebDav operations.


## Dependencies:
1. [ESP8266 Arduino Core version 2.4](https://github.com/esp8266/Arduino)
2. [SdFat library](https://github.com/greiman/SdFat)
  

## Use:
Compile and upload the program to an ESP8266 module. ESP12-E was used for development and testing.
Connect the SPI bus lines to SD card.   
ESP Module  SD Card   
13  MOSI   
12  MISO   
14  SCK    
15  CS   

The card should be formatted for Fat16 or Fat32

To access the drive from Windows, type ```\\esp_hostname_or_ip\DavWWWRoot``` at the Run prompt, or use Map Network Drive menu in Windows Explorer.
