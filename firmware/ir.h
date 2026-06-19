#ifndef IR_H
#define IR_H

#include <Arduino.h>
#include "signal.h"

// Initialize the IR receiver and sender.
void ir_begin();

// READ: pick up one IR frame and fill *out. Returns true if a usable
// signal was captured (decoded OR raw); false on no signal or a capture
// overflow.   [Phase 2 decoded, Phase 5 raw fallback + overflow]
bool ir_receive(LearnedSignal *out);

// SEND: transmit a previously learned signal (decoded or raw).
// Returns true on success.   [Phase 4 decoded, Phase 5 raw]
bool ir_send(const LearnedSignal *sig);

#endif // IR_H
