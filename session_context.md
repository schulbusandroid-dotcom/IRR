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

- **Phase:** Phase 0 (Setup & scaffolding) — **planning complete, no code yet.**
- **Repo:** only the two planning docs exist (`project_plan.md`,
  `session_context.md`). No sketch files written yet (this was intentional —
  the human asked for the plan first).
- **Branch:** `claude/great-maxwell-84wfxu`.

---

## 3. Locked decisions (don't re-litigate without the human)

| Topic | Decision |
|---|---|
| IR capture | **Hybrid** — decode known protocols, fall back to **raw** timing for unknown/fancy remotes |
| Storage | **Onboard 1 KB EEPROM now**, behind a swappable **storage interface** so an external chip can be added later |
| Build tool | **Arduino IDE** (a `.ino` sketch + `.h`/`.cpp` helper files) |
| Board | Arduino **Uno or Nano** (ATmega328P): 32 KB flash, 2 KB RAM, 1 KB EEPROM |
| IR library | **IRremote** by Armin Joachimsmeyer, **v4.x** — *pin the exact version here once installed:* `IRremote vX.Y.Z` |

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

Suggested pin map (all pins centralized in `config.h`; freely movable **except
IR send = D3**, which is fixed by the library's carrier timer):

| Pin | Role | | Pin | Role |
|---|---|---|---|---|
| D2 | IR receiver OUT | | D9 | Switch bit 2 |
| **D3** | IR emitter (via transistor) ⚠️fixed | | D10 | Switch bit 3 |
| D4 | Button READ | | D11 | Switch bit 4 |
| D5 | Button STORE | | D12 | Switch bit 5 (MSB) |
| D6 | Button SEND | | D13 | Sending LED (onboard) |
| D7 | Switch bit 0 (LSB) | | A0 | Error LED |
| D8 | Switch bit 1 | | D0/D1 | **Serial — reserved** |

- Buttons/switches: `INPUT_PULLUP`, other leg to **GND** → **pressed/closed = LOW**.
- IR LED driven through a **transistor** (more range); don't drive from the pin directly.
- ⚠️ Don't use PWM/`analogWrite()`/`tone()` on **pins 3 & 11** while IR is active.

---

## 6. Planned file layout (Arduino sketch folder `IRR/`)

```
IRR.ino            setup()+loop()+button state machine (glue)
config.h           ALL pins + tunables (RAW_BUFFER_LENGTH, baud, EEPROM consts)
signal.h           LearnedSignal struct (one IR signal in RAM)
inputs.h/.cpp      debounced buttons + switches→address(0..63)
ir.h/.cpp          IR receive (decode-or-raw) + send (decoded/raw)
storage.h/.cpp     storage_* interface + EEPROM backend
indicators.h/.cpp  sending/error LED patterns
serialcmd.h/.cpp   serial test/debug command interface
```

Storage interface (keep stable so backends can swap):
`storage_begin / storage_write(addr,sig) / storage_read(addr,&sig) /
storage_is_used(addr) / storage_clear(addr) / storage_format / storage_free_bytes`

---

## 7. How to build / test

- **Build:** Arduino IDE → open the `IRR/` sketch → select Uno/Nano → Upload.
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

---

## 9. Next steps (update me each session)

- [ ] **Start Phase 0:** create the `IRR/` sketch folder with `config.h` (pin map
      from §5) and empty module files; add a serial "hello" banner.
- [ ] Human: install IRremote, **record its exact version in §3**.
- [ ] Human: confirm board (Uno vs Nano) and that *Blink* uploads.
- [ ] Then Phase 1 (inputs & feedback).

---

## 10. Session log

| Date | Who | What changed |
|---|---|---|
| 2026-06-19 | Claude | Kickoff Q&A; wrote `project_plan.md` + `session_context.md`. No code yet (by request). |
