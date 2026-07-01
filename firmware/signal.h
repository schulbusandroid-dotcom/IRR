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
  // Timings are stored as IRremote "ticks" of MICROS_PER_TICK (50 us) each,
  // one byte per mark/space. That is the receiver's native resolution and
  // halves the RAM/EEPROM footprint vs. raw microseconds — important on a
  // 2 KB / 1 KB chip. capture: compensateAndStoreIRResultInArray(); replay:
  // the uint8_t IrSender.sendRaw() overload (see ir.cpp).
  uint8_t  rawLen;                  // number of timing entries actually used
  uint8_t  raw[RAW_MAX_TIMINGS];    // durations as 50 us ticks (0..255)
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
