#pragma once
#include <stdint.h>

// Minimal SH1106 128x64 I2C driver, written against raw Wire.
//
// Why not u8g2? On this board (ESP32-S3 + arduino-esp32 2.0.17) u8g2 puts the
// I2C bus into a state where every transfer takes ~1s (a full frame ~60s).
// Raw Wire transfers are fast and clean (a full 8-page frame is ~24ms @400kHz,
// measured), so we drive the panel directly. Bonus: no dependency, and we
// control the exact init sequence.
//
// Framebuffer is page-major: fb[page*128 + col], one byte = 8 vertical pixels
// (bit 0 = top). Call oledBegin() once, then draw into the buffer and oledShow.

// addr7 = 7-bit I2C address (0x3C or 0x3D; on this board the OLED is 0x3D).
bool oledBegin(uint8_t addr7);

void oledClear();                                    // clear the framebuffer
void oledText(uint8_t x, uint8_t page, const char *s); // 5x7 text; page 0..7, x in px
void oledShow();                                     // push framebuffer to panel
