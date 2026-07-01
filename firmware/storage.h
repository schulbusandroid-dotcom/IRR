#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "signal.h"

// The storage INTERFACE. The rest of the code talks only to these
// functions, never to the EEPROM directly — so the backend can later be
// swapped for an external chip (Phase 7) without touching anything else.

// Initialize storage; format on first run (magic/version check).
void storage_begin();

// Save `sig` to slot `addr` (0..NUM_SLOTS-1). Handles DECODED (7 bytes) and
// RAW (rawLen tick bytes) signals. Returns false if the slot is invalid, the
// signal is empty, or the data heap is full ("memory full").
bool storage_write(uint8_t addr, const LearnedSignal *sig);

// Load slot `addr` into *out. Returns false if the slot is empty/invalid.
bool storage_read(uint8_t addr, LearnedSignal *out);

// Is slot `addr` currently holding a signal?
bool storage_is_used(uint8_t addr);

// Free a single slot.
void storage_clear(uint8_t addr);

// Wipe all storage (re-format).
void storage_format();

// Bytes of data heap still free (for the `mem` command / diagnostics).
uint16_t storage_free_bytes();

#endif // STORAGE_H
