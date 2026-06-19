#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Optional I2C OLED (SSD1306 128x32) for basic status text.
//
// Enable by setting USE_OLED to 1 in config.h once the panel is wired
// (A4=SDA, A5=SCL) and a display library is installed. When USE_OLED is 0
// every function here compiles to a no-op, so the rest of the firmware can
// call them unconditionally without #ifdefs scattered around.

void display_begin();
void display_clear();

// Show the current address plus a short one-line status message.
void display_status(uint8_t address, const char *msg);

// Show a free-form message line.
void display_message(const char *msg);

#endif // DISPLAY_H
