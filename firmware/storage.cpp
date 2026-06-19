#include "storage.h"
#include "config.h"
// #include <EEPROM.h>   // enabled in Phase 3

// =====================================================================
//  Phase 0: STUBS only. EEPROM layout (v1 proposal) is in config.h and
//  project_plan §3.4:  [magic][version][64-entry directory][data heap].
//
//  Phase 3 implements decoded read/write + first-run format.
//  Phase 5 adds variable-length raw payloads + "memory full" handling.
// =====================================================================

void storage_begin() {
  // TODO (Phase 3): read magic/version; storage_format() if unformatted.
}

bool storage_write(uint8_t addr, const LearnedSignal *sig) {
  (void)addr; (void)sig;
  return false;   // TODO (Phase 3 / 5)
}

bool storage_read(uint8_t addr, LearnedSignal *out) {
  (void)addr; (void)out;
  return false;   // TODO (Phase 3 / 5)
}

bool storage_is_used(uint8_t addr) {
  (void)addr;
  return false;   // TODO (Phase 3)
}

void storage_clear(uint8_t addr) {
  (void)addr;
  // TODO (Phase 3)
}

void storage_format() {
  // TODO (Phase 3): write magic/version, clear the directory.
}

uint16_t storage_free_bytes() {
  return 0;       // TODO (Phase 3)
}
