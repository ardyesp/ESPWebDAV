// Using the WebDAV server

#include <ESP8266WiFi.h>
#include "ESPWebDAV.h"

#define HOSTNAME	"Rigidbot"
#define SERVER_PORT	80
#define SD_CARD_CS_PIN	15


const char *ssid = "ssid";
const char *password = "passwd";

ESPWebDAV dav;
String statusMessage;
bool initFailed = false;


// ------------------------
void setup() {
// ------------------------
	WiFi.mode(WIFI_STA);
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	WiFi.hostname(HOSTNAME);
	delay(1000);
	Serial.begin(115200);
	WiFi.begin(ssid, password);
	Serial.println("");

	// Wait for connection
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to "); Serial.println(ssid);
	Serial.print ("IP address: "); Serial.println(WiFi.localIP());
	Serial.print ("RSSI: "); Serial.println(WiFi.RSSI());
	Serial.print ("Mode: "); Serial.println(WiFi.getPhyMode());
	
	// start the SD DAV server
	if(!dav.init(SD_CARD_CS_PIN, SERVER_PORT))		{
		statusMessage = "Failed to initialize SD Card";
		Serial.print("ERROR: "); Serial.println(statusMessage);
		initFailed = true;
	}

	Serial.println("WebDAV server started");
}



// ------------------------
void loop() {
// ------------------------
	// call handle if server was initialized properly
	if(!initFailed)
		dav.handleClient();
	else
		dav.rejectClient(statusMessage);
}


