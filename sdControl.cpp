#include <ESP8266WiFi.h>
#include "sdControl.h"
#include "pins.h"

volatile long SDControl::_spiBlockoutTime = 0;
bool SDControl::_weTookBus = false;

void SDControl::setup() {
  // ----- GPIO -------
	// Detect when other master uses SPI bus
	pinMode(CS_SENSE, INPUT);
	attachInterrupt(CS_SENSE, []() {
		if(!_weTookBus)
			_spiBlockoutTime = millis() + SPI_BLOCKOUT_PERIOD;
	}, FALLING);

	// wait for other master to assert SPI bus first
	delay(SPI_BLOCKOUT_PERIOD);
}

// ------------------------
void SDControl::takeBusControl()	{
// ------------------------
	_weTookBus = true;
	//LED_ON;
	pinMode(MISO_PIN, SPECIAL);	
	pinMode(MOSI_PIN, SPECIAL);	
	pinMode(SCLK_PIN, SPECIAL);	
	pinMode(SD_CS, OUTPUT);
}

// ------------------------
void SDControl::relinquishBusControl()	{
// ------------------------
	pinMode(MISO_PIN, INPUT);	
	pinMode(MOSI_PIN, INPUT);	
	pinMode(SCLK_PIN, INPUT);	
	pinMode(SD_CS, INPUT);
	//LED_OFF;
	_weTookBus = false;
}

bool SDControl::canWeTakeBus() {
	if(millis() < _spiBlockoutTime) {
    return false;
  }
  return true;
}
