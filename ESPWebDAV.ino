// Using the WebDAV server with Rigidbot 3D printer.
// Printer controller is a variation of Rambo running Marlin firmware

#include "ESP8266WiFi.h"
#include "ESPWebDAV.h"
#include "serial.h"
#include "parser.h"
#include "Config.h"

// LED is connected to GPIO2 on this board
#define INIT_LED			{pinMode(2, OUTPUT);}
#define LED_ON				{digitalWrite(2, LOW);}
#define LED_OFF				{digitalWrite(2, HIGH);}

#define HOSTNAME		"FYSETC"
#define SERVER_PORT		80
#define SPI_BLOCKOUT_PERIOD	20000UL
#define WIFI_CONNECT_TIMEOUT 30000UL

#define SD_CS		4
#define MISO		12
#define MOSI		13
#define SCLK		14
#define CS_SENSE	5

ESPWebDAV dav;
String statusMessage;
bool initFailed = false;

volatile long spiBlockoutTime = 0;
bool weHaveBus = false;
bool wifiConnected = false;

bool wifiConnect(const char*ssid,const char*password) {
  if(ssid == NULL || password==NULL) {
    SERIAL_ECHOLN("Please set the wifi ssid and password first");
  }

  // 
  wifiConnected = false;
  
  // Set hostname first
  WiFi.hostname(HOSTNAME);
  // Reduce startup surge current
  WiFi.setAutoConnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  WiFi.begin(ssid, password);

  // Wait for connection
  unsigned long timeout = millis();
  while(WiFi.status() != WL_CONNECTED) {
    blink();
    DBG_PRINT(".");
    if(millis() > timeout + WIFI_CONNECT_TIMEOUT)
      return false;
  }

  DBG_PRINTLN("");
  DBG_PRINT("Connected to "); DBG_PRINTLN(ssid);
  DBG_PRINT("IP address: "); DBG_PRINTLN(WiFi.localIP());
  DBG_PRINT("RSSI: "); DBG_PRINTLN(WiFi.RSSI());
  DBG_PRINT("Mode: "); DBG_PRINTLN(WiFi.getPhyMode());

  wifiConnected = true;

  config.save(ssid,password);

  return true;
}

void startDAVServer() {
  // Check to see if other master is using the SPI bus
  while(millis() < spiBlockoutTime)
    blink();
  
  takeBusControl();
  
  // start the SD DAV server
  if(!dav.init(SD_CS, SPI_FULL_SPEED, SERVER_PORT))   {
    statusMessage = "Failed to initialize SD Card";
    DBG_PRINT("ERROR: "); DBG_PRINTLN(statusMessage);
    // indicate error on LED
    errorBlink();
    initFailed = true;
  }
  else
    blink();
  
  relenquishBusControl();
  DBG_PRINTLN("WebDAV server started");
}

// ------------------------
void setup() {
	// ----- GPIO -------
	// Detect when other master uses SPI bus
	pinMode(CS_SENSE, INPUT);
	attachInterrupt(CS_SENSE, []() {
		if(!weHaveBus)
			spiBlockoutTime = millis() + SPI_BLOCKOUT_PERIOD;
	}, FALLING);
	
	DBG_INIT(115200);
	DBG_PRINTLN("");
	INIT_LED;
	blink();
	
	// wait for other master to assert SPI bus first
	delay(SPI_BLOCKOUT_PERIOD);

	// ----- WIFI -------
  if(config.load() == 1) { // Connected before
    if(wifiConnect(config.ssid(), config.password())) {
  	  startDAVServer();
  	}
    else {
      SERIAL_ECHOLN("Connect fail, please set the wifi config and connect again");
    }
  }
  else {
    SERIAL_ECHOLN("Please set the wifi ssid with M50 and password with M51 , and start connection with M52");
  }
}

#define MAX_CMD_SIZE 96
#define BUFSIZE 4

/**
 * GCode Command Queue
 * A simple ring buffer of BUFSIZE command strings.
 *
 * Commands are copied into this buffer by the command injectors
 * (immediate, serial, sd card) and they are processed sequentially by
 * the main loop. The process_next_command function parses the next
 * command and hands off execution to individual handler functions.
 */
uint8_t commands_in_queue = 0, // Count of commands in the queue
        cmd_queue_index_r = 0, // Ring buffer read (out) position
        cmd_queue_index_w = 0; // Ring buffer write (in) position

uint32_t command_sd_pos[BUFSIZE];
volatile uint32_t current_command_sd_pos;

char command_queue[BUFSIZE][MAX_CMD_SIZE];

static bool send_ok[BUFSIZE];

// Number of characters read in the current line of serial input
static int serial_count; // = 0;

/**
 * Once a new command is in the ring buffer, call this to commit it
 */
inline void _commit_command(bool say_ok) {
  send_ok[cmd_queue_index_w] = say_ok;
  if (++cmd_queue_index_w >= BUFSIZE) cmd_queue_index_w = 0;
  commands_in_queue++;
}

/**
 * Copy a command from RAM into the main command buffer.
 * Return true if the command was successfully added.
 * Return false for a full buffer, or if the 'command' is a comment.
 */
inline bool _enqueuecommand(const char* cmd, bool say_ok=false) {
  if (*cmd == ';' || commands_in_queue >= BUFSIZE) return false;
  strcpy(command_queue[cmd_queue_index_w], cmd);
  _commit_command(say_ok);
  return true;
} 

/**
 * Get all commands waiting on the serial port and queue them.
 * Exit when the buffer is full or when no more characters are
 * left on the serial port.
 */
void get_serial_commands() {
  static char serial_line_buffer[MAX_CMD_SIZE];
  static bool serial_comment_mode = false;

  /**
   * Loop while serial characters are incoming and the queue is not full
   */
  int c;
  while (commands_in_queue < BUFSIZE && (c = Serial.read()) >= 0) {

    char serial_char = c;

    /**
     * If the character ends the line
     */
    if (serial_char == '\n' || serial_char == '\r') {

      serial_comment_mode = false;                      // end of line == end of comment

      // Skip empty lines and comments
      if (!serial_count) { continue; }

      serial_line_buffer[serial_count] = 0;             // Terminate string
      serial_count = 0;                                 // Reset buffer

      char* command = serial_line_buffer;

      while (*command == ' ') command++;                // Skip leading spaces

      // Add the command to the queue
      _enqueuecommand(serial_line_buffer, true);
    }
    else if (serial_count >= MAX_CMD_SIZE - 1) {
      // Keep fetching, but ignore normal characters beyond the max length
      // The command will be injected when EOL is reached
    }
    else if (serial_char == '\\') {   // Handle escapes
      if ((c = MYSERIAL0.read()) >= 0 && !serial_comment_mode) // if we have one more character, copy it over
        serial_line_buffer[serial_count++] = (char)c;
      // otherwise do nothing
    }
    else { // it's not a newline, carriage return or escape char
      if (serial_char == ';') serial_comment_mode = true;
      if (!serial_comment_mode) serial_line_buffer[serial_count++] = serial_char;
    }
  } // queue has space, serial has data
}

/**
 * M50: Set the Wifi ssid
 */
inline void gcode_M50() {
  for (char *fn = parser.string_arg; *fn; ++fn) if (*fn == ' ') *fn = '\0';
  config.ssid(parser.string_arg);
  SERIAL_ECHO(config.ssid());
}

/**
 * M50: Set the Wifi password
 */
inline void gcode_M51() {
  for (char *fn = parser.string_arg; *fn; ++fn) if (*fn == ' ') *fn = '\0';
  config.password(parser.string_arg);
  SERIAL_ECHO(config.password());
}

/**
 * M52: Connect the wifi
 */
inline void gcode_M52() {
  if(wifiConnect(config.ssid(), config.password())) {
    startDAVServer();
  }
  else {
    SERIAL_ECHOLN("Connect fail, please set the wifi config and connect again");
  }
}

/**
 * M53: Check wifi status
 */
inline void gcode_M53() {
  if(WiFi.status() != WL_CONNECTED) {
    SERIAL_ECHOLN("Wifi not connected");
    SERIAL_ECHOLN("Please set the wifi ssid with M50 and password with M51 , and start connection with M52");
  }
  else {
    DBG_PRINTLN("");
    DBG_PRINT("Connected to "); SERIAL_ECHOLN(WiFi.SSID());
    DBG_PRINT("IP address: "); DBG_PRINTLN(WiFi.localIP());
    DBG_PRINT("RSSI: "); DBG_PRINTLN(WiFi.RSSI());
    DBG_PRINT("Mode: "); DBG_PRINTLN(WiFi.getPhyMode());
  }
}

/**
 * Process the parsed command and dispatch it to its handler
 */
void process_parsed_command() {
  //SERIAL_ECHOLNPAIR("command_letter:", parser.command_letter);
  //SERIAL_ECHOLNPAIR("codenum:", parser.codenum);

  // Handle a known G, M, or T
  switch (parser.command_letter) {
    case 'G': switch (parser.codenum) {
      default: parser.unknown_command_error();
    }
    break;

    case 'M': switch (parser.codenum) {
      case 50: gcode_M50(); break;
      case 51: gcode_M51(); break;
      case 52: gcode_M52(); break;
      case 53: gcode_M53(); break;
      default: parser.unknown_command_error();
    }
    break;

    default: parser.unknown_command_error();
  }

  SERIAL_ECHOLN("ok");
}

void process_next_command() {
  char * const current_command = command_queue[cmd_queue_index_r];
  current_command_sd_pos = command_sd_pos[cmd_queue_index_r];

  // Parse the next command in the queue
  parser.parse(current_command);
  process_parsed_command();
}

// ------------------------
void loop() {
// ------------------------
	if(millis() < spiBlockoutTime)
		blink();

	// do it only if there is a need to read FS
	if(wifiConnected) {
  	if(dav.isClientWaiting())	{
  		if(initFailed)
  			return dav.rejectClient(statusMessage);
  		
  		// has other master been using the bus in last few seconds
  		if(millis() < spiBlockoutTime)
  			return dav.rejectClient("Marlin is reading from SD card");
  		
  		// a client is waiting and FS is ready and other SPI master is not using the bus
  		takeBusControl();
  		dav.handleClient();
  		relenquishBusControl();
  	}
	}

	// Get the serial input
  if (commands_in_queue < BUFSIZE) get_serial_commands();

  if (commands_in_queue) {
    process_next_command();

    // The queue may be reset by a command handler or by code invoked by idle() within a handler
    if (commands_in_queue) {
      --commands_in_queue;
      if (++cmd_queue_index_r >= BUFSIZE) cmd_queue_index_r = 0;
    }
  }

  // 
  statusBlink();
}



// ------------------------
void takeBusControl()	{
// ------------------------
	weHaveBus = true;
	LED_ON;
	pinMode(MISO, SPECIAL);	
	pinMode(MOSI, SPECIAL);	
	pinMode(SCLK, SPECIAL);	
	pinMode(SD_CS, OUTPUT);
}



// ------------------------
void relenquishBusControl()	{
// ------------------------
	pinMode(MISO, INPUT);	
	pinMode(MOSI, INPUT);	
	pinMode(SCLK, INPUT);	
	pinMode(SD_CS, INPUT);
	LED_OFF;
	weHaveBus = false;
}




// ------------------------
void blink()	{
// ------------------------
	LED_ON; 
	delay(100); 
	LED_OFF; 
	delay(400);
}



// ------------------------
void errorBlink()	{
// ------------------------
	for(int i = 0; i < 100; i++)	{
		LED_ON; 
		delay(50); 
		LED_OFF; 
		delay(50);
	}
}

void statusBlink() {
  static unsigned long time = 0;
  if(millis() > time + 1000 ) {
    if(wifiConnected) {
      LED_ON; 
  		delay(50); 
  		LED_OFF; 
    }
    else {
      LED_OFF; 
    }
    time = millis();
  }
}
