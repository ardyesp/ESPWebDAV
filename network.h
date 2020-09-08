#ifndef _NETWORK_H_
#define _NETWORK_H_

#define HOSTNAME		"FYSETC"
#define SERVER_PORT		80

#define WIFI_CONNECT_TIMEOUT 30000UL

class Network {
public:
  Network() { initFailed = false;wifiConnecting = true;}
  bool start();
  int startDAVServer();
  bool isConnected();
  bool isConnecting();
  void handle();
  bool ready();

private:
  bool wifiConnected;
  bool wifiConnecting;
  bool initFailed;
};

extern Network network;

#endif
