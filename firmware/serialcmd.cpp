#include "serialcmd.h"
#include "config.h"
#include "inputs.h"
#include "signal.h"
#include "ir.h"
#include "storage.h"

// The one "current" signal in RAM, defined in firmware.ino. The serial
// `read`/`show`/`store`/`send` commands drive/inspect it and the stored
// slots, mirroring the buttons.
extern LearnedSignal last_received_data;

// Simple line buffer for the serial command parser.
static char    s_line[40];
static uint8_t s_len = 0;

void serialcmd_begin() {
  s_len = 0;
}

void serialcmd_print_help() {
  Serial.println(F("Commands ([Phase N] = not wired up yet):"));
  Serial.println(F("  help          - this list"));
  Serial.println(F("  switches      - show current address from the 6 switches"));
  Serial.println(F("  (buttons)     - press READ/STORE/SEND; events print here"));
  Serial.println(F("  read          - learn an IR signal (listens ~5s)"));
  Serial.println(F("  store <addr>  - save last learned signal to a slot 0..63"));
  Serial.println(F("  send <addr>   - transmit the signal stored in a slot"));
  Serial.println(F("  show          - show last learned signal"));
  Serial.println(F("  show <addr>   - show the signal stored in a slot"));
  Serial.println(F("  dump          - list all used slots"));
  Serial.println(F("  clear <addr>  - free a slot"));
  Serial.println(F("  format        - wipe all storage"));
  Serial.println(F("  mem           - free storage bytes + slots in use"));
  Serial.println(F("  raw on|off    - [Phase 6] verbose raw-timing logging"));
}

// Parse a decimal slot address after a command word. Accepts leading/trailing
// spaces, rejects trailing junk, and enforces the 0..NUM_SLOTS-1 range.
static bool parse_addr(const char *arg, uint8_t *out) {
  while (*arg == ' ') arg++;
  if (*arg < '0' || *arg > '9') return false;
  uint16_t v = 0;
  while (*arg >= '0' && *arg <= '9') {
    v = (uint16_t)(v * 10 + (*arg - '0'));
    if (v >= NUM_SLOTS) return false;      // out of range (also guards overflow)
    arg++;
  }
  while (*arg == ' ') arg++;
  if (*arg != '\0') return false;          // trailing junk after the number
  *out = (uint8_t)v;
  return true;
}

// ---- command handlers ------------------------------------------------

static void cmd_store(uint8_t addr) {
  if (last_received_data.type == SIGNAL_EMPTY) {
    Serial.println(F("store: nothing learned yet - use 'read' first"));
    return;
  }
  if (storage_write(addr, &last_received_data)) {
    Serial.print(F("stored to slot "));
    Serial.print(addr);
    Serial.print(F(" ("));
    Serial.print(storage_free_bytes());
    Serial.println(F(" free bytes)"));
  } else {
    Serial.println(F("store: FAILED (memory full)"));
  }
}

static void cmd_send(uint8_t addr) {
  LearnedSignal s;
  if (!storage_read(addr, &s)) {
    Serial.print(F("send: slot "));
    Serial.print(addr);
    Serial.println(F(" is empty"));
    return;
  }
  // ir_send() prints what it transmitted (or why it couldn't). Like the other
  // serial commands, this test path doesn't drive the indicator LEDs — that's
  // the button handler's job in firmware.ino.
  if (!ir_send(&s)) {
    Serial.println(F("send: FAILED (raw replay is Phase 5, or protocol not transmittable)"));
  }
}

static void cmd_show_slot(uint8_t addr) {
  LearnedSignal s;
  if (storage_read(addr, &s)) {
    Serial.print(F("slot "));
    Serial.print(addr);
    Serial.println(F(":"));
    ir_print_signal(&s);
  } else {
    Serial.print(F("slot "));
    Serial.print(addr);
    Serial.println(F(" is empty"));
  }
}

static void cmd_dump() {
  uint8_t used = 0;
  for (uint8_t a = 0; a < NUM_SLOTS; a++) {
    if (!storage_is_used(a)) continue;
    used++;
    LearnedSignal s;
    Serial.print(F("  slot "));
    Serial.print(a);
    Serial.print(F(":"));
    if (storage_read(a, &s)) {
      ir_print_signal(&s);          // prints its own leading spaces + newline
    } else {
      Serial.println(F("  (unreadable)"));
    }
  }
  Serial.print(F("used slots: "));
  Serial.print(used);
  Serial.print(F(" / "));
  Serial.print(NUM_SLOTS);
  Serial.print(F("   free bytes: "));
  Serial.println(storage_free_bytes());
}

static void cmd_clear(uint8_t addr) {
  if (storage_is_used(addr)) {
    storage_clear(addr);
    Serial.print(F("cleared slot "));
    Serial.println(addr);
  } else {
    Serial.print(F("slot "));
    Serial.print(addr);
    Serial.println(F(" was already empty"));
  }
}

static void cmd_mem() {
  uint8_t used = 0;
  for (uint8_t a = 0; a < NUM_SLOTS; a++) {
    if (storage_is_used(a)) used++;
  }
  Serial.print(F("free bytes: "));
  Serial.print(storage_free_bytes());
  Serial.print(F("   used slots: "));
  Serial.print(used);
  Serial.print(F(" / "));
  Serial.println(NUM_SLOTS);
}

// ---- line dispatch ---------------------------------------------------

static void handle_line(char *line) {
  while (*line == ' ') line++;        // skip leading spaces
  if (*line == '\0') return;          // ignore blank lines

  uint8_t addr;

  if (strcmp(line, "help") == 0) {
    serialcmd_print_help();
  } else if (strcmp(line, "switches") == 0) {
    Serial.print(F("address = "));
    Serial.println(inputs_read_address());
  } else if (strcmp(line, "read") == 0) {
    ir_receive(&last_received_data);        // prints its own outcome
  } else if (strcmp(line, "show") == 0) {
    Serial.println(F("last_received_data:"));
    ir_print_signal(&last_received_data);
  } else if (strncmp(line, "show ", 5) == 0) {
    if (parse_addr(line + 5, &addr)) cmd_show_slot(addr);
    else Serial.println(F("usage: show <0..63>"));
  } else if (strncmp(line, "store ", 6) == 0) {
    if (parse_addr(line + 6, &addr)) cmd_store(addr);
    else Serial.println(F("usage: store <0..63>"));
  } else if (strncmp(line, "send ", 5) == 0) {
    if (parse_addr(line + 5, &addr)) cmd_send(addr);
    else Serial.println(F("usage: send <0..63>"));
  } else if (strcmp(line, "dump") == 0) {
    cmd_dump();
  } else if (strncmp(line, "clear ", 6) == 0) {
    if (parse_addr(line + 6, &addr)) cmd_clear(addr);
    else Serial.println(F("usage: clear <0..63>"));
  } else if (strcmp(line, "format") == 0) {
    storage_format();
    Serial.print(F("storage formatted - all slots cleared ("));
    Serial.print(storage_free_bytes());
    Serial.println(F(" free bytes)"));
  } else if (strcmp(line, "mem") == 0) {
    cmd_mem();
  } else {
    // Not recognized (includes Phase 6 commands like `raw on|off`).
    Serial.print(F("Not wired up yet: "));
    Serial.println(line);
    Serial.println(F("Type 'help' for the command list and phase tags."));
  }
}

void serialcmd_poll() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;          // ignore CR (Windows line endings)
    if (c == '\n') {
      s_line[s_len] = '\0';
      handle_line(s_line);
      s_len = 0;
    } else if (s_len < sizeof(s_line) - 1) {
      s_line[s_len++] = c;
    } else {
      s_len = 0;                      // line too long: reset buffer
    }
  }
}
