#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>

// Which button fired (debounced, one event per physical press).
enum Button : uint8_t {
  BTN_NONE  = 0,
  BTN_READ  = 1,
  BTN_STORE = 2,
  BTN_SEND  = 3,
};

// Configure pin modes for the buttons and switches (INPUT_PULLUP).
void inputs_begin();

// Poll the buttons. Returns a Button on a fresh debounced press,
// otherwise BTN_NONE.
//   [Phase 1: implement millis() debounce + falling-edge detection.]
Button inputs_poll();

// Read the 6 address switches into a value 0 .. NUM_SLOTS-1.
uint8_t inputs_read_address();

#endif // INPUTS_H
