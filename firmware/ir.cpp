// =====================================================================
//  IR receive/send via the IRremote library (pinned v4.7.1).
//
//  Include order matters: config.h defines RAW_BUFFER_LENGTH, which the
//  library reads, so it must come BEFORE <IRremote.hpp>. The .hpp carries
//  the implementation and must be included EXACTLY ONCE in the whole
//  project — this is that one place. See project_plan.md §3.6.
//
//  Phase 2 implements the DECODED receive path; Phase 4 implements the
//  DECODED send path (ir_send). RAW capture, raw replay + overflow
//  storage arrive in Phase 5.
// =====================================================================

#include "config.h"          // must define RAW_BUFFER_LENGTH first
#include <IRremote.hpp>      // include exactly once, here
#include "ir.h"

void ir_begin() {
  // LED feedback would blink the built-in LED (D13) on every receive, but
  // D13 is our "sending" indicator — keep its meaning ours, so disable it.
  IrReceiver.begin(PIN_IR_RECEIVE, DISABLE_LED_FEEDBACK);
  // In v4.7.1 begin(sendPin) just records the pin; it does NOT touch the
  // built-in LED, so our D13 "sending" indicator stays ours to control.
  IrSender.begin(PIN_IR_SEND);
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
  if (sig->type == SIGNAL_DECODED) {
    // Rebuild a send frame from the stored decoded fields and let the library
    // choose the right protocol encoder. write() reads protocol/address/command
    // (plus numberOfBits for Sony's 12/15/20-bit variants) and returns 0 for a
    // protocol it cannot transmit. Zero-init so every unused field (and flags)
    // starts clean.
    IRData d = {};
    d.protocol     = (decode_type_t)sig->protocol;
    d.address      = sig->address;
    d.command      = sig->command;
    d.numberOfBits = sig->numberOfBits;
    d.flags        = IRDATA_FLAGS_EMPTY;   // a fresh frame, not a repeat

    bool ok = (IrSender.write(&d) != 0);

    // The library's blessed "keep receiving cleanly after a send" call. With
    // v4's software-PWM send it is effectively a NOP, but it is harmless and
    // keeps the next READ working if the send path ever needs the timer.
    IrReceiver.restartAfterSend();

    if (ok) {
      Serial.println(F("IR: sent"));
      ir_print_signal(sig);                // reuse the shared formatter
    } else {
      Serial.print(F("IR: cannot transmit protocol "));
      Serial.println(getProtocolString((decode_type_t)sig->protocol));
    }
    return ok;
  }

  if (sig->type == SIGNAL_RAW) {
    // Raw replay via IrSender.sendRaw(sig->raw, sig->rawLen, IR_SEND_KHZ)
    // arrives in Phase 5, alongside raw capture and raw EEPROM payloads.
    Serial.println(F("IR: raw send arrives in Phase 5"));
    return false;
  }

  return false;   // SIGNAL_EMPTY — nothing to send
}
