// =====================================================================
//  IR receive/send via the IRremote library (pinned v4.7.1).
//
//  Include order matters: config.h defines RAW_BUFFER_LENGTH, which the
//  library reads, so it must come BEFORE <IRremote.hpp>. The .hpp carries
//  the implementation and must be included EXACTLY ONCE in the whole
//  project — this is that one place. See project_plan.md §3.6.
//
//  Phase 2 implements the DECODED receive path. RAW capture + overflow
//  storage is Phase 5; ir_send() is Phase 4.
// =====================================================================

#include "config.h"          // must define RAW_BUFFER_LENGTH first
#include <IRremote.hpp>      // include exactly once, here
#include "ir.h"

void ir_begin() {
  // LED feedback would blink the built-in LED (D13) on every receive, but
  // D13 is our "sending" indicator — keep its meaning ours, so disable it.
  IrReceiver.begin(PIN_IR_RECEIVE, DISABLE_LED_FEEDBACK);
  IrSender.begin(PIN_IR_SEND);   // readies the send pin for Phase 4; harmless now
}

void ir_print_signal(const LearnedSignal *sig) {
  if (sig->type == SIGNAL_DECODED) {
    Serial.print(F("  "));
    Serial.print(getProtocolString((decode_type_t)sig->protocol));
    Serial.print(F("  addr=0x")); Serial.print(sig->address, HEX);
    Serial.print(F(" cmd=0x"));   Serial.print(sig->command, HEX);
    Serial.print(F(" bits="));    Serial.println(sig->numberOfBits);
  } else if (sig->type == SIGNAL_RAW) {
    Serial.print(F("  raw, "));
    Serial.print(sig->rawLen);
    Serial.println(F(" timings"));
  } else {
    Serial.println(F("  (empty - nothing learned yet)"));
  }
}

IrReadResult ir_receive(LearnedSignal *out) {
  Serial.println(F("Listening for IR (aim a remote and press a button)..."));

  // Discard any frame that completed before this call, so we capture the
  // fresh press rather than something stale in the buffer.
  if (IrReceiver.decode()) IrReceiver.resume();

  uint32_t start = millis();
  while ((millis() - start) < IR_LISTEN_TIMEOUT_MS) {
    if (!IrReceiver.decode()) continue;

    IRData &d = IrReceiver.decodedIRData;

    // Holding the button sends repeat frames; skip them and keep listening
    // for a genuine, fully-decoded press.
    if (d.flags & IRDATA_FLAGS_IS_REPEAT) {
      IrReceiver.resume();
      continue;
    }

    // Frame was longer than the capture buffer: refuse it rather than store
    // something truncated (this really matters for Phase 5 raw / AC remotes).
    if (d.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
      IrReceiver.resume();
      Serial.println(F("IR: frame too long (overflow) - not stored"));
      return IR_READ_OVERFLOW;
    }

    // Not a known protocol. Raw capture/replay arrives in Phase 5; for now
    // we report it and store nothing.
    if (d.protocol == UNKNOWN) {
      IrReceiver.resume();
      Serial.println(F("IR: received, but protocol UNKNOWN (raw support: Phase 5)"));
      return IR_READ_UNDECODED;
    }

    // Good decode — fill the signal (only touch *out on success).
    out->type         = SIGNAL_DECODED;
    out->protocol     = (uint8_t)d.protocol;
    out->address      = d.address;
    out->command      = d.command;
    out->numberOfBits = (uint8_t)d.numberOfBits;
    out->flags        = d.flags;
    IrReceiver.resume();

    Serial.println(F("IR: learned"));
    ir_print_signal(out);
    return IR_READ_DECODED;
  }

  Serial.println(F("IR: no signal (timed out)"));
  return IR_READ_NONE;
}

bool ir_send(const LearnedSignal *sig) {
  (void)sig;
  // TODO (Phase 4 decoded, Phase 5 raw):
  //   DECODED -> rebuild IRData, IrSender.write(&data);
  //   RAW     -> IrSender.sendRaw(sig->raw, sig->rawLen, IR_SEND_KHZ);
  return false;
}
