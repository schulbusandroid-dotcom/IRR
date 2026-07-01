#include "serialcmd.h"
#include "config.h"
#include "inputs.h"
#include "signal.h"
#include "ir.h"

// The one "current" signal in RAM, defined in firmware.ino. The serial
// `read`/`show` commands drive/inspect it, mirroring the READ button.
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
  Serial.println(F("  store <addr>  - [Phase 3] save last signal to a slot"));
  Serial.println(F("  send <addr>   - [Phase 4] transmit a stored slot"));
  Serial.println(F("  show          - show last learned signal (slots: Phase 3)"));
  Serial.println(F("  dump          - [Phase 3] list all slots"));
  Serial.println(F("  clear <addr>  - [Phase 3] free a slot"));
  Serial.println(F("  format        - [Phase 3] wipe all storage"));
  Serial.println(F("  mem           - [Phase 3] free storage bytes"));
  Serial.println(F("  raw on|off    - [Phase 5] verbose raw logging"));
}

static void handle_line(char *line) {
  while (*line == ' ') line++;        // skip leading spaces
  if (*line == '\0') return;          // ignore blank lines

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
    Serial.println(F("show <addr> (stored slots) lands in Phase 3"));
  } else {
    // TODO (Phase 3+): parse store/send/dump/etc. and call the matching
    // module functions. For now, acknowledge politely.
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
