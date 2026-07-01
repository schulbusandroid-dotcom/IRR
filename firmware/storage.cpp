#include "storage.h"
#include "config.h"
#include <EEPROM.h>

// =====================================================================
//  Phase 3/5: onboard-EEPROM backend for DECODED and RAW signals.
//
//  On-EEPROM layout (see project_plan §3.4 and config.h):
//    [0]        MAGIC     — detects first run / a different format
//    [1]        VERSION   — bump when the byte layout changes
//    [2 .. 257] DIRECTORY — NUM_SLOTS entries × EEPROM_DIR_ENTRY_SZ bytes:
//                 byte0: flags  (bit7 = used, bit6 = raw)
//                 byte1: payload length in bytes
//                 byte2..3: payload start (absolute EEPROM address, LE)
//    [258 .. end] DATA HEAP — payloads, bump-allocated as slots are stored.
//
//  A DECODED payload is EEPROM_DECODED_BYTES (7) bytes:
//    [0] protocol  [1..2] address(LE)  [3..4] command(LE)
//    [5] numberOfBits  [6] flags
//  A RAW payload is exactly `rawLen` bytes — one 50 us tick each (see
//  signal.h). The directory length field carries the count, so raw needs no
//  in-heap header. Raw uses far more room than decoded (a TV frame ~30-70
//  ticks; a "fancy" remote more), so only a few raw slots fit the 766-byte
//  heap before storage_write() returns false ("memory full").
//
//  Allocation strategy:
//    * Re-storing a slot with a SAME-length payload overwrites in place, so
//      re-recording a decoded signal (always 7 bytes) never grows the heap.
//    * Otherwise the payload is appended at the heap high-water mark. The
//      high-water mark is recomputed from the directory (no extra persistent
//      state to keep in sync), so clearing the top allocation reclaims it.
//    * A used slot re-stored at a DIFFERENT length orphans its old bytes until
//      the next format(). Fixed 7-byte decoded records never trigger this;
//      variable-length raw records can, which is the documented fragmentation
//      trade-off (project_plan §3.4 / §7). Heap compaction stays a future item.
//
//  Raw was added without bumping VERSION: it only sets the existing RAW flag
//  and uses the existing length field, so EEPROMs written by Phase 3/4
//  (decoded only) remain valid and keep their stored signals.
//
//  All byte writes go through EEPROM.update(), which skips unchanged bytes to
//  protect the ~100k-write endurance (project_plan §6).
// =====================================================================

// First heap address = right after the header + directory.
static const uint16_t kHeapStart =
    EEPROM_HEADER_BYTES + (uint16_t)NUM_SLOTS * EEPROM_DIR_ENTRY_SZ;

// Total EEPROM size (1024 on the ATmega328P) — asked at runtime so the same
// code works on chips with a different EEPROM size.
static inline uint16_t eeprom_size() { return (uint16_t)EEPROM.length(); }

// ---- Directory access ------------------------------------------------

struct DirEntry {
  uint8_t  flags;
  uint8_t  length;
  uint16_t offset;   // absolute EEPROM address of the payload
};

static inline uint16_t dir_entry_addr(uint8_t slot) {
  return EEPROM_HEADER_BYTES + (uint16_t)slot * EEPROM_DIR_ENTRY_SZ;
}

static void dir_read(uint8_t slot, DirEntry *e) {
  uint16_t a = dir_entry_addr(slot);
  e->flags  = EEPROM.read(a);
  e->length = EEPROM.read(a + 1);
  e->offset = (uint16_t)EEPROM.read(a + 2)
            | ((uint16_t)EEPROM.read(a + 3) << 8);
}

static void dir_write(uint8_t slot, const DirEntry *e) {
  uint16_t a = dir_entry_addr(slot);
  EEPROM.update(a,     e->flags);
  EEPROM.update(a + 1, e->length);
  EEPROM.update(a + 2, (uint8_t)(e->offset & 0xFF));
  EEPROM.update(a + 3, (uint8_t)(e->offset >> 8));
}

// Lowest free heap address: one past the highest-ending used allocation.
static uint16_t heap_next_offset() {
  uint16_t top = kHeapStart;
  for (uint8_t s = 0; s < NUM_SLOTS; s++) {
    DirEntry e;
    dir_read(s, &e);
    if (e.flags & EEPROM_DIR_FLAG_USED) {
      uint16_t end = e.offset + e.length;
      if (end > top) top = end;
    }
  }
  return top;
}

// ---- DECODED payload (de)serialization -------------------------------

static void decoded_to_bytes(const LearnedSignal *s, uint8_t *b) {
  b[0] = s->protocol;
  b[1] = (uint8_t)(s->address & 0xFF);
  b[2] = (uint8_t)(s->address >> 8);
  b[3] = (uint8_t)(s->command & 0xFF);
  b[4] = (uint8_t)(s->command >> 8);
  b[5] = s->numberOfBits;
  b[6] = s->flags;
}

static void bytes_to_decoded(const uint8_t *b, LearnedSignal *s) {
  signal_clear(s);
  s->type         = SIGNAL_DECODED;
  s->protocol     = b[0];
  s->address      = (uint16_t)b[1] | ((uint16_t)b[2] << 8);
  s->command      = (uint16_t)b[3] | ((uint16_t)b[4] << 8);
  s->numberOfBits = b[5];
  s->flags        = b[6];
}

// ---- Public interface ------------------------------------------------

void storage_begin() {
  // Re-format if the EEPROM is unformatted or written by a different version.
  if (EEPROM.read(0) != EEPROM_MAGIC ||
      EEPROM.read(1) != EEPROM_FORMAT_VER) {
    storage_format();
  }
}

void storage_format() {
  // Clear every directory entry first, then stamp the header LAST — so a power
  // loss mid-format leaves the magic unset and we simply re-format next boot.
  DirEntry empty = { 0, 0, 0 };
  for (uint8_t s = 0; s < NUM_SLOTS; s++) {
    dir_write(s, &empty);
  }
  EEPROM.update(1, EEPROM_FORMAT_VER);
  EEPROM.update(0, EEPROM_MAGIC);
}

bool storage_write(uint8_t addr, const LearnedSignal *sig) {
  if (addr >= NUM_SLOTS) return false;

  // Serialize into a common (payload, len, flags) shape. DECODED is a fixed
  // 7-byte record; RAW is exactly rawLen tick bytes straight from sig->raw
  // (the directory length field records the count — no separate length byte).
  uint8_t        decoded_buf[EEPROM_DECODED_BYTES];
  const uint8_t *payload;
  uint8_t        len;
  uint8_t        type_flag;

  if (sig->type == SIGNAL_DECODED) {
    decoded_to_bytes(sig, decoded_buf);
    payload   = decoded_buf;
    len       = EEPROM_DECODED_BYTES;
    type_flag = 0;
  } else if (sig->type == SIGNAL_RAW) {
    if (sig->rawLen == 0) return false;     // nothing captured
    payload   = sig->raw;
    len       = sig->rawLen;                // 1 tick byte per entry
    type_flag = EEPROM_DIR_FLAG_RAW;
  } else {
    return false;                           // SIGNAL_EMPTY
  }

  DirEntry e;
  dir_read(addr, &e);

  uint16_t offset;
  if ((e.flags & EEPROM_DIR_FLAG_USED) && e.length == len) {
    offset = e.offset;                 // same-size overwrite: reuse in place
  } else {
    offset = heap_next_offset();       // append at the high-water mark
    if ((uint32_t)offset + len > eeprom_size()) {
      return false;                    // heap full — nothing written
    }
  }

  for (uint8_t i = 0; i < len; i++) {
    EEPROM.update(offset + i, payload[i]);
  }

  e.flags  = EEPROM_DIR_FLAG_USED | type_flag;
  e.length = len;
  e.offset = offset;
  dir_write(addr, &e);
  return true;
}

bool storage_read(uint8_t addr, LearnedSignal *out) {
  if (addr >= NUM_SLOTS) return false;

  DirEntry e;
  dir_read(addr, &e);
  if (!(e.flags & EEPROM_DIR_FLAG_USED)) return false;

  if (e.flags & EEPROM_DIR_FLAG_RAW) {
    // Raw payload = e.length tick bytes. Guard against a corrupt length that
    // would overrun the fixed raw[] buffer.
    if (e.length == 0 || e.length > RAW_MAX_TIMINGS) return false;
    signal_clear(out);
    out->type   = SIGNAL_RAW;
    out->rawLen = e.length;
    for (uint8_t i = 0; i < e.length; i++) {
      out->raw[i] = EEPROM.read(e.offset + i);
    }
    return true;
  }

  if (e.length != EEPROM_DECODED_BYTES) return false;    // unexpected size

  uint8_t payload[EEPROM_DECODED_BYTES];
  for (uint8_t i = 0; i < EEPROM_DECODED_BYTES; i++) {
    payload[i] = EEPROM.read(e.offset + i);
  }
  bytes_to_decoded(payload, out);
  return true;
}

bool storage_is_used(uint8_t addr) {
  if (addr >= NUM_SLOTS) return false;
  DirEntry e;
  dir_read(addr, &e);
  return (e.flags & EEPROM_DIR_FLAG_USED) != 0;
}

void storage_clear(uint8_t addr) {
  if (addr >= NUM_SLOTS) return;
  DirEntry empty = { 0, 0, 0 };
  dir_write(addr, &empty);
  // The payload bytes are left in place; they're only reachable through a
  // used directory entry. If this was the top allocation, heap_next_offset()
  // now returns a lower value, so the space is reclaimed automatically.
}

uint16_t storage_free_bytes() {
  // Contiguous space above the high-water mark. (Orphaned holes from
  // differing-size re-stores are reclaimed only by format(); see the header
  // note. For decoded-only Phase 3 there are no holes, so this is exact.)
  return eeprom_size() - heap_next_offset();
}
