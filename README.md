# IRR — IR Remote Recorder / Replayer

An Arduino (Uno/Nano) device that **learns** infrared remote signals and
**replays** them. Three buttons (**READ / STORE / SEND**), six switches as a
0–63 **address**, an **IR receiver** and an **IR LED**. Learn a signal, save it
to a slot, send it back later — a small programmable universal remote that
remembers up to 64 signals.

## Status

**Phase 5 — Raw fallback (fancy remotes): code complete; awaiting the human's
on-hardware test.** Phases 1 (inputs & feedback), 2 (decoded IR receive) and 3
(decoded EEPROM storage) are **confirmed working on hardware**; Phase 4 (decoded
IR send) is code-complete. The sketch in [`firmware/`](./firmware) reads the
buttons and address switches, **learns an IR signal on READ** — decoded for known
protocols, or **raw timings for remotes it can't decode** (the "fancy" ones) —
**persists it on STORE**, and **transmits it on SEND** out of the IR LED on D3,
decoded via the library or raw via `sendRaw`. Raw is stored compactly as 50 µs
"ticks" (1 byte each); the 1 KB EEPROM holds all 64 decoded signals but only a
handful of raw ones before STORE reports "memory full". Over-long frames are
refused rather than saved truncated. Serial commands: `read`/`store <addr>`/
`send <addr>`/`show [<addr>]`/`dump`/`clear <addr>`/`format`/`mem`. Next step is
the **raw round-trip test on the Nano** with the previously-overflowing remote,
which needs the **IR-LED transmitter wired** (transistor + IR LED on D3) — see
*Build & upload*.

## Repo layout

```
project_plan.md       Full plan: hardware, architecture, phases, glossary
session_context.md    2-minute orientation snapshot (start here)
firmware/             Arduino sketch — open firmware/firmware.ino in the IDE
  config.h            ALL pins + tunables — edit here if you re-wire
  firmware.ino        setup()/loop() + button state machine + banner
  signal.h            LearnedSignal data model (one IR signal in RAM)
  inputs.*            buttons + switches -> address (0..63)
  ir.*                IR receive / send (IRremote)         [decoded + raw done]
  storage.*           storage interface + EEPROM backend   [decoded + raw done]
  indicators.*        sending / error LEDs
  serialcmd.*         serial test/debug interface
```

## Build & upload (Arduino IDE)

1. Install the **IRremote** library (v4.7.1) via the Library Manager.
2. Open **`firmware/firmware.ino`**.
3. Select your board (Arduino **Uno** or **Nano**) and the serial port → **Upload**.
4. Open **Serial Monitor at 115200 baud**. You should see the banner; type
   `help` and press Enter.

> Phase 0 has no hardware dependencies — it compiles and runs before any wiring,
> so you can use it to confirm your upload + serial toolchain works.

## More

- **[project_plan.md](./project_plan.md)** — the full plan (hardware BOM, pin
  map, architecture, all phases, risks, glossary).
- **[session_context.md](./session_context.md)** — quick orientation for a fresh
  session (human or AI).
