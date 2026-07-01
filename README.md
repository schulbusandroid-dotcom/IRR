# IRR — IR Remote Recorder / Replayer

An Arduino (Uno/Nano) device that **learns** infrared remote signals and
**replays** them. Three buttons (**READ / STORE / SEND**), six switches as a
0–63 **address**, an **IR receiver** and an **IR LED**. Learn a signal, save it
to a slot, send it back later — a small programmable universal remote that
remembers up to 64 signals.

## Status

**Phase 4 — IR send (decoded): code complete; awaiting the human's on-hardware
end-to-end test.** Phases 1 (inputs & feedback), 2 (decoded IR receive) and 3
(decoded EEPROM storage) are **confirmed working on hardware** — stored signals
land in the right slots and survive a power cycle. The sketch in
[`firmware/`](./firmware) prints a serial banner, reads the buttons and address
switches, **learns a decoded IR signal on READ** (IRremote v4.7.1 on D2),
**persists it on STORE**, and now **transmits a stored decoded signal on SEND**
out of the IR LED on D3 — with a `send <addr>` serial command alongside
`store`/`show <addr>`/`dump`/`clear`/`format`/`mem`. Raw-signal capture/replay
(the "fancy" remotes that overflow on READ) is still **Phase 5**. Next step is
the **learn → store → send end-to-end test on the Nano**, which needs the
**IR-LED transmitter wired** (transistor + IR LED on D3) — see *Build & upload*.

## Repo layout

```
project_plan.md       Full plan: hardware, architecture, phases, glossary
session_context.md    2-minute orientation snapshot (start here)
firmware/             Arduino sketch — open firmware/firmware.ino in the IDE
  config.h            ALL pins + tunables — edit here if you re-wire
  firmware.ino        setup()/loop() + button state machine + banner
  signal.h            LearnedSignal data model (one IR signal in RAM)
  inputs.*            buttons + switches -> address (0..63)
  ir.*                IR receive / send (IRremote)         [receive+decoded send done; raw -> Phase 5]
  storage.*           storage interface + EEPROM backend   [decoded done; raw -> Phase 5]
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
