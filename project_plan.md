# IRR — IR Remote Recorder/Replayer — Project Plan

> **What this device is:** An Arduino-based "IR memory" box. You point any
> infrared remote at it and press **READ** to learn a signal. You set an
> address on 6 switches and press **STORE** to save that signal to memory.
> Later you set the same address and press **SEND** to replay the signal out of
> an IR LED, controlling your TV / amplifier / etc. Think of it as a tiny,
> programmable universal remote that remembers up to 64 signals.

This document is the **full plan**. If you just want to get oriented quickly,
read [`session_context.md`](./session_context.md) first.

---

## 0. Reading guide (for a C beginner)

You said you're new to C and to IR. So this plan is written so that:

- **You** mostly do the physical work: wiring/soldering, uploading sketches
  with the Arduino IDE, pressing buttons, pointing remotes.
- **The code** (written across future sessions) does the rest, and is split
  into small, clearly-named files so you only ever change one thing at a time.
- Every phase ends with a **"How to test"** and a **"Done when…"** checklist
  that needs *no* coding from you — just clicking *Upload* and watching the
  Serial Monitor.

There is a **Glossary** at the end. Whenever a word looks scary (carrier,
mark/space, pull-up, debounce…), it's defined there.

---

## 1. Locked decisions (from the kickoff Q&A)

| Decision | Choice | Why it matters |
|---|---|---|
| **IR capture** | **Hybrid**: decode if possible, else record raw timing | Common remotes stored tiny & readable; "fancy" remotes still work |
| **Storage** | **Onboard EEPROM now**, behind a swappable storage interface | Fits today's hardware; external memory chip can be added later with no rewrite |
| **Build tool** | **Arduino IDE** | Simplest to upload; project is a `.ino` sketch + helper files |
| **Board** | Arduino **Uno or Nano** (both ATmega328P) | 32 KB flash, 2 KB RAM, **1 KB EEPROM** — identical for our purposes |
| **IR library** | **IRremote** (Arduino-IRremote), pinned at **v4.7.1** | De-facto standard; both decode and raw send — [repo](https://github.com/Arduino-IRremote/Arduino-IRremote) |
| **Display (opt.)** | Optional **I²C SSD1306 128×32** on A4/A5, behind a `USE_OLED` flag | Status text; off by default to protect RAM & keep the build lean (Phase 8) |

---

## 2. Hardware

### 2.1 Bill of materials (BOM)

| Qty | Part | Notes |
|---|---|---|
| 1 | Arduino Uno or Nano | ATmega328P |
| 1 | IR receiver module (TSOP38238 / VS1838B) | 38 kHz demodulating receiver — 3 pins (V, GND, OUT) |
| 1 | IR LED (940 nm) | The emitter |
| 1 | NPN transistor (2N2222 / PN2222 / BC547) | Drives the IR LED harder than a pin can alone → more range |
| 1 | Resistor ~220 Ω–1 kΩ | Transistor base resistor |
| 1 | Resistor ~100 Ω (or per LED) | IR LED current limit |
| 1 | LED + ~220 Ω resistor | **Error** indicator |
| — | (Sending indicator) | Uses the **onboard LED on pin 13** — no extra part |
| 3 | Momentary push buttons | READ / STORE / SEND |
| 1 | 6-way DIP switch (or 6 toggles) | The binary address (0–63) |
| 1 | 100 nF capacitor | Decoupling across the IR receiver's V/GND (recommended) |
| — | Breadboard + jumper wires | |
| *(optional)* | I²C OLED, SSD1306 **128×32** | Status display (Phase 8) — wires to A4/A5, addr 0x3C |
| *(future)* | I²C EEPROM (24LC256) + 2× 4.7 kΩ | Optional external storage upgrade (Phase 7) |

### 2.2 Suggested pin map (all pins flexible)

> All pins live in **one file, `config.h`**, so you can re-wire freely and just
> change the numbers in one place.

| Pin | Role | Mode | Notes |
|---|---|---|---|
| D0 / D1 | USB Serial (RX/TX) | — | **Reserved** — don't use for anything else |
| **D2** | IR receiver OUT | INPUT | Any digital pin works |
| **D3** | IR emitter (via transistor) | OUTPUT | Any pin works (software PWM) — suggestion only |
| **D4** | Button: READ | INPUT_PULLUP | Pressed = LOW |
| **D5** | Button: STORE | INPUT_PULLUP | Pressed = LOW |
| **D6** | Button: SEND | INPUT_PULLUP | Pressed = LOW |
| **D7** | Switch bit 0 (LSB) | INPUT_PULLUP | Closed/ON = LOW = bit `1` |
| **D8** | Switch bit 1 | INPUT_PULLUP | |
| **D9** | Switch bit 2 | INPUT_PULLUP | |
| **D10** | Switch bit 3 | INPUT_PULLUP | |
| **D11** | Switch bit 4 | INPUT_PULLUP | |
| **D12** | Switch bit 5 (MSB) | INPUT_PULLUP | |
| **D13** | Sending indicator | OUTPUT | Onboard LED |
| **A0** | Error indicator LED | OUTPUT | Used as a normal digital pin |
| **A4** | OLED SDA *(optional)* | I²C | Reserved for the SSD1306; free if unused |
| **A5** | OLED SCL *(optional)* | I²C | Reserved for the SSD1306; free if unused |

✅ **Good news — every pin above is freely movable** (just change `config.h`).
In IRremote 4.x the carrier is generated in *software* by default, so the IR
**send pin is no longer tied to a hardware-timer pin** (the old "must be D3"
rule is gone). The one caution is unrelated to our pin choice: the **receive**
side uses a timer that clashes with `analogWrite()` (PWM) and `tone()`. Our
design uses **neither** — LEDs are plain on/off `digitalWrite` — so there's no
conflict. Just don't add `tone()`/PWM later without reading §3.6.

### 2.3 Wiring notes

- **Buttons & switches:** one leg → the pin, other leg → **GND**. We enable
  the chip's internal pull-up resistor (`INPUT_PULLUP`), so **no external
  resistors** are needed. A pressed button / closed switch reads **LOW**.
- **IR receiver (TSOP):** V → 5V, GND → GND, OUT → D2. Add the 100 nF cap
  across V/GND. Its output idles HIGH and pulses LOW — the library handles that.
- **IR LED:** Don't drive it straight from a pin at full power. Use the
  transistor: pin D3 → base resistor → transistor base; LED + current resistor
  between 5V and the transistor collector; emitter → GND. (A future wiring
  diagram will make this concrete.)
- **DIP switch** is the nicest way to set a 0–63 address: 6 tiny switches you
  flip to make a binary number.

---

## 3. Software architecture

### 3.1 File layout (Arduino sketch folder `firmware/`)

The Arduino IDE compiles **every** `.ino`, `.cpp`, and `.h` in the sketch
folder together, so we can keep things tidy in small modules instead of one
giant file:

```
firmware/
├── firmware.ino       # setup() + loop() + the button state machine (the "glue")
├── config.h           # ALL pin numbers & tunable constants live here
├── signal.h           # the LearnedSignal data structure (one IR signal in RAM)
├── inputs.h / .cpp    # debounced buttons + read 6 switches → address 0..63
├── ir.h / .cpp        # IR receive (decode-or-raw) + IR send (decoded/raw)
├── storage.h / .cpp   # storage INTERFACE + the EEPROM backend
├── indicators.h / .cpp# sending LED + error LED patterns
├── serialcmd.h / .cpp # the serial test/debug command interface
└── display.h / .cpp   # optional I²C OLED status screen (USE_OLED in config.h)
```

**Why split like this?** As a beginner you'll usually only touch `config.h`
(pin numbers) and read the Serial Monitor. Each module has one job, so when
something breaks, it's obvious where to look.

### 3.2 The in-memory data model

There is **one** "current" signal in RAM at a time, the variable you called
`last_received_data`:

```
LearnedSignal {
  type            // EMPTY | DECODED | RAW
  // when DECODED (tiny, human-readable):
  protocol        // e.g. NEC, Sony, RC5 …
  address, command, numberOfBits, flags
  // when RAW (universal, bigger):
  rawLen          // how many timing values
  raw[ ]          // the on/off durations, in microseconds
}
```

- **READ** fills this from the IR receiver.
- **STORE** copies it into EEPROM at the switch address.
- **SEND** loads a slot from EEPROM back into a temp `LearnedSignal` and
  transmits it.

Only 1–2 of these live in RAM at once, so memory stays small.

### 3.3 The button state machine (in `loop()`)

```
IDLE
 ├─ READ pressed  → LISTEN for one clean IR frame
 │                   ├─ decoded?  → last_received_data = DECODED(...)
 │                   ├─ unknown?  → last_received_data = RAW(timings)
 │                   ├─ overflow? → ERROR (frame too long, nothing stored)
 │                   └─ feedback over serial + LEDs
 ├─ STORE pressed → addr = readSwitches(); storage.write(addr, last_received_data)
 │                   └─ ERROR if no signal yet, or memory full
 └─ SEND pressed  → addr = readSwitches(); s = storage.read(addr)
                     ├─ used? → blink D13, transmit s (decoded or raw)
                     └─ empty? → ERROR
```

Buttons are **debounced** (using `millis()`, not `delay()`) so one press = one
action.

### 3.4 Storage: the interface + the EEPROM backend

This is the part that keeps the project flexible. Code never talks to EEPROM
directly — it talks to a small **storage interface**:

```
storage_begin()                       // init + first-run format
storage_write(addr, signal) -> ok?    // save a signal to slot 0..63
storage_read(addr, &signal) -> ok?    // load a signal from a slot
storage_is_used(addr) -> bool
storage_clear(addr)                   // free a slot
storage_format()                      // wipe everything
storage_free_bytes() -> n             // how much room is left
```

Today this is implemented on the **onboard 1 KB EEPROM**. Later, swapping in an
external chip (Phase 7) means writing a *new backend* behind the *same*
functions — nothing else in the project changes.

#### Onboard EEPROM format v1 — decoded (Phase 3) + raw (Phase 5), both finalized

The hard truth: **1 KB cannot hold 64 *raw* signals** (a single TV frame is
~30–70 timing values; "fancy" remotes are bigger still). So the layout is a
small **directory** plus a shared **data heap**, which lets *all 64 slots* hold
tiny decoded signals, and lets *a few* slots hold raw signals until the heap
fills up — at which point STORE reports "memory full" on the error LED and
serial.

```
[0]      MAGIC  (detect first run / wrong format)     — EEPROM_MAGIC 0x49
[1]      VERSION                                       — EEPROM_FORMAT_VER 1
[2..257] DIRECTORY: 64 entries × 4 bytes
            byte0: flags  (bit7 = used, bit6 = isRaw)
            byte1: payload length in bytes
            byte2..3: payload start = absolute EEPROM address (little-endian)
[258..1023] DATA HEAP (766 bytes), bump-allocated as signals are stored
```

- **Decoded payload = exactly 7 bytes** (implemented Phase 3):
  `protocol(1) address(2,LE) command(2,LE) numberOfBits(1) flags(1)` → all 64
  slots fit easily (64×7 = 448 B, heap is 766 B).
- **Raw payload = exactly `rawLen` bytes** (implemented Phase 5): one byte per
  mark/space, each a **50 µs "tick"** (`MICROS_PER_TICK`). We store *ticks*, not
  microseconds, because (a) it's the receiver's native resolution — no precision
  lost — and (b) it **halves** the RAM/EEPROM footprint vs. 2-byte microseconds,
  which matters on a 2 KB/1 KB chip. The directory's `length` field *is* the tick
  count, so no separate length byte lives in the heap. Capture uses IRremote's
  `compensateAndStoreIRResultInArray()` (mark/space compensation + tick conv +
  clip to 255, leading gap dropped); replay uses the `uint8_t` `sendRaw()`
  overload at 38 kHz. A raw frame therefore costs `rawLen` bytes (~30–70 for a
  TV button), so only a handful fit the 766-byte heap before "memory full".
- Raw was added **without a VERSION bump** — it reuses the existing RAW flag and
  length field — so EEPROMs written by Phase 3/4 (decoded only) stay valid and
  keep their signals after upgrading to Phase 5.
- **Overwrite policy (v1, implemented):** re-storing a slot whose new payload is
  the **same length** overwrites *in place* (no heap growth) — so re-recording a
  decoded signal (fixed 7 bytes), or a raw signal of the same length, never
  fragments or grows the heap. A re-store at a *different* length (only possible
  for raw) appends at the high-water mark and **orphans the old bytes** until the
  next `format()`. Heap compaction remains the documented future improvement for
  variable-length raw (§7).
- `storage_free_bytes()` = EEPROM size − heap high-water mark, reported over
  serial (`mem`, and after every `store`) so you always know capacity. The
  high-water mark is recomputed from the directory, so clearing the top
  allocation reclaims its space automatically.

### 3.5 Handling "fancy" remotes gracefully

Your specific worry. Three concrete mechanisms:

1. **Raw fallback** — if the library can't decode it, we still capture and
   replay the raw timing. Nothing is "unsupported."
2. **Big enough capture buffer** — `RAW_BUFFER_LENGTH` (in `config.h`) is set
   large enough for long frames (library default **200**; ~100 covers normal
   48-bit protocols, big AC remotes need up to ~750). Bigger buffer = more RAM
   used, so there's a sensible cap.
3. **Overflow detection** — if a frame is *still* too long for the buffer, the
   library sets the `IRDATA_FLAGS_WAS_OVERFLOW` flag. We **detect it, refuse to
   store a truncated signal, and show an error** — instead of silently saving
   something broken.

> Reality check: ordinary TV/audio/STB remotes are well within reach. Some
> **air-conditioner** remotes send hundreds of values and may exceed onboard
> EEPROM (and even RAM) — those are the realistic case for the **external
> EEPROM upgrade (Phase 7)** and a larger buffer. The plan handles them by
> failing loudly and clearly, not by corrupting data.

### 3.6 IRremote 4.7.1 specifics (verified against the library)

Repo: <https://github.com/Arduino-IRremote/Arduino-IRremote> — pinned at
**v4.7.1**. The exact identifiers below were confirmed against the library so
future coding sessions don't have to re-derive them:

- **Send pin is flexible.** v4 makes the 38 kHz carrier in *software* by default
  (`SEND_PWM_BY_TIMER` not defined) → **any** pin can send. Set it with
  `IrSender.begin(IR_SEND_PIN)`, or `#define IR_SEND_PIN 3` before
  `#include <IRremote.hpp>` (the macro form is smaller/faster on AVR).
- **Receive pin is flexible** via `IrReceiver.begin(IR_RECEIVE_PIN, ...)`;
  receiving uses a hardware timer for 50 µs sampling.
- **Avoid `tone()` / `analogWrite()` while receiving** — they disturb that timer
  (`tone()` stops reception; PWM on pins 3 & 11 interferes). We use neither; if
  ever needed, call `IrReceiver.restartTimer()` afterward.
- **Capture buffer:** `#define RAW_BUFFER_LENGTH 200` before the include
  (default 200, must be even; up to ~750 for AC remotes). Bigger = more RAM.
- **Overflow:** `IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW`
  (library then sets `rawlen = 0` to stop repeat flagging) → we refuse to store.
- **Unknown protocol:** `IrReceiver.decodedIRData.protocol == UNKNOWN` → switch
  to raw capture/replay.
- **Raw replay:** `IrSender.sendRaw(uint16_t* timings, uint16_t length,
  uint8_t frequencyKHz)` at 38 kHz, or `sendRaw_P()` to read the array from
  flash (PROGMEM) on AVR.

---

## 4. Phased plan

Each phase is independently uploadable and testable. Phases build on each other,
so the device does something real as early as Phase 4.

### Phase 0 — Setup & scaffolding
**Goal:** Toolchain works; project skeleton exists; you can upload.

- [x] Install Arduino IDE; install **IRremote** library (**v4.7.1**, pinned in
      `session_context.md`).
- [ ] Pick board (Uno or Nano) and confirm it uploads (run the stock *Blink*).
- [x] Create the sketch folder `firmware/` with module stub files and `config.h`
      holding the pin map from §2.2. **(done — compiles clean under g++ mock)**
- [x] Write the wiring/BOM into the repo (done — plan §2 + `README.md`).

**How to test:** Upload *Blink*; onboard LED blinks. Open Serial Monitor at
**115200 baud**, see a "hello" banner from an empty sketch.
**Done when:** You can edit → upload → see serial output reliably.

---

### Phase 1 — Inputs & feedback foundation
**Goal:** The box can *sense* you and *signal back*, before any IR.

- [x] Debounced read of the 3 buttons (READ/STORE/SEND). *(millis() debounce +
      falling-edge in `inputs_poll()`; one press = one event, no repeat.)*
- [x] Read the 6 switches → an address `0..63`. Print it. *(`inputs_poll_address()`
      prints on a debounced change; `switches` command prints on demand.)*
- [x] `indicators`: helper to pulse the **sending LED** (D13) and flash the
      **error LED** (A0) in a recognizable pattern. *(`indicator_pulse_sending()`
      + `indicator_error()`.)*
- [x] `serialcmd` stub: typing `help` lists commands; `switches` prints the
      current address; `sw`/button events print to serial.

> **Code complete; awaiting the human's first on-hardware check (Arduino Nano).**
> STORE/SEND already take the real "nothing learned yet" / "slot empty" error
> paths (flashing the error LED), so both LEDs are exercised in Phase 1; the IR
> capture/store/send behind them arrives in Phases 2–4.

**How to test:** Open Serial Monitor. Press each button → see its name. Flip
DIP switches → see the address number change (e.g. `101010` → `42`).
**Done when:** Every button and switch reads correctly and one press = one line.

---

### Phase 2 — IR receive (decoded)
**Goal:** READ learns a recognizable remote into `last_received_data` (RAM only).

- [x] Integrate IRremote receive on D2. *(`ir_begin()`; `<IRremote.hpp>` included
      exactly once in `ir.cpp`, after `config.h` sets `RAW_BUFFER_LENGTH`.)*
- [x] On **READ**: capture one frame; if decoded, fill `last_received_data` as
      DECODED and print `protocol / address / command / bits`. *(`ir_receive()`
      listens up to `IR_LISTEN_TIMEOUT_MS`, skips repeat frames, only writes
      `*out` on success.)*
- [x] Handle "no signal" / "noise" cleanly (error feedback, nothing stored).
      *(timeout → `IR_READ_NONE`; UNKNOWN → `IR_READ_UNDECODED`; overflow →
      `IR_READ_OVERFLOW`; each flashes the error LED and stores nothing.)*
- [x] Serial: `read` command simulates a READ; `show` prints `last_received_data`.

> **Code complete; awaiting the human's on-hardware check (Arduino Nano + TSOP
> IR receiver on D2).** Requires the **IRremote v4.7.1** library installed.
> Verified off-device: full sketch compiles + links against a mock IRremote,
> and a scriptable `ir_receive()` test passes 16/16 (decoded, repeat-skip,
> overflow, unknown, timeout, stale-frame flush).

**How to test:** Point a normal TV remote, press READ → serial shows something
like `NEC addr=0x00 cmd=0x45`. Different buttons → different commands.
**Done when:** Common remotes decode reliably and print cleanly.

---

### Phase 3 — Storage interface + EEPROM backend (decoded only)
**Goal:** STORE persists decoded signals; they survive a power cycle.

- [x] Implement the `storage_*` interface (§3.4) with the EEPROM backend.
      *(`storage.cpp`: header + 64-entry directory + bump-allocated heap;
      all byte writes via `EEPROM.update()` to protect write endurance.)*
- [x] First-run **format** (magic/version), directory + heap allocation.
      *(`storage_begin()` re-formats on wrong magic/version; `storage_format()`
      clears the directory, then stamps the header **last** so a power loss
      mid-format simply re-formats next boot.)*
- [x] **STORE**: `addr = switches`; write `last_received_data`; error if empty.
      *(`handle_store()` now calls `storage_write()`; empty → error LED,
      heap-full → `ERR_MEM_FULL`. Re-recording a decoded signal overwrites the
      same slot in place — the 7-byte payload never grows the heap.)*
- [x] Serial: `store <addr>`, `dump` (list all slots), `show <addr>`,
      `clear <addr>`, `format`, `mem` (free bytes). *(all live in `serialcmd.cpp`.)*

> **Code complete; awaiting the human's power-cycle check (same Phase 2 wiring —
> no new hardware).** Verified off-device: the whole sketch compiles + links
> clean (`-Wall -Wextra`) against mock `Arduino.h`/`EEPROM.h`/`IRremote.hpp`,
> a storage unit test passes **36/36** (format, decoded write/read, in-place
> overwrite, **persistence across a simulated power cycle**, top-clear reclaim,
> bad-addr/empty/raw rejection, all-64-slots fill, memory-full refusal), and a
> scripted serial session runs learn→store→`dump`→`clear` end to end.

**How to test:** READ a remote → STORE at address 5 → `dump` shows slot 5 used →
**unplug & replug** → `dump` still shows slot 5. Persistence confirmed.
**Done when:** Decoded signals store, list, and survive power loss.

---

### Phase 4 — IR send (decoded) — *first end-to-end!*
**Goal:** SEND replays a stored decoded signal and actually controls a device.

- [x] **SEND**: `addr = switches`; load slot; if used, transmit decoded signal
      via IR LED on D3; pulse the sending LED. *(`handle_send()` loads the slot,
      lights D13 for the transmission, and calls `ir_send()`.)*
- [x] Error feedback if the slot is empty. *(empty slot → `ERR_EMPTY_SLOT`; a
      transmit failure — raw slot or a protocol `write()` can't encode →
      `ERR_NO_SIGNAL`.)*
- [x] Serial: `send <addr>`. *(`cmd_send()` in `serialcmd.cpp`.)*

`ir_send()` rebuilds an `IRData` from the stored decoded fields (protocol /
address / command / numberOfBits, flags reset to `IRDATA_FLAGS_EMPTY`), calls
`IrSender.write(&data)` (returns 0 for a protocol it can't encode), then
`IrReceiver.restartAfterSend()` so the next READ stays clean. Raw replay is
still Phase 5.

> **Code complete; awaiting the human's on-hardware end-to-end check.** Needs the
> **IR-LED transmitter wired** (transistor + IR LED on D3). Verified off-device:
> the whole sketch compiles + links clean (`-Wall -Wextra`) against the mock
> `Arduino.h`/`EEPROM.h`/`IRremote.hpp`, and a send unit test passes **32/32**
> (decoded fields forwarded, flags reset, `restartAfterSend()` called,
> unsupported-protocol / raw / empty all refused without a bogus transmit,
> a storage→send round-trip, and the serial `send <addr>` dispatch for
> used / empty / out-of-range slots).

**How to test:** Learn your TV's "mute" (Phase 2/3), store at addr 1, point the
box's IR LED at the TV, set switches to 1, press SEND → TV mutes. 🎉
**Done when:** Learn → store → send works for at least two real devices.

---

### Phase 5 — Raw fallback (the fancy-remote handling)
**Goal:** Remotes that *don't* decode are still learned and replayed.

- [x] On READ, if protocol is UNKNOWN, capture **raw timings** into
      `last_received_data` as RAW. *(`capture_raw()` in `ir.cpp` stores
      `rawlen-1` 50 µs ticks via `compensateAndStoreIRResultInArray()`;
      `RAW_BUFFER_LENGTH` stays 200.)*
- [x] **Overflow handling:** detect too-long frames; **refuse to store**;
      clear error signal (no truncated saves). *(the `IRDATA_FLAGS_WAS_OVERFLOW`
      check is unchanged; a defensive `rawlen-1 > RAW_MAX_TIMINGS` guard also
      refuses. Both leave `*out` untouched → `ERR_OVERFLOW`.)*
- [x] Extend the EEPROM backend to store/read variable-length raw payloads in
      the heap; **"memory full"** detection + error. *(`storage_write`/`_read`
      handle `SIGNAL_RAW`: payload = `rawLen` tick bytes, RAW flag set, refuse
      when the heap can't fit → `ERR_MEM_FULL`.)*
- [x] **SEND** replays raw via `IrSender.sendRaw(...)` (38 kHz carrier).
      *(`ir_send()` calls the `uint8_t` `sendRaw` overload, then
      `restartAfterSend()`.)*
- [x] Decide & document the **overwrite/fragmentation** policy. *(§3.4:
      same-length re-store in place; different-length raw re-store orphans old
      bytes until `format()`; compaction is a future item, §7.)*

> **Code complete; awaiting the human's on-hardware check with the "fancy"
> remote** (same wiring as Phase 4; needs the IR-LED transmitter on D3). Verified
> off-device: whole sketch compiles + links clean (`-Wall -Wextra`) against the
> mock `Arduino.h`/`EEPROM.h`/`IRremote.hpp`; a raw unit test passes **39/39**
> (raw capture on READ incl. gap-drop + overflow refusal, raw EEPROM
> store/read/round-trip, decoded+raw coexistence, same-/different-length raw
> overwrite, memory-full refusal, raw `sendRaw` replay at 38 kHz, and a full
> serial `read → store → dump → send` fancy-remote round-trip), and the Phase 4
> send suite still passes **33/33**.

**How to test:** Use a remote that printed `UNKNOWN` in Phase 2 (or an unusual
one). READ → `show` says RAW with N timings → STORE → SEND controls the target.
Try a very long frame and confirm it errors instead of corrupting.
**Done when:** At least one non-decodable remote round-trips, and over-long
frames fail loudly.

> **Note on the specific "fancy" remote that overflowed:** if it still reports
> overflow on READ, its frame is longer than `RAW_BUFFER_LENGTH` (200). That is
> the honest "fail loudly" path — raising the buffer costs RAM and, for
> AC-class remotes, exceeds the 1 KB EEPROM anyway, which is exactly the
> **external-EEPROM (Phase 7)** case. Remotes that fit now round-trip fully.

---

### Phase 6 — Robustness, debugging polish & docs
**Goal:** Predictable behavior at every edge; great test/debug experience.

- [ ] Edge cases: SEND on empty slot, STORE with no signal, memory full,
      switch change mid-press, held-button repeats.
- [ ] Optional **checksum** per stored entry to catch EEPROM corruption.
- [ ] Note on **EEPROM write endurance** (~100k writes) — don't write in a loop.
- [ ] Finalize the serial test interface (`help`, `dump`, `mem`, verbose `raw`
      toggle, `format`).
- [ ] Write a **manual test checklist** and a **troubleshooting table**
      (no decode / nothing on serial / send not working / etc.).

**How to test:** Walk the full checklist; every error path shows the right LED +
message; nothing hangs.
**Done when:** You can hand the box to someone else and the serial menu explains
itself.

---

### Phase 7 — *(Optional / future)* External EEPROM backend
**Goal:** Full raw capacity for all 64 slots, including big AC-style remotes.

- [ ] Add I²C EEPROM (24LC256, 32 KB) on A4/A5 with pull-ups.
- [ ] Implement a **new storage backend** behind the *same* `storage_*`
      interface; select backend in `config.h`.
- [ ] Optional migration of existing onboard data.

**How to test:** Same tests as Phases 3–5, now with many large raw signals
stored simultaneously.
**Done when:** 64 raw signals coexist; onboard vs external is a one-line config.

---

### Phase 8 — *(Optional)* OLED status display
**Goal:** A small I²C OLED shows live info; the LEDs stay for instant status.

- [ ] Wire an **I²C SSD1306 128×32** to A4 (SDA) / A5 (SCL); set `USE_OLED 1`.
- [ ] Pick a library — `Adafruit_GFX`+`Adafruit_SSD1306` (512 B buffer at
      128×32) **or** `U8g2`/`U8x8` (lighter on RAM); pin the version.
- [ ] Implement `firmware/display.cpp`: `display_begin()` + show the current
      address/slot, last protocol, and status text ("stored", "memory full",
      "empty", free bytes).
- [ ] Refresh **from idle / after an action only — never during an IR READ
      capture** (keeps the receive timing clean).
- [ ] Watch RAM: confirm headroom next to the IR buffers (the `USE_OLED` flag
      lets you build with or without the panel).

**How to test:** With `USE_OLED 1`, the panel shows the address as you flip
switches and updates after READ/STORE/SEND. With `USE_OLED 0`, the build is
unchanged (display calls compile to no-ops).
**Done when:** The OLED mirrors the serial status, and toggling `USE_OLED`
cleanly includes/excludes it.

---

## 5. Testing & debugging strategy

Because you do the hardware and you're new to C, the project leans hard on two
beginner-friendly tools:

### 5.1 The Serial Monitor (your main window into the box)
- Fixed baud **115200**.
- Every action prints a plain-English line: what was read, which address, how
  many bytes stored, how much memory is left, and any error.

### 5.2 The serial command interface (test without perfect timing)
You can drive and inspect the whole device by typing commands — no need to nail
button timing or even have all buttons wired yet:

| Command | Does |
|---|---|
| `help` | List all commands |
| `switches` | Show current 6-switch address (0–63) |
| `read` | Simulate the READ button |
| `store <addr>` | Save `last_received_data` to a slot |
| `send <addr>` | Transmit the signal in a slot |
| `show [<addr>]` | Show `last_received_data`, or a stored slot, in detail |
| `dump` | List every slot: used? decoded/raw? length? |
| `clear <addr>` | Free one slot |
| `format` | Wipe all storage |
| `mem` | Show free storage bytes |
| `raw on\|off` | Verbose raw-timing logging |

### 5.3 Indicator LEDs (at-a-glance status)
- **Sending LED (D13):** pulses while transmitting.
- **Error LED (A0):** distinct flash patterns for *no signal*, *empty slot*,
  *memory full*, *capture overflow*.

### 5.4 Per-phase acceptance
Each phase's **"Done when…"** is the acceptance test. We don't move on until
it's green.

### 5.5 (Optional, AI-side) compile checking
The chosen build path is the Arduino IDE (you upload). As a *nicety*, future
sessions may also compile-check the sketch headlessly with `arduino-cli` to
catch errors before you upload — but this is optional and never required.

---

## 6. Risks & mitigations

| Risk | Mitigation |
|---|---|
| 1 KB EEPROM can't hold many raw signals | Heap layout + free-space reporting + "memory full" error; external EEPROM (Phase 7) for scale |
| "Fancy"/long frames overflow capture | `RAW_BUFFER_LENGTH` 200 (up to ~750) + `IRDATA_FLAGS_WAS_OVERFLOW` detection that refuses truncated saves |
| EEPROM wear (~100k writes) | Only write on STORE (never in loops); optional checksum to detect corruption |
| Timer conflict with PWM/`tone()` | IR **send** is software PWM (any pin); the **receive** timer clashes with `analogWrite()`/`tone()` — our design uses neither (LEDs via `digitalWrite`), so no conflict; see §3.6 |
| OLED frame buffer vs 2 KB SRAM | Use **128×32** (512 B buffer, not 1 KB); consider a page-buffered/text lib (U8g2/U8x8); keep `USE_OLED` off until wired |
| Weak IR send range | Drive the LED via a transistor, not directly from a pin |
| Library behavior changes between versions | Pin the IRremote major version; record it in `session_context.md` |
| Beginner friction | One-file pin config, modular code, serial menu, per-phase checklists, glossary |

---

## 7. Open questions / decisions deferred (revisit when relevant)

- Exact EEPROM byte format: **decoded finalized in Phase 3** (7-byte payload)
  and **raw finalized in Phase 5** (`rawLen` bytes, one 50 µs tick each; the
  directory length carries the count) — see §3.4. Both live under format v1.
- Overwrite/fragmentation policy: **implemented (Phase 3, extended Phase 5)** —
  same-length re-store overwrites in place; a different-length re-store (only
  raw can differ in length) orphans old bytes until `format()`. **Heap
  compaction is still open** — a documented future improvement, worth revisiting
  in **Phase 6/7** if raw re-recording churn makes fragmentation bite.
- Whether to add the external EEPROM (**Phase 7**) — depends on whether you hit
  the onboard limit in practice with the remotes you care about (a single big
  "fancy"/AC remote can exhaust the 766-byte heap on its own).
- Error-LED blink patterns — finalized in **Phase 6**.

---

## 8. Glossary (plain-English)

- **IR (infrared):** Invisible light remotes use to talk. The box "sees" it
  with the IR receiver and "speaks" it with the IR LED.
- **Carrier / 38 kHz:** Remotes flicker the IR light ~38,000×/sec so the
  receiver can pick it out from sunlight. The library makes this flicker for us.
- **Mark / space:** A "mark" is light-on for a moment, a "space" is off. A
  remote button = a specific pattern of marks and spaces.
- **Protocol (decoded):** A known "language" (NEC, Sony, RC5…). If the remote
  speaks one, we can store it as a tiny code that's easy to read.
- **Raw:** When we don't recognize the language, we just record the exact
  on/off **timings** and play them back. Works for anything; uses more memory.
- **EEPROM:** A small (1 KB) memory on the chip that **remembers after power
  off** — where stored signals live.
- **Pull-up / `INPUT_PULLUP`:** A built-in resistor that keeps an input HIGH
  until a button/switch connects it to GND (then it reads LOW). Lets us wire
  buttons with no extra parts.
- **Debounce:** A button physically "chatters" for a few ms when pressed; we
  wait briefly so one press counts once.
- **PWM / timer:** Hardware that toggles a pin very fast. IRremote 4.x makes the
  38 kHz carrier in *software*, so the send pin can be any pin; a timer is used
  for *receiving*, which is why we avoid `analogWrite()`/`tone()`.
- **I²C:** A 2-wire bus (SDA + SCL) for talking to peripherals like the OLED.
  On the Uno/Nano those are pins A4/A5. The optional SSD1306 display uses it.
- **Frame buffer:** A display library's in-RAM copy of the screen. A 128×32
  OLED needs 512 bytes; 128×64 needs 1 KB — a lot on a 2 KB chip.

---

*End of plan. Keep [`session_context.md`](./session_context.md) updated at the
end of every working session.*
