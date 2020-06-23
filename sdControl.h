#ifndef _SD_CONTROL_H_
#define _SD_CONTROL_H_

#define SPI_BLOCKOUT_PERIOD	20000UL

class SDControl {
public:
  SDControl() { }
  static void setup();
  static void takeBusControl();
  static void relinquishBusControl();
  static bool canWeTakeBus();
 
private:
  static volatile long _spiBlockoutTime;
  static bool _weTookBus;
};

extern SDControl sdcontrol;

#endif
