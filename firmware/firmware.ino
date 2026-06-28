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
static void handle_read() {
  // Phase 2/5 will replace this with: if (ir_receive(&last_received_data))
  // report the decoded/raw signal, else indicator_error(ERR_NO_SIGNAL).
  Serial.println(F("READ  (IR capture lands in Phase 2)"));
  indicator_pulse_sending(60);     // quick "press registered" blink on D13
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
  // Phase 3/5: storage_write(addr, &last_received_data); error on full heap.
  Serial.println(F("  -> (storage lands in Phase 3)"));
  indicator_pulse_sending(60);
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
