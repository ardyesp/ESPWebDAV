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
### Compile and upload

#### Compile

If you don't want to update the firmware. You don't need to do this. Compile and upload the program to an ESP8266 module. 

- Open the project

  Download this project and open it with [arduino](https://www.arduino.cc/) software.

- Add board manager link

  Add boards manager link: `https://arduino.esp8266.com/stable/package_esp8266com_index.json` to File->Preferences board manager, Documentation: https://arduino-esp8266.readthedocs.io/en/2.7.1/ 

- Select board

  Select Tools->boards->Generic ESP8285 Module.

- Click the Arduino compile button

#### Upload

1. Pulg in the USB cable to your computer.
2. Press and hold the module FLSH 
3. Connect the USB cable to the module
4. Release the module FLSH button
5. Click the Arduino upload button

### Config

First you can see our video [here](https://www.youtube.com/watch?v=YAFAK-jPcOs). You have two ways to config the module.

*note: The card should be formatted for Fat16 or Fat32*

#### Option 1: INI file

You can edit the example ```SETUP.INI``` file in ```ini``` folder, change the SSID and PASSWORD value. And then copy ```SETUP.INI``` file to your root SD card. Then insert it to the module. 

1. Turn the module option button to ```USB2UART``` 
2. Open a COM software in your computer
3. Connect the module to your computer with USB cable
4. Open the software COM port

you can see the module IP and other information.

*note: if you miss the serial output, you can click the ```RST``` button in the module.*

#### Option 2 : Command

Insert your sdcard to the module.

1. Turn the module option button to ```USB2UART``` 
2. Open a COM software in your computer
3. Connect the module to your computer with USB cable
4. Open the software COM port

And use the following command to connect the network or check the network status

    M50: Set the wifi ssid , 'M50 ssid-name'
    M51: Set the wifi password , 'M51 password'
    M52: Start to connect the wifi
    M53: Check the connection status
### Access

#### windows

To access the drive from Windows, type ```\\ip\DavWWWRoot``` at the Run prompt, this will show in serial output as our [video](https://www.youtube.com/watch?v=YAFAK-jPcOs) shows.

Or use Map Network Drive menu in Windows Explorer.

#### MAC

Just need to use  ```http://192.168.0.x``` in access network drive option

## References
Marlin Firmware - [http://marlinfw.org/](http://marlinfw.org/)   

Cura Slicer - [https://ultimaker.com/en/products/ultimaker-cura-software](https://ultimaker.com/en/products/ultimaker-cura-software)   

3D Printer LCD and SD Card Interface - [http://reprap.org/wiki/RepRapDiscount_Full_Graphic_Smart_Controller](http://reprap.org/wiki/RepRapDiscount_Full_Graphic_Smart_Controller)   

LCD Schematics - [http://reprap.org/mediawiki/images/7/79/LCD_connect_SCHDOC.pdf](http://reprap.org/mediawiki/images/7/79/LCD_connect_SCHDOC.pdf)   



