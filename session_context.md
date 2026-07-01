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

- **Phase:** Phase 2 (IR receive, decoded) — **code complete; awaiting
  on-hardware test.** Phase 0 + **Phase 1 confirmed working on a Nano clone**
  (upload, serial, buttons, switch→address, both LEDs — all green).
- **Repo:** planning docs + the Arduino sketch in `firmware/`. **Real now:**
  `config.h`, `signal.h`, module APIs, debounced `inputs_poll()` /
  `inputs_poll_address()`, the LED helpers, the serial interface
  (`help`/`switches`/**`read`**/**`show`**), the banner, the button state
  machine, and **IR receive on D2** (`ir_begin()`, `ir_receive()` decoded path,
  `ir_print_signal()` via IRremote v4.7.1). **Still stubbed:** IR send
  (`ir_send()` → Ph 4), storage (`storage.*` → Ph 3/5), raw capture (Ph 5),
  OLED (`display.*` → Ph 8).
- **Verified (off-device, no AVR toolchain here):** whole sketch compiles +
  links clean (`-Wall -Wextra`) against mock `Arduino.h` + mock `IRremote.hpp`;
  scriptable unit tests pass for debounce (**24/24**) and `ir_receive`
  (**16/16**: decoded field-mapping, repeat-skip, overflow-refusal, unknown,
  timeout, stale-frame flush — all leaving `last_received_data` intact on
  failure). Real IRremote compile + on-air decode is the human's check.
- **▶ Pending human check (Arduino Nano + TSOP receiver on D2):** wire the IR
  receiver (V→5V, GND→GND, OUT→D2, 100 nF across V/GND), upload, open Serial
  @115200, `read` (or press READ), aim a TV remote → expect a line like
  `NEC addr=0x.. cmd=0x..`; different buttons → different commands. Needs the
  **IRremote v4.7.1** library installed (already done, per §3).
- **Branch:** `claude/arduino-nano-hardware-upload-ajbcd9` · **PR:** #1.

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

0. Setup & scaffolding — toolchain + skeleton + pin map ← **we are here**
1. Inputs & feedback — buttons, switches→address, LEDs, serial stub
2. IR receive (decoded) — READ fills `last_received_data`
3. Storage interface + EEPROM (decoded) — STORE persists; survives power cycle
4. IR send (decoded) — **first end-to-end:** learn→store→send controls a device
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
- [ ] **▶ Human — Phase 2 hardware test (Nano + TSOP on D2):** wire the IR
      receiver (V→5V, GND→GND, OUT→D2, 100 nF across V/GND), upload, open Serial
      @115200, type `read` (or press READ) and aim a TV remote → expect e.g.
      `NEC addr=0x.. cmd=0x..`; different buttons → different commands; `show`
      re-prints the last. Confirm a few common remotes decode cleanly.
- [ ] **Next session → Phase 3 (storage, decoded):** implement the `storage_*`
      EEPROM backend (first-run format, directory + heap), wire STORE to persist
      `last_received_data`, add `store`/`dump`/`clear`/`format`/`mem` serial
      commands. No new hardware — testable on the same wiring (plus a power-cycle
      to prove persistence).

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
