#ifndef SERIALCMD_H
#define SERIALCMD_H

#include <Arduino.h>

// Initialize the serial command interface.
void serialcmd_begin();

// Read any pending serial input and act on completed command lines.
// Call this every loop().   [Phase 1+: wire commands to real actions.]
void serialcmd_poll();

// Print the list of available commands.
void serialcmd_print_help();

#endif // SERIALCMD_H
