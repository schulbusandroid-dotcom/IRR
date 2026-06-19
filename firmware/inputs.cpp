#include "inputs.h"
#include "config.h"

// Switch pins ordered bit0..bit5 so the address is easy to assemble.
static const uint8_t kSwitchPins[ADDRESS_BITS] = {
  PIN_SW_BIT0, PIN_SW_BIT1, PIN_SW_BIT2,
  PIN_SW_BIT3, PIN_SW_BIT4, PIN_SW_BIT5,
};

void inputs_begin() {
  pinMode(PIN_BTN_READ,  INPUT_PULLUP);
  pinMode(PIN_BTN_STORE, INPUT_PULLUP);
  pinMode(PIN_BTN_SEND,  INPUT_PULLUP);
  for (uint8_t i = 0; i < ADDRESS_BITS; i++) {
    pinMode(kSwitchPins[i], INPUT_PULLUP);
  }
}

uint8_t inputs_read_address() {
  uint8_t addr = 0;
  for (uint8_t i = 0; i < ADDRESS_BITS; i++) {
    if (digitalRead(kSwitchPins[i]) == SWITCH_ON_LEVEL) {
      addr |= (uint8_t)(1 << i);
    }
  }
  return addr;
}

Button inputs_poll() {
  // TODO (Phase 1): millis()-based debounce + falling-edge detection on the
  // three buttons; return the matching Button exactly once per press.
  return BTN_NONE;
}
