#ifndef IR_H
#define IR_H

#include <Arduino.h>
#include "signal.h"

// Outcome of one READ capture attempt.
enum IrReadResult : uint8_t {
  IR_READ_DECODED   = 0,   // *out filled with a DECODED signal
  IR_READ_RAW       = 1,   // *out filled with a RAW signal          (Phase 5)
  IR_READ_NONE      = 2,   // listen window timed out, nothing arrived
  IR_READ_UNDECODED = 3,   // a frame arrived but protocol was UNKNOWN (raw: Phase 5)
  IR_READ_OVERFLOW  = 4,   // frame too long for RAW_BUFFER_LENGTH — not stored
};

// Initialize the IR receiver (D2) and sender (D3).
void ir_begin();

// READ: listen up to IR_LISTEN_TIMEOUT_MS for one clean IR frame. On a good
// decode, fills *out as DECODED and returns IR_READ_DECODED; otherwise leaves
// *out untouched and returns the reason. Prints a human-readable line either
// way.   [Phase 2 decoded; Phase 5 adds the raw fallback]
IrReadResult ir_receive(LearnedSignal *out);

// Pretty-print a learned signal (decoded / raw / empty) to Serial. Shared by
// the READ path and the serial `show` command so output stays consistent.
void ir_print_signal(const LearnedSignal *sig);

// SEND: transmit a previously learned signal (decoded or raw).
// Returns true on success.   [Phase 4 decoded, Phase 5 raw]
bool ir_send(const LearnedSignal *sig);

#endif // IR_H
