#include "ir.h"
#include "config.h"

// =====================================================================
//  Phase 0: STUBS only. The IRremote library is intentionally NOT pulled
//  in yet, so the scaffolding compiles and uploads on its own.
//
//  Wiring it up later (project_plan §3.6 has the verified API names):
//
//    #include "config.h"            // defines RAW_BUFFER_LENGTH FIRST
//    #define IR_SEND_PIN PIN_IR_SEND
//    #include <IRremote.hpp>        // include exactly ONCE, here
//
//    ir_begin():    IrReceiver.begin(PIN_IR_RECEIVE, ENABLE_LED_FEEDBACK);
//                   IrSender.begin(PIN_IR_SEND);
//    ir_receive():  if (IrReceiver.decode()) {
//                     // overflow? -> bail out, do NOT store a partial frame
//                     if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) ...
//                     if (protocol == UNKNOWN) copy rawbuf -> out->raw[] (RAW)
//                     else fill decoded fields (DECODED)
//                     IrReceiver.resume();
//                   }
//    ir_send():     DECODED -> IrSender.write(&irdata);
//                   RAW     -> IrSender.sendRaw(sig->raw, sig->rawLen, IR_SEND_KHZ);
// =====================================================================

void ir_begin() {
  // TODO (Phase 2): initialize IrReceiver / IrSender.
}

bool ir_receive(LearnedSignal *out) {
  (void)out;
  // TODO (Phase 2 decoded, Phase 5 raw fallback + overflow handling).
  return false;
}

bool ir_send(const LearnedSignal *sig) {
  (void)sig;
  // TODO (Phase 4 decoded, Phase 5 raw).
  return false;
}
