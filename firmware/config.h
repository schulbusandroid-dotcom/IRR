#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
//  IRR — central configuration
//
//  Everything you might want to change (pins, sizes, timings) lives here.
//  If you re-wire the hardware, you should only need to edit THIS file.
//  See ../project_plan.md (§2.2 pin map, §3.4 storage, §3.6 IRremote).
// =====================================================================

#include <Arduino.h>

// ---- Firmware identity ----------------------------------------------
#define FIRMWARE_NAME     "IRR"
#define FIRMWARE_VERSION  "0.6.0-phase5"

// ---- Serial ----------------------------------------------------------
#define SERIAL_BAUD       115200UL

// ---- Pin map ---------------------------------------------------------
// Buttons & switches use the chip's internal pull-ups and switch to GND,
// so a pressed button / closed switch reads LOW (see *_LEVEL below).
// Pins D0/D1 are the USB serial port — do NOT use them for anything else.
//
// In IRremote 4.x the IR carrier is generated in software, so BOTH IR
// pins can be any digital pin (there is no fixed "must be D3" rule).

// IR
#define PIN_IR_RECEIVE    2      // TSOP receiver OUT
#define PIN_IR_SEND       3      // IR LED (driven through a transistor)

// Buttons
#define PIN_BTN_READ      4
#define PIN_BTN_STORE     5
#define PIN_BTN_SEND      6

// 6 address switches (bit 0 = LSB ... bit 5 = MSB)
#define PIN_SW_BIT0       7
#define PIN_SW_BIT1       8
#define PIN_SW_BIT2       9
#define PIN_SW_BIT3       10
#define PIN_SW_BIT4       11
#define PIN_SW_BIT5       12

// Indicator LEDs
#define PIN_LED_SENDING   13     // onboard LED
#define PIN_LED_ERROR     A0     // external LED + ~220 ohm resistor

// Optional I2C OLED (see USE_OLED below) uses the fixed hardware I2C pins
// A4 = SDA and A5 = SCL. Leave those free. Spare pins: A1, A2, A3.

// ---- Input electrical convention ------------------------------------
// With INPUT_PULLUP + switch-to-ground, the pin reads LOW when
// pressed/closed. These names keep the rest of the code readable.
#define BTN_PRESSED_LEVEL   LOW
#define SWITCH_ON_LEVEL     LOW

// ---- Address / slots -------------------------------------------------
#define ADDRESS_BITS        6
#define NUM_SLOTS           (1 << ADDRESS_BITS)   // 64

// ---- Button debounce -------------------------------------------------
#define DEBOUNCE_MS         25

// ---- IR capture ------------------------------------------------------
// RAW_BUFFER_LENGTH is read by the IRremote library; it MUST be defined
// before <IRremote.hpp> is included. ir.cpp includes config.h first, so
// this override takes effect (Phase 2). 200 is the library default; raise
// toward ~750 for long air-conditioner remotes (costs ~2 bytes RAM each).
#ifndef RAW_BUFFER_LENGTH
#define RAW_BUFFER_LENGTH   200
#endif

// Max raw timing entries kept in a LearnedSignal in RAM. Each entry is one
// byte (a 50 us tick — see signal.h), so sizeof(LearnedSignal) grows by 1 byte
// per entry. Kept equal to RAW_BUFFER_LENGTH so any non-overflowing frame fits.
#define RAW_MAX_TIMINGS     RAW_BUFFER_LENGTH

// Carrier frequency (kHz) used when replaying a RAW signal.
#define IR_SEND_KHZ         38

// After READ, how long to listen for one IR frame before giving up (ms).
// Blocking window: press READ, then aim the remote and press a button.
#define IR_LISTEN_TIMEOUT_MS  5000UL

// ---- Optional OLED display (I2C SSD1306 128x32) ---------------------
// OFF by default, so the build stays lean until you wire the panel and
// install a display library. Set USE_OLED to 1 to enable; then fill in
// firmware/display.cpp (see project_plan "Phase 8 — Optional OLED").
// Uses hardware I2C pins A4 (SDA) / A5 (SCL). 128x32 keeps the RAM frame
// buffer at 512 bytes, leaving room beside the IR capture buffers.
#define USE_OLED            0
#define OLED_WIDTH          128
#define OLED_HEIGHT         32
#define OLED_I2C_ADDR       0x3C   // common for 128x32 (some panels: 0x3D)

// ---- Storage (onboard EEPROM) layout — v1 (decoded + raw) ------------
// Finalized for DECODED in Phase 3; RAW added in Phase 5 without a format
// bump — raw simply uses the directory's RAW flag + length, so v1 EEPROMs
// written by Phase 3/4 stay readable. See project_plan §3.4.  1 KB EEPROM:
//   [magic][version][64-entry directory][shared data heap]
#define EEPROM_MAGIC        0x49   // 'I' — marks a formatted EEPROM
#define EEPROM_FORMAT_VER   1
#define EEPROM_HEADER_BYTES 2      // [0] = magic, [1] = version
#define EEPROM_DIR_ENTRY_SZ 4      // flags, length, offset (2 bytes LE)
// (heap start = HEADER + NUM_SLOTS * DIR_ENTRY_SZ = 258; computed in storage.cpp)

// Directory entry byte0 = flags. bit7 marks the slot as used; bit6 marks the
// payload as raw. Other bits are reserved 0.
#define EEPROM_DIR_FLAG_USED 0x80
#define EEPROM_DIR_FLAG_RAW  0x40

// Serialized size of a DECODED payload in the heap (see storage.cpp):
// protocol(1) + address(2) + command(2) + numberOfBits(1) + flags(1).
// A RAW payload instead is exactly `rawLen` bytes (one 50 us tick each); the
// directory entry's length field carries that count, so no separate length
// byte is stored in the heap.
#define EEPROM_DECODED_BYTES 7

#endif // CONFIG_H
