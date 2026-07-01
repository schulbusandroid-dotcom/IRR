// =====================================================================
//  IRR — IR Remote Recorder / Replayer
//
//  Phase 0 scaffolding: this sketch compiles, uploads, prints a banner,
//  and answers a minimal serial 'help' / 'switches'. All device logic is
//  stubbed (see each module's `TODO (Phase N)` notes) and gets filled in
//  phase by phase. Full plan: ../project_plan.md  Quick start: ../session_context.md
// =====================================================================

#include "config.h"
#include "signal.h"
#include "inputs.h"
#include "ir.h"
#include "storage.h"
#include "indicators.h"
#include "serialcmd.h"
#include "display.h"

// The single "current" signal in RAM — your `last_received_data`.
LearnedSignal last_received_data;

// ---- forward declarations -------------------------------------------
static void print_banner();
static void handle_read();
static void handle_store();
static void handle_send();

void setup() {
  Serial.begin(SERIAL_BAUD);

  signal_clear(&last_received_data);
  inputs_begin();
  indicators_begin();
  storage_begin();
  ir_begin();
  serialcmd_begin();
  display_begin();        // no-op unless USE_OLED is set (see config.h)

  print_banner();
  display_status(inputs_read_address(), "ready");
}

void loop() {
  // Serial test/debug interface (works now: 'help', 'switches').
  serialcmd_poll();

  // ---- Switch address: report changes as you flip the DIP switches ----
  uint8_t addr;
  if (inputs_poll_address(&addr)) {
    Serial.print(F("address = "));
    Serial.println(addr);
  }

  // ---- Button state machine ----
  // Phase 1: debounced presses are reported with serial + LED feedback.
  // The handlers already do the parts that are real today (an empty
  // last_received_data, an empty slot) and grow into IR/storage in 2-5.
  switch (inputs_poll()) {
    case BTN_READ:  handle_read();  break;
    case BTN_STORE: handle_store(); break;
    case BTN_SEND:  handle_send();  break;
    case BTN_NONE:
    default:        break;
  }
}

// ---------------------------------------------------------------------

static void print_banner() {
  Serial.println();
  Serial.println(F("==============================================="));
  Serial.print  (F("  "));  Serial.print(F(FIRMWARE_NAME));
  Serial.print  (F("  v")); Serial.println(F(FIRMWARE_VERSION));
  Serial.println(F("  IR Remote Recorder / Replayer"));
  Serial.println(F("==============================================="));
  Serial.print  (F("  build      : ")); Serial.print(F(__DATE__));
  Serial.print  (F(" "));               Serial.println(F(__TIME__));
  Serial.print  (F("  slots      : ")); Serial.println(NUM_SLOTS);
  Serial.print  (F("  switch addr: ")); Serial.println(inputs_read_address());
  Serial.println(F("  Type 'help' and press Enter."));
  Serial.println();
}

// READ button -> learn a signal into last_received_data.
// ir_receive() prints the details; here we pick the LED feedback. It only
// overwrites last_received_data on a successful decode, so a failed READ
// leaves whatever was already learned intact.
static void handle_read() {
  switch (ir_receive(&last_received_data)) {
    case IR_READ_DECODED:
    case IR_READ_RAW:              // (raw arrives in Phase 5)
      indicator_pulse_sending(60); // success blink on D13
      break;
    case IR_READ_OVERFLOW:
      indicator_error(ERR_OVERFLOW);
      break;
    case IR_READ_NONE:
    case IR_READ_UNDECODED:
    default:
      indicator_error(ERR_NO_SIGNAL);
      break;
  }
}

// STORE button -> save last_received_data at the switch address.
static void handle_store() {
  uint8_t addr = inputs_read_address();
  Serial.print(F("STORE addr="));
  Serial.print(addr);

  if (last_received_data.type == SIGNAL_EMPTY) {
    Serial.println(F("  -> nothing learned yet"));
    indicator_error(ERR_NO_SIGNAL);
    return;
  }
  // Persist the learned signal to EEPROM at the switch address. A decoded
  // signal always fits; storage_write only fails once the heap is full
  // (realistic for raw signals in Phase 5, not for decoded).
  if (storage_write(addr, &last_received_data)) {
    Serial.print(F("  -> stored ("));
    Serial.print(storage_free_bytes());
    Serial.println(F(" free bytes)"));
    indicator_pulse_sending(60);
  } else {
    Serial.println(F("  -> STORE failed (memory full)"));
    indicator_error(ERR_MEM_FULL);
  }
}

// SEND button -> transmit the signal stored at the switch address.
static void handle_send() {
  uint8_t addr = inputs_read_address();
  Serial.print(F("SEND  addr="));
  Serial.print(addr);

  LearnedSignal s;
  if (!storage_read(addr, &s)) {           // stub returns false -> empty slot
    Serial.println(F("  -> slot empty"));
    indicator_error(ERR_EMPTY_SLOT);
    return;
  }
  // Phase 4/5: indicator_sending(true); ir_send(&s); indicator_sending(false);
  Serial.println(F("  -> (IR send lands in Phase 4)"));
}
