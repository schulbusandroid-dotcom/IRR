#ifndef INDICATORS_H
#define INDICATORS_H

#include <Arduino.h>

// Error codes -> distinct error-LED blink patterns (refined in Phase 6).
enum ErrorCode : uint8_t {
  ERR_NONE       = 0,
  ERR_NO_SIGNAL  = 1,   // READ found nothing usable
  ERR_EMPTY_SLOT = 2,   // SEND on an empty slot
  ERR_MEM_FULL   = 3,   // STORE but the heap is full
  ERR_OVERFLOW   = 4,   // captured frame too long for the buffer
};

void indicators_begin();

// Turn the "sending" LED on/off.
void indicator_sending(bool on);

// Flash the error LED to report `code` (blocking, brief).
void indicator_error(ErrorCode code);

#endif // INDICATORS_H
