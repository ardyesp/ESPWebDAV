#ifndef _NETWORK_H_
#define _NETWORK_H_

#define HOSTNAME		"FYSETC"
#define SERVER_PORT		80

#define WIFI_CONNECT_TIMEOUT 30000UL

class Network {
public:
  Network() { initFailed = false;}
  bool start();
  int startDAVServer();
  bool isConnected();
  void handle();
  bool ready();

private:
  bool wifiConnected;
  bool initFailed;
};

extern Network network;

#endif
