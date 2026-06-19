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

  print_banner();
}

void loop() {
  // Serial test/debug interface (works now: 'help', 'switches').
  serialcmd_poll();

  // ---- Button state machine (skeleton) ----
  // inputs_poll() is a Phase 0 stub returning BTN_NONE, so these handlers
  // do not run yet. Phases 1-5 bring them to life.
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
  // TODO (Phase 2/5):
  //   if (ir_receive(&last_received_data)) { feedback + serial report }
  //   else { indicator_error(ERR_NO_SIGNAL); }
}

// STORE button -> save last_received_data at the switch address.
static void handle_store() {
  // TODO (Phase 3/5):
  //   uint8_t addr = inputs_read_address();
  //   if (last_received_data.type == SIGNAL_EMPTY) error;
  //   else if (!storage_write(addr, &last_received_data)) error(ERR_MEM_FULL);
}

// SEND button -> transmit the signal stored at the switch address.
static void handle_send() {
  // TODO (Phase 4/5):
  //   uint8_t addr = inputs_read_address();
  //   LearnedSignal s;
  //   if (!storage_read(addr, &s)) error(ERR_EMPTY_SLOT);
  //   else { indicator_sending(true); ir_send(&s); indicator_sending(false); }
}
