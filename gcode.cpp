#include "gcode.h"
#include "config.h"
#include "parser.h"
#include "network.h"
#include "serial.h"
#include <ESP8266WiFi.h>

Gcode gcode;

void Gcode::Handle() {
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
}

/**
 * Once a new command is in the ring buffer, call this to commit it
 */
void Gcode::_commit_command(bool say_ok) {
  send_ok[cmd_queue_index_w] = say_ok;
  if (++cmd_queue_index_w >= BUFSIZE) cmd_queue_index_w = 0;
  commands_in_queue++;
}

/**
 * Copy a command from RAM into the main command buffer.
 * Return true if the command was successfully added.
 * Return false for a full buffer, or if the 'command' is a comment.
 */
bool Gcode::_enqueuecommand(const char* cmd, bool say_ok) {
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
void Gcode::get_serial_commands() {
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
void Gcode::gcode_M50() {
  for (char *fn = parser.string_arg; *fn; ++fn) if (*fn == ' ') *fn = '\0';
  config.ssid(parser.string_arg);
  SERIAL_ECHO("ssid:");
  SERIAL_ECHOLN(config.ssid());
}

/**
 * M50: Set the Wifi password
 */
void Gcode::gcode_M51() {
  for (char *fn = parser.string_arg; *fn; ++fn) if (*fn == ' ') *fn = '\0';
  config.password(parser.string_arg);
  SERIAL_ECHO("password:");
  SERIAL_ECHOLN(config.password());
}

/**
 * M52: Connect the wifi
 */
void Gcode::gcode_M52() {
  if(!network.start()) {
    SERIAL_ECHOLN("Connect fail, please check your INI file or set the wifi config and connect again");
    SERIAL_ECHOLN("- M50: Set the wifi ssid , 'M50 ssid-name'");
    SERIAL_ECHOLN("- M51: Set the wifi password , 'M51 password'");
    SERIAL_ECHOLN("- M52: Start to connect the wifi");
    SERIAL_ECHOLN("- M53: Check the connection status");
  }
}

/**
 * M53: Check wifi status
 */
void Gcode::gcode_M53() {
  if(WiFi.status() != WL_CONNECTED) {
    SERIAL_ECHOLN("Wifi not connected");
    SERIAL_ECHOLN("- M50: Set the wifi ssid , 'M50 ssid-name'");
    SERIAL_ECHOLN("- M51: Set the wifi password , 'M51 password'");
    SERIAL_ECHOLN("- M52: Start to connect the wifi");
    SERIAL_ECHOLN("- M53: Check the connection status");
  }
  else {
    SERIAL_ECHOLN("");
    SERIAL_ECHO("Connected to "); SERIAL_ECHOLN(WiFi.SSID());
    SERIAL_ECHO("IP address: "); SERIAL_ECHOLN(WiFi.localIP());
    SERIAL_ECHO("RSSI: "); SERIAL_ECHOLN(WiFi.RSSI());
    SERIAL_ECHO("Mode: "); SERIAL_ECHOLN(WiFi.getPhyMode());
    SERIAL_ECHO("Asscess to SD at the Run prompt : \\\\"); SERIAL_ECHO(WiFi.localIP());SERIAL_ECHOLN("\\DavWWWRoot");
  }
}

/**
 * Process the parsed command and dispatch it to its handler
 */
void Gcode::process_parsed_command() {
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

void Gcode::process_next_command() {
  char * const current_command = command_queue[cmd_queue_index_r];
  current_command_sd_pos = command_sd_pos[cmd_queue_index_r];

  // Parse the next command in the queue
  parser.parse(current_command);
  process_parsed_command();
}
