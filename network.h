#ifndef _NETWORK_H_
#define _NETWORK_H_

#define HOSTNAME		"FYSETC"
#define SERVER_PORT		80

#define SD_CS		4
#define MISO_PIN		12
#define MOSI_PIN		13
#define SCLK_PIN		14
#define CS_SENSE	5

#define SPI_BLOCKOUT_PERIOD	20000UL
#define WIFI_CONNECT_TIMEOUT 30000UL

class Network {
public:
  Network() { initFailed = false;}
  static void setup();
  static void takeBusControl();
  static void relenquishBusControl();
  bool start();
  void startDAVServer();
  bool isConnected();
  void handle();
  bool ready();

private:
  bool wifiConnected;
  bool initFailed;
  static volatile long _spiBlockoutTime;
  static bool _weHaveBus;
};

extern Network network;

#endif
