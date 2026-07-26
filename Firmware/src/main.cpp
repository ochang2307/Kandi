#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include "power.h"
#include "oled.h"
#include "gps.h"
#include "imu.h"
#include "mag.h"

// WS2812 ring: 16 LEDs, data-in on IO2 (a free S3 GPIO, not a strapping pin).
#define LED_PIN   2
#define NUM_LEDS  16
CRGB leds[NUM_LEDS];       // pixel buffer; edits here do nothing until FastLED.show()

int ledIndex = 0;          // current position of the chase around the ring

bool imuOk = false;        // did the IMU answer its chip-ID read at boot?
bool magOk = false;        // ditto for the magnetometer

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Kandi board 1 boot");

    // Power up the board rails FIRST. The OLED (and GPS, IMU, mag, LoRa) sit
    // on AXP2101-switched 3.3V rails that boot OFF.
    if (!initBoardPower()) {
        Serial.println("PMU init failed -- OLED will stay dark");
    }
    delay(100);

    // OLED + magnetometer share this bus: SDA=17, SCL=18.
    // Bump the TX buffer past the 128-byte default first: a full-width page
    // write is 128 data bytes + the 0x40 control byte = 129, and the GPS lines
    // below do run to the right edge.
    Wire.setBufferSize(256);
    Wire.begin(17, 18);
    Wire.setClock(400000);   // full 8-page frame ~24ms at 400kHz (measured clean)

    // Scan the 17/18 bus and disambiguate 0x3C/0x3D: on this board the QMC6310N
    // magnetometer and the SH1106 OLED both live in that address pair. Read
    // register 0x00 -- the mag returns its chip id 0x80, the OLED does not.
    // (The OLED is at 0x3D here; the mag squats on the usual 0x3C.)
    uint8_t oledAddr = 0;
    Serial.print("I2C scan (17/18):");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() != 0) continue;
        Serial.printf(" 0x%02x", addr);
        if (addr == 0x3C || addr == 0x3D) {
            Wire.beginTransmission(addr);
            Wire.write((uint8_t)0x00);
            Wire.endTransmission();
            Wire.requestFrom((int)addr, 1);
            uint8_t id = Wire.available() ? Wire.read() : 0xFF;
            if (id == 0x80) { Serial.print("(mag)"); }
            else            { Serial.print("(oled)"); oledAddr = addr; }
        }
    }
    Serial.println();

    if (oledAddr) {
        Serial.printf("OLED at 0x%02x, init\n", oledAddr);
        oledBegin(oledAddr);
    } else {
        Serial.println("no OLED found on 17/18");
    }

    // --- WS2812 ring (additive; the PMU + OLED init above is untouched) ---
    // addLeds<> binds the WS2812 chipset + GRB byte order to IO2 and our pixel
    // buffer, and sets up the RMT peripheral that clocks out the one-wire
    // protocol. Nothing lights until FastLED.show().
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    // Safety caps, set BEFORE any pixel is written: ~10% global brightness and a
    // hard 500mA @ 5V power ceiling so the ring can never brown out the board.
    // (16 WS2812s at full-white would pull ~1A.)
    FastLED.setBrightness(25);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
    FastLED.clear(true);   // start with the ring dark (all pixels off)

    // --- GPS (additive) ---
    // The ALDO4 rail feeding the MAX-M10S was already switched on by
    // initBoardPower() above, so all that's left is the enable pin + UART.
    if (gpsBegin()) {
        Serial.println("GPS UART alive (bytes arriving)");
    } else {
        Serial.println("GPS silent -- no bytes in 2s. Set GPS_DEBUG_RAW 1 in gps.h");
    }

    // --- Sensors (additive) ---
    // Both hang off the ALDO1/ALDO2 rails already enabled in initBoardPower().
    // A failure here is a chip-ID read that didn't come back: unpowered or
    // miswired, as opposed to a sensor that answers but reads garbage (for that,
    // flip the *_DEBUG_RAW toggles in imu.h / mag.h).
    imuOk = imuBegin();
    Serial.println(imuOk ? "IMU  chip ID OK" : "IMU  chip ID FAILED (no response)");

    magOk = magBegin();
    Serial.println(magOk ? "MAG  chip ID OK" : "MAG  chip ID FAILED (no response)");
}

void loop() {
    // Drain the GPS UART on EVERY pass, not once per tick. The module streams
    // ~960 bytes/sec at 9600 baud into a 256-byte driver buffer, so anything
    // that stalls the loop for more than ~250ms silently overruns it and
    // corrupts sentences. That's why the 1s cadence below is a millis() check
    // and not the delay(1000) this loop used to end with.
    gpsPump();

    static uint32_t lastTick = 0;
    if (millis() - lastTick < 1000) return;
    lastTick = millis();

    // --- everything below runs once per second ---

    // WS2812 chase: one mid-brightness pixel walks around the ring, 1 step/sec.
    FastLED.clear();                    // all pixels off (in the buffer)
    leds[ledIndex] = CRGB(0, 0, 120);   // single lit pixel, mid-brightness blue
    FastLED.show();                     // clock the buffer out to the ring

    GpsStatus g = gpsStatus();

    // Both sensor reads are non-blocking and return false rather than waiting,
    // so a sulking sensor can't stall the loop and starve the GPS UART.
    ImuData imu;
    MagData m;
    bool haveImu = imuOk && imuRead(imu);
    bool haveMag = magOk && magRead(m);

    // Mirror the ring index on the OLED so screen + ring cross-check at a
    // glance, then stack GPS and sensor state underneath it. Lines are kept
    // under 21 chars -- that's the 5x7 font's limit across 128px.
    char line[24];
    oledClear();

    snprintf(line, sizeof(line), "KANDI       L:%02d", ledIndex);
    oledText(0, 0, line);

    snprintf(line, sizeof(line), "SATS:%2lu  HDOP %.1f",
             (unsigned long)g.sats, g.hdop);
    oledText(0, 1, line);

    if (g.valid) {
        snprintf(line, sizeof(line), "%.6f", g.lat);
        oledText(0, 2, line);
        snprintf(line, sizeof(line), "%.6f", g.lon);
        oledText(0, 3, line);
    } else {
        // No fix yet: show the satellite count on its own line so it's obvious
        // when it starts climbing, plus the raw byte count -- 0 bytes means the
        // module is silent (a wiring/power problem), not just searching.
        oledText(0, 2, "NO FIX");
        snprintf(line, sizeof(line), "sats %lu  rx %lu B",
                 (unsigned long)g.sats, (unsigned long)g.chars);
        oledText(0, 3, line);
    }

    // Accel in m/s^2 to 1dp: at rest one axis should read about +/-9.8 and the
    // other two near 0. Tilt the board and watch gravity move between axes.
    if (haveImu) {
        snprintf(line, sizeof(line), "A %5.1f %5.1f %5.1f", imu.ax, imu.ay, imu.az);
    } else {
        snprintf(line, sizeof(line), "A %s", imuOk ? "no data" : "OFFLINE");
    }
    oledText(0, 5, line);

    // Mag in whole microtesla: the total field should be ~25-65uT anywhere on
    // Earth, and the values should swing as you rotate the board.
    if (haveMag) {
        snprintf(line, sizeof(line), "M %5.0f %5.0f %5.0f", m.mx, m.my, m.mz);
    } else {
        snprintf(line, sizeof(line), "M %s", magOk ? "no data" : "OFFLINE");
    }
    oledText(0, 6, line);

    oledShow();

    if (g.valid) {
        Serial.printf("GPS: fix  sats=%lu  lat=%.6f  lon=%.6f  hdop=%.2f  (LED %d)\n",
                      (unsigned long)g.sats, g.lat, g.lon, g.hdop, ledIndex);
    } else {
        Serial.printf("GPS: NO FIX  sats=%lu  rx=%lu bytes%s  (LED %d)\n",
                      (unsigned long)g.sats, (unsigned long)g.chars,
                      g.chars == 0 ? "  <- module silent!" : "", ledIndex);
    }

    // No math here on purpose -- this step only proves the sensors respond.
    // Heading, tilt compensation and calibration are milestone 3.
    if (haveImu) {
        Serial.printf("IMU: accel %7.3f %7.3f %7.3f m/s2   gyro %8.3f %8.3f %8.3f dps\n",
                      imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz);
    } else {
        Serial.printf("IMU: %s\n", imuOk ? "no new sample this tick" : "OFFLINE (chip ID failed at boot)");
    }

    if (haveMag) {
        Serial.printf("MAG: %8.2f %8.2f %8.2f uT%s\n",
                      m.mx, m.my, m.mz, m.overflow ? "   <- OVERFLOW" : "");
    } else {
        Serial.printf("MAG: %s\n", magOk ? "read failed this tick" : "OFFLINE (chip ID failed at boot)");
    }

    ledIndex = (ledIndex + 1) % NUM_LEDS;   // wrap 0..15 with the ring
}
