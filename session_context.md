# IRR — Session Context

> **Purpose of this file:** Let a brand-new session (human or AI) get oriented in
> ~2 minutes. It is the short, always-current snapshot. The full detail lives in
> [`project_plan.md`](./project_plan.md). **Update the "Status" and "Next steps"
> sections at the end of every session.**

---

## 1. What we're building (one paragraph)

An Arduino (Uno/Nano) **IR remote recorder/replayer**. It has **3 buttons**
(READ, STORE, SEND), **6 switches** (a binary address, 0–63), an **IR receiver**
and an **IR LED**. **READ** learns an IR signal from any remote into a RAM
variable (`last_received_data`). **STORE** saves that signal into EEPROM at the
address set on the switches. **SEND** replays the signal stored at the switch
address out of the IR LED. Goal: a small programmable universal remote that
remembers up to 64 signals and survives power-off.

---

## 2. Status

- **Phase:** Phase 4 (IR send — decoded) — **code complete; awaiting the human's
  on-hardware end-to-end check.** Phase 0/1 confirmed on a Nano clone; **Phase 2
  confirmed on hardware** (cheap remote decoded `NEC addr=0x0 cmd=0x52 bits=32`;
  a "fancy" remote correctly hit the overflow-refusal path: `IR: frame too long
  (overflow) - not stored`); **Phase 3 confirmed on hardware** (the cheap
  remote's signals store into the right slots and survive a power cycle — `dump`
  shows them appropriately). The "fancy" remote still overflows on READ; that's
  the Phase 5 raw-capture case, working as designed for now.
- **Repo:** planning docs + the Arduino sketch in `firmware/`. **Real now:**
  `config.h`, `signal.h`, module APIs, debounced inputs, the LED helpers, the
  banner + button state machine, **IR receive on D2** (`ir_receive()` decoded),
  **EEPROM storage** (`storage.*`), and **IR send on D3** (`ir_send()` decoded:
  STORE→SEND replays a stored decoded signal; serial `send <addr>`; SEND button
  pulses D13, errors on empty slot / non-transmittable). **Still stubbed:** raw
  capture + raw EEPROM payloads + raw replay (Ph 5), OLED (`display.*` → Ph 8).
- **Verified (off-device, no AVR toolchain here):** whole sketch compiles +
  links clean (`-Wall -Wextra`) against mock `Arduino.h`/`EEPROM.h`/
  `IRremote.hpp`; unit tests pass for debounce (**24/24**), `ir_receive`
  (**16/16**), **storage (36/36)**, and **ir_send (32/32**: decoded fields
  forwarded to `IrSender.write`, send flags reset to `IRDATA_FLAGS_EMPTY`,
  `IrReceiver.restartAfterSend()` called, unsupported-protocol / raw / empty all
  refused without a bogus transmit, a storage→send round-trip, and the serial
  `send <addr>` dispatch for used / empty / out-of-range slots). Real
  on-hardware transmit is the human's check.
- **▶ Pending human check (needs the IR-LED transmitter wired: transistor + IR
  LED on D3):** learn a remote (READ), `store 1`, aim the box's IR LED at the
  device, set switches to 1, press SEND (or `send 1`) → the device responds. Try
  two devices for the Phase 4 "done when".
- **Branch:** `claude/arduino-ir-overflow-phase-4-9im90n`.

---

## 3. Locked decisions (don't re-litigate without the human)

| Topic | Decision |
|---|---|
| IR capture | **Hybrid** — decode known protocols, fall back to **raw** timing for unknown/fancy remotes |
| Storage | **Onboard 1 KB EEPROM now**, behind a swappable **storage interface** so an external chip can be added later |
| Build tool | **Arduino IDE** (a `.ino` sketch + `.h`/`.cpp` helper files) |
| Board | Arduino **Uno or Nano** (ATmega328P): 32 KB flash, 2 KB RAM, 1 KB EEPROM |
| IR library | **IRremote** (Arduino-IRremote) **v4.7.1** (pinned) — [github](https://github.com/Arduino-IRremote/Arduino-IRremote). v4 = software-PWM send ⇒ **any** send pin |
| Display (opt.) | **I²C SSD1306 128×32** on A4/A5, behind `USE_OLED` (default **off**). 128×32 → 512 B RAM buffer. Additive module, Phase 8. |

---

## 4. The human's context (read me!)

- **New to C and to IR.** Explain plainly; keep code modular and one-thing-at-a-time.
- Will personally handle: **wiring/soldering, uploading via Arduino IDE, pressing
  buttons, aiming remotes.** Wants a **clear way to debug/test** — that's why the
  project has a **serial command interface** + **indicator LEDs** + per-phase
  checklists.
- Specific worry: **"fancy" remotes with longer IR packages** must be handled
  **gracefully** (→ raw fallback + capture-overflow detection that refuses to
  store truncated data; see plan §3.5).

---

## 5. Hardware quick-reference

Suggested pin map (all pins centralized in `config.h` and **all freely movable**
— IRremote 4.x uses software PWM, so even the IR send pin is not fixed):

| Pin | Role | | Pin | Role |
|---|---|---|---|---|
| D2 | IR receiver OUT | | D9 | Switch bit 2 |
| **D3** | IR emitter (via transistor) | | D10 | Switch bit 3 |
| D4 | Button READ | | D11 | Switch bit 4 |
| D5 | Button STORE | | D12 | Switch bit 5 (MSB) |
| D6 | Button SEND | | D13 | Sending LED (onboard) |
| D7 | Switch bit 0 (LSB) | | A0 | Error LED |
| D8 | Switch bit 1 | | D0/D1 | **Serial — reserved** |

- Buttons/switches: `INPUT_PULLUP`, other leg to **GND** → **pressed/closed = LOW**.
- IR LED driven through a **transistor** (more range); don't drive from the pin directly.
- ⚠️ Avoid `analogWrite()`/`tone()` anywhere — they disturb the IR **receive**
  timer. Our design doesn't use them (LEDs are plain `digitalWrite`).
- Optional **I²C OLED (SSD1306 128×32)** → A4 (SDA) / A5 (SCL), addr 0x3C,
  enabled by `USE_OLED` in `config.h`. Spare pins: A1–A3.

---

## 6. File layout (Arduino sketch folder `firmware/`)

Repo root is **not** the sketch folder — the sketch lives in `firmware/` (open
`firmware/firmware.ino` in the IDE). The docs stay at the repo root.

```
firmware/firmware.ino   setup()+loop()+state machine (glue) + serial banner
firmware/config.h       ALL pins + tunables (RAW_BUFFER_LENGTH, baud, EEPROM)
firmware/signal.h       LearnedSignal struct (one IR signal in RAM)
firmware/inputs.*       buttons + switches->address(0..63)  [read_address real]
firmware/ir.*           IR receive (decode-or-raw) + send   [STUB -> Ph 2/4/5]
firmware/storage.*      storage_* interface + EEPROM backend [STUB -> Ph 3/5]
firmware/indicators.*   sending/error LEDs                  [begin/sending real]
firmware/serialcmd.*    serial test/debug interface         [help/switches real]
firmware/display.*      optional I²C OLED (USE_OLED)         [no-op stub -> Ph 8]
```

Storage interface (keep stable so backends can swap):
`storage_begin / storage_write(addr,sig) / storage_read(addr,&sig) /
storage_is_used(addr) / storage_clear(addr) / storage_format / storage_free_bytes`

---

## 7. How to build / test

- **Build:** Arduino IDE → open `firmware/firmware.ino` → select Uno/Nano → Upload.
- **Serial Monitor:** **115200 baud.** Every action prints a plain-English line.
- **Serial commands** (test without perfect button timing):
  `help`, `switches`, `read`, `store <addr>`, `send <addr>`, `show [<addr>]`,
  `dump`, `clear <addr>`, `format`, `mem`, `raw on|off`.
- **LEDs:** D13 pulses while sending; A0 flashes patterns for errors
  (no signal / empty slot / memory full / capture overflow).
- Acceptance = each phase's **"Done when…"** in the plan.

---

## 8. Phase map (full detail in project_plan.md §4)

0. Setup & scaffolding — toolchain + skeleton + pin map
1. Inputs & feedback — buttons, switches→address, LEDs, serial stub
2. IR receive (decoded) — READ fills `last_received_data`  ✅ hardware-confirmed
3. Storage interface + EEPROM (decoded) — STORE persists  ✅ hardware-confirmed
4. IR send (decoded) — learn→store→send controls a device ← **we are here** (code done)
5. Raw fallback — fancy remotes: capture/store/replay raw; overflow handling
6. Robustness + debug polish + docs — edge cases, checksum, test checklist
7. *(future)* External EEPROM backend — full raw capacity for all 64 slots
8. *(optional)* OLED status display — I²C SSD1306 128×32, behind `USE_OLED`

---

## 9. Next steps (update me each session)

- [x] **Phase 0 scaffolding** created in `firmware/` (compiles; banner + minimal
      serial `help`/`switches`; all device logic stubbed).
- [x] Human installed **IRremote v4.7.1** (recorded in §3).
- [x] **OLED folded into plan** (optional): A4/A5 reserved, `display.*` no-op
      stub + `USE_OLED` flag in `config.h`, Phase 8 added. Off by default.
- [x] **Phase 1 implemented + CONFIRMED ON HARDWARE** (Nano clone): debounced
      buttons, switch→address, both LEDs, serial — all working.
- [x] **Phase 2 implemented:** IRremote v4.7.1 receive on D2 — `ir_begin()`,
      `ir_receive()` (blocking listen-with-timeout, repeat-skip, overflow/unknown
      handling, only writes on success), `ir_print_signal()`, `read`/`show`
      serial commands, READ handler with LED feedback. Compiles + unit-tested
      off-device (16/16).
- [x] **Phase 2 CONFIRMED ON HARDWARE (Nano + TSOP on D2):** cheap remote
      decoded `NEC addr=0x0 cmd=0x52 bits=32`; a "fancy" remote correctly
      refused an over-long frame (`IR: frame too long (overflow) - not stored`).
      Both the decode path and the overflow-refusal path are proven on-air.
- [x] **Phase 3 implemented (storage, decoded):** `storage.*` EEPROM backend
      (first-run format with magic/version, 64-entry directory + bump-allocated
      heap, 7-byte decoded payload, in-place same-size overwrite, `EEPROM.update`
      for wear); wired STORE to persist `last_received_data` (mem-full → error
      LED); added `store`/`show <addr>`/`dump`/`clear`/`format`/`mem` serial
      commands. Bumped to `0.4.0-phase3`. Verified off-device: full sketch
      compiles+links and a storage unit test passes **36/36** (incl. persistence
      across a simulated power cycle); scripted serial session runs end to end.
- [x] **Phase 3 persistence CONFIRMED ON HARDWARE:** the cheap remote's signals
      store into the correct slots and survive a power cycle (`dump` shows them
      appropriately). The "fancy" remote still overflows on READ — expected; raw
      capture for it is Phase 5.
- [x] **Phase 4 implemented (IR send, decoded):** `ir_send()` re-encodes a stored
      decoded signal via `IrSender.write()` and calls `IrReceiver.restartAfterSend()`;
      wired `handle_send()` (load slot → pulse D13 → transmit → error on empty /
      non-transmittable) and the serial `send <addr>` command. Bumped to
      `0.5.0-phase4`. Verified off-device: full sketch compiles+links
      (`-Wall -Wextra`) and an ir_send unit test passes **32/32**.
- [ ] **▶ Human — Phase 4 end-to-end test (needs the IR-LED transmitter wired:
      transistor + IR LED on D3):** READ a remote, `store 1`, aim the box at the
      device, set switches to 1, press SEND (or `send 1`) → the device responds.
      Confirm learn → store → send works for at least two real devices.
- [ ] **Next session → Phase 5 (raw fallback):** capture raw timings when the
      protocol is UNKNOWN (the "fancy" remote), refuse over-long frames, extend
      the EEPROM backend to variable-length raw payloads, and replay raw via
      `IrSender.sendRaw(...)`. This is the direct fix for the fancy-remote
      overflow.

---

## 10. Session log

| Date | Who | What changed |
|---|---|---|
| 2026-06-19 | Claude | Kickoff Q&A; wrote `project_plan.md` + `session_context.md`. No code yet (by request). |
| 2026-06-19 | Claude | Pinned IRremote **v4.7.1**; verified library APIs; corrected send-pin note (v4 software PWM ⇒ any pin); added plan §3.6. |
| 2026-06-19 | Claude | **Phase 0 scaffolding:** created `firmware/` (config.h, signal.h, module stubs, banner, serial help/switches) + root `README.md`. Compiles clean under g++ mock; device logic stubbed for Phase 1+. |
| 2026-06-19 | Claude | Folded in **optional I²C OLED** (SSD1306 128×32): reserved A4/A5, added `display.*` no-op stub + `USE_OLED` flag, BOM/pin-map/risk/glossary notes, and **Phase 8**. Build still compiles with `USE_OLED 0`. |
| 2026-06-28 | Claude | **Phase 1 (Inputs & feedback):** implemented debounced `inputs_poll()` + `inputs_poll_address()`, button/address events over serial, `indicator_pulse_sending()`, and real button handlers (READ acks; STORE/SEND take "nothing learned"/"slot empty" error paths). Bumped version to `0.2.0-phase1`. Verified off-device: full sketch compiles+links (`-Wall -Wextra`) and a scriptable debounce unit test passes 24/24. **Stopped here — first on-hardware upload/test (Nano) is now the human's step.** |
| 2026-07-01 | Human | **Phase 1 confirmed on a Nano clone:** upload OK, serial works, buttons read, switch binary→address mapping correct, both feedback LEDs work. Green-lit continuing. |
| 2026-07-01 | Claude | **Phase 2 (IR receive, decoded):** brought in IRremote v4.7.1 on D2 — `ir_begin()`, `ir_receive()` (listen-with-timeout, repeat-skip, overflow/unknown handling, writes `*out` only on success), `ir_print_signal()`; wired the READ handler + `read`/`show` serial commands; added `IR_LISTEN_TIMEOUT_MS`; bumped to `0.3.0-phase2`. Verified off-device: full sketch compiles+links against a mock IRremote and a scriptable `ir_receive` test passes 16/16. **Stopped — on-hardware IR decode test (Nano + TSOP on D2) is the human's step.** |
| 2026-07-01 | Human | **Phase 2 confirmed on hardware (two remotes):** a cheap remote decoded cleanly (`NEC addr=0x0 cmd=0x52 bits=32`); a "fancy" remote produced `IR: frame too long (overflow) - not stored` — i.e. the overflow-refusal path fired as designed (raw capture for such remotes is Phase 5). Green-lit continuing. |
| 2026-07-01 | Claude | **Phase 3 (storage, decoded EEPROM):** implemented `storage.*` on the onboard EEPROM — first-run format (magic 0x49 / version 1, header stamped last), 64-entry directory + bump-allocated data heap, 7-byte decoded payload, same-size in-place overwrite, `EEPROM.update` to spare write endurance. Wired STORE to `storage_write()` (mem-full → `ERR_MEM_FULL`); added `store`/`show <addr>`/`dump`/`clear`/`format`/`mem` serial commands and a `parse_addr` helper. Bumped to `0.4.0-phase3`. Verified off-device: whole sketch compiles+links (`-Wall -Wextra`) vs mock `Arduino.h`/`EEPROM.h`/`IRremote.hpp`; storage unit test **36/36** (incl. persistence across a simulated power cycle, top-clear reclaim, all-64-fill, memory-full refusal); scripted serial session runs learn→store→dump→clear. **Stopped — on-hardware power-cycle persistence test (Nano) is the human's step.** |
| 2026-07-01 | Human | **Phase 3 confirmed on hardware:** the cheap remote's signals store into the right save slots and `dump` shows them appropriately, surviving a power cycle. The "fancy" remote still overflows on READ (the Phase 5 raw-capture case). Green-lit Phase 4. |
| 2026-07-01 | Claude | **Phase 4 (IR send, decoded):** implemented `ir_send()` — rebuilds an `IRData` from the stored decoded fields (protocol/address/command/numberOfBits, flags reset to `IRDATA_FLAGS_EMPTY`), calls `IrSender.write(&data)` (0 = protocol not encodable), then `IrReceiver.restartAfterSend()` to keep the next READ clean; raw replay left for Phase 5. Wired `handle_send()` (load slot → `indicator_sending` around the transmit → `ERR_EMPTY_SLOT` / `ERR_NO_SIGNAL` feedback) and added the serial `send <addr>` command (`cmd_send`). Confirmed `IrSender.begin(sendPin)` in v4.7.1 does NOT grab the built-in LED, so D13 stays our indicator. Bumped to `0.5.0-phase4`. Verified off-device: full sketch compiles+links (`-Wall -Wextra`); ir_send unit test **32/32** (fields forwarded, flags reset, restartAfterSend called, unsupported/raw/empty refused without a bogus transmit, storage→send round-trip, serial `send` dispatch for used/empty/out-of-range). **Stopped — on-hardware end-to-end transmit (Nano + IR-LED transmitter on D3) is the human's step.** |
