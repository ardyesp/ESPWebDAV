#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SdFat.h>
#include <EEPROM.h>
#include "pins.h"
#include "config.h"
#include "serial.h"
#include "sdControl.h"

int Config::loadSD() {
  SdFat sdfat;

  SERIAL_ECHOLN("Going to load config from INI file");

  if(!sdcontrol.canWeTakeBus()) {
    SERIAL_ECHOLN("Marlin is controling the bus");
    return -1;
  }
  sdcontrol.takeBusControl();
  
  if(!sdfat.begin(SD_CS, SPI_FULL_SPEED)) {
    SERIAL_ECHOLN("Initial SD failed");
    sdcontrol.relinquishBusControl();
    return -2;
  }

  File file = sdfat.open("SETUP.INI", FILE_READ);
  if (!file) {
    SERIAL_ECHOLN("Open INI file failed");
    sdcontrol.relinquishBusControl();
    return -3;
  }

  // Get SSID and PASSWORD from file
  int rst = 0,step = 0;
  String buffer,sKEY,sValue;
  while (file.available()) { // check for EOF
    buffer = file.readStringUntil('\n');
    if(buffer.length() == 0) continue; // Empty line
    buffer.replace(" ", ""); // Delete all blank
    buffer.replace("\r", ""); // Delete all CR
    int iS = buffer.indexOf('='); // Get the seperator
    if(iS < 0) continue; // Bad line
    sKEY = buffer.substring(0,iS);
    sValue = buffer.substring(iS+1);
    if(sKEY == "SSID") {
      SERIAL_ECHOLN("INI file : SSID found");
      if(sValue.length()>0) {
        memset(data.ssid,'\0',WIFI_SSID_LEN);
        sValue.toCharArray(data.ssid,WIFI_SSID_LEN);
        step++;
      }
      else {
        rst = -4;
        goto FAIL;
      }
    }
    else if(sKEY == "PASSWORD") {
      SERIAL_ECHOLN("INI file : PASSWORD found");
      if(sValue.length()>0) {
        memset(data.psw,'\0',WIFI_PASSWD_LEN);
        sValue.toCharArray(data.psw,WIFI_PASSWD_LEN);
        step++;
      }
      else {
        rst = -5;
        goto FAIL;
      }
    }
    else continue; // Bad line
  }
  if(step != 2) { // We miss ssid or password
    //memset(data,) // TODO: do we need to empty the data?
    SERIAL_ECHOLN("Please check your SSDI or PASSWORD in ini file");
    rst = -6;
    goto FAIL;
  }

  FAIL:
  file.close();
  sdcontrol.relinquishBusControl();
  return rst;
}

unsigned char Config::load() {
  // Try to get the config from ini file
  if(0 == loadSD())
  {
    return 1; // Return as connected before
  }

  SERIAL_ECHOLN("Going to load config from EEPROM");

  EEPROM.begin(EEPROM_SIZE);
  uint8_t *p = (uint8_t*)(&data);
  for (int i = 0; i < sizeof(data); i++)
  {
    *(p + i) = EEPROM.read(i);
  }
  EEPROM.commit();

  if(data.flag) {
    SERIAL_ECHOLN("Going to use the old config to connect the network");
  }
  SERIAL_ECHOLN("We didn't connect the network before");
  return data.flag;
}

char* Config::ssid() {
  return data.ssid;
}

void Config::ssid(char* ssid) {
  if(ssid == NULL) return;
  strncpy(data.ssid,ssid,WIFI_SSID_LEN);
}

char* Config::password() {
  return data.psw;
}

void Config::password(char* password) {
  if(password == NULL) return;
  strncpy(data.psw,password,WIFI_PASSWD_LEN);
}

void Config::save(const char*ssid,const char*password) {
  if(ssid ==NULL || password==NULL)
    return;

  EEPROM.begin(EEPROM_SIZE);
  data.flag = 1;
  strncpy(data.ssid, ssid, WIFI_SSID_LEN);
  strncpy(data.psw, password, WIFI_PASSWD_LEN);
  uint8_t *p = (uint8_t*)(&data);
  for (int i = 0; i < sizeof(data); i++)
  {
    EEPROM.write(i, *(p + i));
  }
  EEPROM.commit();
}

void Config::save() {
  if(data.ssid == NULL || data.psw == NULL)
    return;

  EEPROM.begin(EEPROM_SIZE);
  data.flag = 1;
  uint8_t *p = (uint8_t*)(&data);
  for (int i = 0; i < sizeof(data); i++)
  {
    EEPROM.write(i, *(p + i));
  }
  EEPROM.commit();
}

// Save to ip address to sdcard
int Config::save_ip(const char *ip) {
  SdFat sdfat;

  SERIAL_ECHOLN("Going to save config to ip.gcode file");

  if(!sdcontrol.canWeTakeBus()) {
    SERIAL_ECHOLN("Marlin is controling the bus");
    return -1;
  }
  sdcontrol.takeBusControl();
  
  if(!sdfat.begin(SD_CS, SPI_FULL_SPEED)) {
    SERIAL_ECHOLN("Initial SD failed");
    sdcontrol.relinquishBusControl();
    return -2;
  }

  // Remove the old file
  sdfat.remove("ip.gcode");

  File file = sdfat.open("ip.gcode", FILE_WRITE);
  if (!file) {
    SERIAL_ECHOLN("Open ip file failed");
    sdcontrol.relinquishBusControl();
    return -3;
  }

  // Get SSID and PASSWORD from file
  char buf[21] = "M117 ";
  strncat(buf,ip,15);
  file.write(buf, 21);
  file.close();
}

Config config;
