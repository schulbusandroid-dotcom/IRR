#ifndef IR_H
#define IR_H

#include <Arduino.h>
#include "signal.h"

// Outcome of one READ capture attempt.
enum IrReadResult : uint8_t {
  IR_READ_DECODED   = 0,   // *out filled with a DECODED signal
  IR_READ_RAW       = 1,   // *out filled with a RAW signal (unknown protocol)
  IR_READ_NONE      = 2,   // listen window timed out, nothing arrived
  IR_READ_UNDECODED = 3,   // an unknown frame arrived but had no usable timings
  IR_READ_OVERFLOW  = 4,   // frame too long for RAW_BUFFER_LENGTH — not stored
};

// Initialize the IR receiver (D2) and sender (D3).
void ir_begin();

// READ: listen up to IR_LISTEN_TIMEOUT_MS for one clean IR frame. A known
// protocol fills *out as DECODED (IR_READ_DECODED); an unknown one is captured
// as RAW timings (IR_READ_RAW). On any failure (none/undecoded/overflow) *out
// is left untouched. Prints a human-readable line either way.
IrReadResult ir_receive(LearnedSignal *out);

// Pretty-print a learned signal (decoded / raw / empty) to Serial. Shared by
// the READ path and the serial `show` command so output stays consistent.
void ir_print_signal(const LearnedSignal *sig);

// SEND: transmit a previously learned signal out of the IR LED (D3).
// Decoded signals are re-encoded via the library (Phase 4); raw replay is
// Phase 5. Returns true if a frame was actually transmitted.
bool ir_send(const LearnedSignal *sig);

#endif // IR_H
