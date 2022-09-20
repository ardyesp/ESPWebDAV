#ifndef _GCODE_H_
#define _GCODE_H_

#define MAX_CMD_SIZE 96
#define BUFSIZE 4

/**
 * GCode
 *
 *  - Handle gcode
 */
class Gcode {
public:
  void Handle();
  
private:
  void _commit_command(bool say_ok);
  bool _enqueuecommand(const char* cmd, bool say_ok=false);
  void get_serial_commands();
  void gcode_M50();
  void gcode_M51();
  void gcode_M52();
  void gcode_M53();
  void process_parsed_command();
  void process_next_command();
  
  /**
   * GCode Command Queue
   * A simple ring buffer of BUFSIZE command strings.
   *
   * Commands are copied into this buffer by the command injectors
   * (immediate, serial, sd card) and they are processed sequentially by
   * the main loop. The process_next_command function parses the next
   * command and hands off execution to individual handler functions.
   */
  unsigned char commands_in_queue = 0, // Count of commands in the queue
                cmd_queue_index_r = 0, // Ring buffer read (out) position
                cmd_queue_index_w = 0; // Ring buffer write (in) position

  unsigned long command_sd_pos[BUFSIZE];
  volatile unsigned long current_command_sd_pos;

  char command_queue[BUFSIZE][MAX_CMD_SIZE];

  bool send_ok[BUFSIZE];

  // Number of characters read in the current line of serial input
  int serial_count; // = 0;
};

extern Gcode gcode;

#endif
