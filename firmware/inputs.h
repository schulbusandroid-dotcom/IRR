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
// otherwise BTN_NONE. Call every loop(). One physical press = one event;
// holding a button does not repeat (millis() debounce + falling edge).
Button inputs_poll();

// Read the 6 address switches into a value 0 .. NUM_SLOTS-1.
uint8_t inputs_read_address();

// Poll the switch address for changes. When the address has changed and
// settled (debounced), stores it in *addr and returns true; otherwise
// returns false. Lets loop() print only on a real, stable change.
bool inputs_poll_address(uint8_t *addr);

#endif // INPUTS_H
