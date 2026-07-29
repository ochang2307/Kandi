#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include "power.h"
#include "oled.h"
#include "gps.h"
#include "imu.h"
#include "mag.h"
#include "compass.h"
#include "navigation.h"
#include "ledlogic.h"
#include "selftest.h"
#include "radio.h"

// WS2812 ring: 16 LEDs, data-in on IO2 (a free S3 GPIO, not a strapping pin).
#define LED_PIN   2
#define NUM_LEDS  16
CRGB leds[NUM_LEDS];       // pixel buffer; edits here do nothing until FastLED.show()

// --- Test waypoint (edit these to move the target) ---
// ~100m due north of 20090 Glasgow Dr: verified with the ported haversine as
// 100.1m / bearing 360.0 from the front door. Stand at the house and the top
// LED pair should light when the board points north.
#define TARGET_LAT 37.276481
#define TARGET_LON -122.024791

// Ring display state. The 1s nav tick DECIDES (mode + which logical LED), the
// fast render tick below DRAWS. Split so the no-fix pulse can breathe smoothly
// at ~40fps without the nav math running any faster than 1Hz.
enum RingMode { RING_SEARCHING, RING_POINTING };
RingMode ringMode   = RING_SEARCHING;
int      pointerLed = 0;   // logical sector 0-7 (0 = top), valid when POINTING

bool imuOk = false;        // did the IMU answer its chip-ID read at boot?
bool magOk = false;        // ditto for the magnetometer

// Draw the current ring state. Called every ~25ms from loop().
//
// This is the display layer, so the logical->physical mapping lives HERE, not
// in ledForBearing(): logical sector s (45 deg wide, 0 = top, clockwise) lights
// physical pair {2s, 2s+1} on the 16-LED ring. That assumes physical LED 0 sits
// at 12 o'clock with indices running clockwise -- if the ring ends up mounted
// rotated, fix it in this function with an offset, never in the logic.
static void renderRing() {
    FastLED.clear();
    if (ringMode == RING_POINTING) {
        leds[(pointerLed * 2)     % NUM_LEDS] = CRGB(0, 0, 120);
        leds[(pointerLed * 2 + 1) % NUM_LEDS] = CRGB(0, 0, 120);
    } else {
        // Searching: one pixel at the top breathing on a ~2s triangle wave.
        // Unmistakably different from the steady two-pixel pointer.
        uint8_t breath = triwave8((uint8_t)(millis() / 8));   // 256 steps x 8ms
        leds[0] = CRGB(0, 0, scale8(breath, 120));
    }
    FastLED.show();
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Kandi board 1 boot");

    // Prove the ported core logic still matches the verified Python before
    // touching any hardware. Pure math -- needs nothing but serial. A FAIL
    // line here means the port diverged; catch it at boot, not in a field test.
    runSelfTests();

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

    // --- LoRa (additive) ---
    // ALDO3 rail is already up from initBoardPower(). The radio claims the
    // global SPI bus (FSPI, 12/13/11) -- the IMU deliberately uses its own
    // HSPI instance so this doesn't collide.
#if IS_SENDER
    Serial.println("!! SENDER build: TX begins in seconds. ANTENNA MUST BE ON. !!");
#endif
    if (!radioBegin()) {
        Serial.println("LORA offline -- check ALDO3 / antenna / wiring");
    }
}

void loop() {
    // Drain the GPS UART on EVERY pass, not once per tick. The module streams
    // ~960 bytes/sec at 9600 baud into a 256-byte driver buffer, so anything
    // that stalls the loop for more than ~250ms silently overruns it and
    // corrupts sentences. That's why the 1s cadence below is a millis() check
    // and not the delay(1000) this loop used to end with.
    gpsPump();

    // Radio state machine every pass, same contract as gpsPump(): never
    // blocks, all airtime happens inside the SX1262 behind the DIO1 interrupt.
    radioTick();

    // Fast render tick: redraw the ring every ~25ms so the searching pulse
    // breathes smoothly. Costs ~0.5ms per frame over RMT (non-blocking for
    // I2C/interrupts), nowhere near the 250ms GPS overrun budget.
    static uint32_t lastRender = 0;
    if (millis() - lastRender >= 25) {
        lastRender = millis();
        renderRing();
    }

    static uint32_t lastTick = 0;
    if (millis() - lastTick < 1000) return;
    lastTick = millis();

    // --- everything below runs once per second ---

    GpsStatus g = gpsStatus();

    // One pass of the compass pipeline. This is also where the accel and mag get
    // sampled -- it hands the raw values back so the readouts below don't have to
    // read the sensors a second time (imuRead() gates on a data-ready flag, so an
    // immediate repeat call would just come back empty). Non-blocking throughout.
    CompassData c = compassHeading();
    ImuData &imu = c.imu;
    MagData &m = c.mag;
    bool haveImu = imuOk && c.haveImu;
    bool haveMag = magOk && c.haveMag;

    // The compass can miss a tick (data-ready gate), and a one-tick heading
    // dropout shouldn't make the ring flicker back to "searching" -- so hold
    // the last good heading. GPS validity is NOT held the same way: gpsStatus()
    // already does its own 5s freshness window internally.
    static float lastHeading  = 0.0f;
    static bool  haveHeading  = false;
    if (c.ok) {
        lastHeading = c.heading;
        haveHeading = true;
    }

    // --- Navigation: current fix -> target, through the ported core logic ---
    // distance()/bearing() are the haversine pair, then relativeBearing()
    // subtracts where the device points, then ledForBearing() buckets into the
    // 8 logical sectors. Exactly the Python pipeline, on live data.
    double distM = 0.0, targetBrg = 0.0;
    float  relBrg = 0.0f;
    bool   navValid = g.valid && haveHeading;
    if (navValid) {
        distM     = distance(g.lat, g.lon, TARGET_LAT, TARGET_LON);
        targetBrg = bearing(g.lat, g.lon, TARGET_LAT, TARGET_LON);
        relBrg    = relativeBearing((float)targetBrg, lastHeading);
        pointerLed = ledForBearing(relBrg);
        ringMode   = RING_POINTING;
    } else {
        ringMode = RING_SEARCHING;
    }

    // Stack GPS, compass, sensor, and nav state on the OLED. Lines are kept
    // under 21 chars -- that's the 5x7 font's limit across 128px.
    char line[24];
    oledClear();

    RadioStats r = radioStats();
#if IS_SENDER
    snprintf(line, sizeof(line), "KANDI %-4s TX%5lu",
             navValid ? "NAV" : "SRCH", (unsigned long)r.txCount);
#else
    snprintf(line, sizeof(line), "KANDI %-4s RX%5lu",
             navValid ? "NAV" : "SRCH", (unsigned long)r.rxCount);
#endif
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

    // Heading, tilt-compensated, degrees clockwise from north. Pitch and roll
    // ride alongside it so a heading that looks wrong can be checked against the
    // tilt it was derived from.
    if (c.ok) {
        snprintf(line, sizeof(line), "HDG %5.1f P%+3.0f R%+3.0f",
                 c.heading, c.pitchDeg, c.rollDeg);
    } else {
        snprintf(line, sizeof(line), "HDG --  (no sample)");
    }
    oledText(0, 4, line);

#if IS_SENDER
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
#else
    // Receiver: these two pages show link quality instead of raw sensors
    // (still on serial) -- this is the readout that matters for range testing.
    if (r.rxCount > 0) {
        snprintf(line, sizeof(line), "PKT#%-6lu err %lu",
                 (unsigned long)r.lastCounter, (unsigned long)r.crcErrors);
        oledText(0, 5, line);
        snprintf(line, sizeof(line), "RSSI%5.0f  SNR%5.1f", r.rssi, r.snr);
        oledText(0, 6, line);
    } else {
        oledText(0, 5, r.online ? "PKT --  listening" : "RADIO OFFLINE");
    }
#endif

    // Nav summary on the bottom page: distance, absolute bearing to target,
    // relative bearing, and which logical LED is lit. Cross-checks the ring.
    if (navValid) {
        snprintf(line, sizeof(line), "T%5.0fm B%03.0f R%03.0f L%d",
                 distM, targetBrg, relBrg, pointerLed);
    } else {
        snprintf(line, sizeof(line), "TGT --  %s",
                 g.valid ? "(no heading)" : "(no fix)");
    }
    oledText(0, 7, line);

    oledShow();

    if (g.valid) {
        Serial.printf("GPS: fix  sats=%lu  lat=%.6f  lon=%.6f  hdop=%.2f\n",
                      (unsigned long)g.sats, g.lat, g.lon, g.hdop);
    } else {
        Serial.printf("GPS: NO FIX  sats=%lu  rx=%lu bytes%s\n",
                      (unsigned long)g.sats, (unsigned long)g.chars,
                      g.chars == 0 ? "  <- module silent!" : "");
    }

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

    // Heading is only as good as the hard-iron offsets, which are still zero --
    // expect it to be badly biased (and to barely move) until the figure-eight
    // calibration fills them in. The offsets are echoed so it's obvious from the
    // log alone which state you're looking at.
    if (c.ok) {
        Serial.printf("CMP: heading %6.1f deg  pitch %6.1f  roll %6.1f  "
                      "(offsets %.1f %.1f %.1f uT)\n",
                      c.heading, c.pitchDeg, c.rollDeg,
                      magOffsetX, magOffsetY, magOffsetZ);
    } else {
        Serial.printf("CMP: no heading (%s%s)\n",
                      c.haveImu ? "" : "no accel sample ",
                      c.haveMag ? "" : "no mag sample");
    }

    if (navValid) {
        Serial.printf("NAV: dist %.1fm  brg %.1f  rel %.1f  -> LED %d (pair %d+%d)\n",
                      distM, targetBrg, relBrg, pointerLed,
                      pointerLed * 2, pointerLed * 2 + 1);
    } else {
        Serial.printf("NAV: searching (%s)\n",
                      g.valid ? "fix ok, waiting on heading" : "no GPS fix");
    }
}
