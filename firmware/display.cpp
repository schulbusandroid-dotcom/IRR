#include "display.h"
#include "config.h"

#if USE_OLED

// =====================================================================
//  Phase 8 (optional): real OLED implementation goes here.
//
//  Panel: I2C SSD1306, OLED_WIDTH x OLED_HEIGHT (128x32), addr OLED_I2C_ADDR.
//  Two good library choices for the ATmega328P:
//    A) Adafruit_GFX + Adafruit_SSD1306  (familiar; 512-byte buffer at 128x32)
//    B) U8g2 page-buffer, or U8x8 text-only (lighter on RAM)
//
//  Adafruit (I2C) sketch:
//    #include <Wire.h>
//    #include <Adafruit_GFX.h>
//    #include <Adafruit_SSD1306.h>
//    static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
//    display_begin(): Wire.begin();
//                     oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
//    draw:            oled.clearDisplay(); oled.setCursor(0,0);
//                     oled.print(...); oled.display();
//
//  IMPORTANT: refresh from idle / after an action — NOT during an IR READ
//  capture (a multi-ms I2C transfer can disturb receive timing).
// =====================================================================

void display_begin() {
  // TODO (Phase 8): Wire.begin(); panel init.
}

void display_clear() {
  // TODO (Phase 8)
}

void display_status(uint8_t address, const char *msg) {
  (void)address; (void)msg;
  // TODO (Phase 8): draw "addr N" + msg.
}

void display_message(const char *msg) {
  (void)msg;
  // TODO (Phase 8)
}

#else  // USE_OLED == 0 : no-op build (default — no library needed)

void display_begin() {}
void display_clear() {}
void display_status(uint8_t address, const char *msg) { (void)address; (void)msg; }
void display_message(const char *msg) { (void)msg; }

#endif // USE_OLED
