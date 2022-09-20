/**
 * Base on Malrin https://github.com/MarlinFirmware/Marlin
 * parser.cpp - Parser for a GCode line, providing a parameter interface.
 */
#include "serial.h"
#include "parser.h"

// Must be declared for allocation and to satisfy the linker
// Zero values need no initialization.
char *GCodeParser::command_ptr,
     *GCodeParser::string_arg,
     *GCodeParser::value_ptr;
char GCodeParser::command_letter;
int GCodeParser::codenum;

char *GCodeParser::command_args; // start of parameters

// Create a global instance of the GCode parser singleton
GCodeParser parser;

/**
 * Clear all code-seen (and value pointers)
 *
 * Since each param is set/cleared on seen codes,
 * this may be optimized by commenting out ZERO(param)
 */
void GCodeParser::reset() {
  string_arg = NULL;                    // No whole line argument
  command_letter = '?';                 // No command letter
  codenum = 0;                          // No command code
}

// Populate all fields by parsing a single line of GCode
// 58 bytes of SRAM are used to speed up seen/value
void GCodeParser::parse(char *p) {

  reset(); // No codes to report

  // Skip spaces
  while (*p == ' ') ++p;

  // Skip N[-0-9] if included in the command line
  if (*p == 'N' && NUMERIC_SIGNED(p[1])) {
    p += 2;                  // skip N[-0-9]
    while (NUMERIC(*p)) ++p; // skip [0-9]*
    while (*p == ' ')   ++p; // skip [ ]*
  }

  // *p now points to the current command, which should be G, M, or T
  command_ptr = p;

  // Get the command letter, which must be G, M, or T
  const char letter = *p++;

  // Nullify asterisk and trailing whitespace
  char *starpos = strchr(p, '*');
  if (starpos) {
    --starpos;                          // *
    while (*starpos == ' ') --starpos;  // spaces...
    starpos[1] = '\0';
  }

  // Bail if the letter is not G, M, or T
  switch (letter) { case 'G': case 'M': case 'T': break; default: return; }

  // Skip spaces to get the numeric part
  while (*p == ' ') p++;

  // Bail if there's no command code number
  if (!NUMERIC(*p)) return;

  // Save the command letter at this point
  // A '?' signifies an unknown command
  command_letter = letter;

  // Get the code number - integer digits only
  codenum = 0;
  do {
    codenum *= 10, codenum += *p++ - '0';
  } while (NUMERIC(*p));

  // Skip all spaces to get to the first argument, or nul
  while (*p == ' ') p++;

  // The command parameters (if any) start here, for sure!

  // Only use string_arg for these M codes
  if (letter == 'M') switch (codenum) { case 50: case 51: case 23: case 28: case 30: case 117: case 118: case 928: string_arg = p; return; default: break; }

  #if defined(DEBUG_GCODE_PARSER)
    const bool debug = codenum == 800;
  #endif

  /**
   * Find all parameters, set flags and pointers for fast parsing
   *
   * Most codes ignore 'string_arg', but those that want a string will get the right pointer.
   * The following loop assigns the first "parameter" having no numeric value to 'string_arg'.
   * This allows M0/M1 with expire time to work: "M0 S5 You Win!"
   * For 'M118' you must use 'E1' and 'A1' rather than just 'E' or 'A'
   */
  string_arg = NULL;
  while (const char code = *p++) {                    // Get the next parameter. A NUL ends the loop

    // Special handling for M32 [P] !/path/to/file.g#
    // The path must be the last parameter
    if (code == '!' && letter == 'M' && codenum == 32) {
      string_arg = p;                           // Name starts after '!'
      char * const lb = strchr(p, '#');         // Already seen '#' as SD char (to pause buffering)
      if (lb) *lb = '\0';                       // Safe to mark the end of the filename
      return;
    }

    // Arguments MUST be uppercase for fast GCode parsing
    #define PARAM_TEST true

    if (PARAM_TEST) {

      while (*p == ' ') p++;                    // Skip spaces between parameters & values

      const bool has_num = valid_float(p);

      #if defined(DEBUG_GCODE_PARSER)
        if (debug) {
          SERIAL_ECHOPAIR("Got letter ", code);
          SERIAL_ECHOPAIR(" at index ", (int)(p - command_ptr - 1));
          if (has_num) SERIAL_ECHOPGM(" (has_num)");
        }
      #endif

      if (!has_num && !string_arg) {            // No value? First time, keep as string_arg
        string_arg = p - 1;
        #if defined(DEBUG_GCODE_PARSER)
          if (debug) SERIAL_ECHOPAIR(" string_arg: ", hex_address((void*)string_arg)); // DEBUG
        #endif
      }

      #if defined(DEBUG_GCODE_PARSER)
        if (debug) SERIAL_EOL();
      #endif
    }
    else if (!string_arg) {                     // Not A-Z? First time, keep as the string_arg
      string_arg = p - 1;
      #if defined(DEBUG_GCODE_PARSER)
        if (debug) SERIAL_ECHOPAIR(" string_arg: ", hex_address((void*)string_arg)); // DEBUG
      #endif
    }

    if (!WITHIN(*p, 'A', 'Z')) {                // Another parameter right away?
      while (*p && DECIMAL_SIGNED(*p)) p++;     // Skip over the value section of a parameter
      while (*p == ' ') p++;                    // Skip over all spaces
    }
  }
}

void GCodeParser::unknown_command_error() {
  SERIAL_ECHO_START();
  SERIAL_ECHOPAIR("Unknown:", command_ptr);
  SERIAL_CHAR('"');
  SERIAL_EOL();
}

#if defined(DEBUG_GCODE_PARSER)

  void GCodeParser::debug() {
    SERIAL_ECHOPAIR("Command: ", command_ptr);
    SERIAL_ECHOPAIR(" (", command_letter);
    SERIAL_ECHO(codenum);
    SERIAL_ECHOLNPGM(")");
    SERIAL_ECHOPAIR(" args: \"", command_args);
    SERIAL_CHAR('"');
    if (string_arg) {
      SERIAL_ECHOPGM(" string: \"");
      SERIAL_ECHO(string_arg);
      SERIAL_CHAR('"');
    }
    SERIAL_ECHOPGM("\n\n");
    for (char c = 'A'; c <= 'Z'; ++c) {
      if (seen(c)) {
        SERIAL_ECHOPAIR("Code '", c); SERIAL_ECHOPGM("':");
        if (has_value()) {
          SERIAL_ECHOPAIR("\n    float: ", value_float());
          SERIAL_ECHOPAIR("\n     long: ", value_long());
          SERIAL_ECHOPAIR("\n    ulong: ", value_ulong());
          SERIAL_ECHOPAIR("\n   millis: ", value_millis());
          SERIAL_ECHOPAIR("\n   sec-ms: ", value_millis_from_seconds());
          SERIAL_ECHOPAIR("\n      int: ", value_int());
          SERIAL_ECHOPAIR("\n   ushort: ", value_ushort());
          SERIAL_ECHOPAIR("\n     byte: ", (int)value_byte());
          SERIAL_ECHOPAIR("\n     bool: ", (int)value_bool());
          SERIAL_ECHOPAIR("\n   linear: ", value_linear_units());
        }
        else
          SERIAL_ECHOPGM(" (no value)");
        SERIAL_ECHOPGM("\n\n");
      }
    }
  }

#endif // DEBUG_GCODE_PARSER
