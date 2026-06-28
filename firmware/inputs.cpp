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

// The three momentary buttons, in a fixed order so we can keep per-button
// debounce state in small parallel arrays.
static const uint8_t kButtonPins[3] = { PIN_BTN_READ, PIN_BTN_STORE, PIN_BTN_SEND };
static const Button  kButtonIds [3] = { BTN_READ,     BTN_STORE,     BTN_SEND     };

Button inputs_poll() {
  // Per-button debounce state (kept between calls).
  static bool     lastRaw[3];        // last raw reading (true = pressed)
  static bool     stable[3];         // debounced state (true = pressed)
  static uint32_t changedAt[3];      // millis() of the last raw change
  static bool     initialized = false;

  uint32_t now = millis();

  // First call: latch the current levels so a button already held at boot
  // is treated as the resting state (no spurious press event).
  if (!initialized) {
    for (uint8_t i = 0; i < 3; i++) {
      bool pressed = (digitalRead(kButtonPins[i]) == BTN_PRESSED_LEVEL);
      lastRaw[i]   = pressed;
      stable[i]    = pressed;
      changedAt[i] = now;
    }
    initialized = true;
    return BTN_NONE;
  }

  Button event = BTN_NONE;
  for (uint8_t i = 0; i < 3; i++) {
    bool pressed = (digitalRead(kButtonPins[i]) == BTN_PRESSED_LEVEL);

    // Restart the debounce timer whenever the raw reading flips (chatter).
    if (pressed != lastRaw[i]) {
      lastRaw[i]   = pressed;
      changedAt[i] = now;
    }

    // A reading that has held steady past DEBOUNCE_MS is accepted.
    if ((now - changedAt[i]) >= DEBOUNCE_MS && pressed != stable[i]) {
      if (!pressed) {
        stable[i] = false;                 // debounced release: no event
      } else if (event == BTN_NONE) {
        stable[i] = true;                  // debounced press: emit once
        event = kButtonIds[i];
      }
      // If another button already fired this poll, leave this one's stable
      // state unchanged so its press is reported on the next poll instead.
    }
  }
  return event;
}

bool inputs_poll_address(uint8_t *addr) {
  // Debounce the 6-switch address the same way: only report once it has
  // settled and actually differs from the value we last reported.
  static bool     initialized = false;
  static uint8_t  reported;            // last value handed back to the caller
  static uint8_t  candidate;           // value currently settling
  static uint32_t changedAt;

  uint32_t now = millis();
  uint8_t  a   = inputs_read_address();

  if (!initialized) {
    reported    = a;
    candidate   = a;
    changedAt   = now;
    initialized = true;
    return false;
  }

  if (a != candidate) {
    candidate = a;
    changedAt = now;
  }

  if ((now - changedAt) >= DEBOUNCE_MS && candidate != reported) {
    reported = candidate;
    *addr    = reported;
    return true;
  }
  return false;
}
