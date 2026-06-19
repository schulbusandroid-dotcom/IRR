#ifndef SIGNAL_H
#define SIGNAL_H

#include <Arduino.h>
#include "config.h"

// The kind of data held in a LearnedSignal.
enum SignalType : uint8_t {
  SIGNAL_EMPTY   = 0,   // nothing stored
  SIGNAL_DECODED = 1,   // a recognized protocol (tiny, human-readable)
  SIGNAL_RAW     = 2,   // raw on/off timings (universal, bigger)
};

// One learned IR signal, held in RAM. This is the type of the global
// `last_received_data`. Depending on `type`, EITHER the decoded fields OR
// the raw fields are meaningful (not both).
struct LearnedSignal {
  SignalType type;

  // ---- when type == SIGNAL_DECODED ----
  uint8_t  protocol;      // IRremote decode_type_t, stored as a byte
  uint16_t address;
  uint16_t command;
  uint8_t  numberOfBits;
  uint8_t  flags;

  // ---- when type == SIGNAL_RAW ----
  uint8_t  rawLen;                  // number of timing entries actually used
  uint16_t raw[RAW_MAX_TIMINGS];    // durations in microseconds
};

// Reset a signal back to empty.
inline void signal_clear(LearnedSignal *sig) {
  sig->type         = SIGNAL_EMPTY;
  sig->protocol     = 0;
  sig->address      = 0;
  sig->command      = 0;
  sig->numberOfBits = 0;
  sig->flags        = 0;
  sig->rawLen       = 0;
}

#endif // SIGNAL_H
