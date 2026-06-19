#include "indicators.h"
#include "config.h"

void indicators_begin() {
  pinMode(PIN_LED_SENDING, OUTPUT);
  pinMode(PIN_LED_ERROR,   OUTPUT);
  digitalWrite(PIN_LED_SENDING, LOW);
  digitalWrite(PIN_LED_ERROR,   LOW);
}

void indicator_sending(bool on) {
  digitalWrite(PIN_LED_SENDING, on ? HIGH : LOW);
}

void indicator_error(ErrorCode code) {
  // Basic version: blink the error LED `code` times. Phase 6 turns these
  // into clearly distinguishable patterns (one per ErrorCode).
  for (uint8_t i = 0; i < (uint8_t)code; i++) {
    digitalWrite(PIN_LED_ERROR, HIGH);
    delay(150);
    digitalWrite(PIN_LED_ERROR, LOW);
    delay(150);
  }
}
