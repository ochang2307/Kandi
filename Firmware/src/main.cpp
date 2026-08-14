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
#include "mesh.h"     // MESH_DEVICE_ID for the header line
#include "calibration.h"
#include "roster.h"

// WS2812 ring: 16 LEDs, data-in on IO2 (a free S3 GPIO, not a strapping pin).
#define LED_PIN   2
#define NUM_LEDS  16
CRGB leds[NUM_LEDS];       // pixel buffer; edits here do nothing until FastLED.show()

// --- Test waypoint (range-test builds only; MESH_MODE navigates to a live
// mesh member from the roster instead) ---
#define TARGET_LAT 37.276481
#define TARGET_LON -122.024791

// BOOT button (IO0). Only input we have; used to cancel the arrival flash.
#define BUTTON_PIN 0

// Ring display state, per the design doc's LED UX. The 1s nav tick DECIDES
// (mode + which logical LED + blink rate), the fast ~25ms render tick DRAWS,
// so blinks and breathing stay smooth while the nav math runs at 1Hz.
enum RingMode {
    RING_NO_FIX,          // no own GPS fix: blue breathing pulse at top
    RING_LOST,            // no member heard / >120s silent: magenta slow
                          //   breathing at top -- NOT a bearing
    RING_NAVIGATING,      // live bearing: LED pair blinking, rate = distance
    RING_STALE,           // >30s-old bearing: same pair, dim breathing --
                          //   "remembered, not live" must read differently
    RING_ARRIVED_FLASH,   // <10m, first ~8s: whole ring flashing
    RING_ARRIVED_STEADY,  // <10m after the flash (or button): steady pair
    RING_POINTING,        // range-test builds: steady pair at the waypoint
};
RingMode ringMode    = RING_NO_FIX;
int      pointerLed  = 0;     // logical sector 0-7 (0 = top)
uint32_t blinkHalfMs = 800;   // NAVIGATING blink half-period (on time = off time)

bool imuOk = false;        // did the IMU answer its chip-ID read at boot?
bool magOk = false;        // ditto for the magnetometer

// Draw the current ring state. Called every ~25ms from loop().
//
// This is the display layer, so the logical->physical mapping lives HERE, not
// in ledForBearing(): logical sector s (45 deg wide, 0 = top, clockwise)
// lights physical pair {2s, 2s+1} on the 16-LED ring (the "single LED" of the
// 8-sector model). Assumes physical LED 0 at 12 o'clock, indices clockwise --
// if the ring mounts rotated, fix it here with an offset, never in the logic.
//
// Color notes: navigation is blue for now (member colors come with bonding).
// Yellow and green are RESERVED (battery/charging, design doc). LOST uses
// magenta so it can never be confused with the blue NO_FIX pulse -- both sit
// at the top pixel, and "no GPS" vs "no friend" need different answers.
static void setPair(int sector, const CRGB &c) {
    leds[(sector * 2)     % NUM_LEDS] = c;
    leds[(sector * 2 + 1) % NUM_LEDS] = c;
}

static void renderRing() {
    FastLED.clear();
    uint32_t now = millis();

    switch (ringMode) {
        case RING_NO_FIX: {
            // ~2s blue breathing at the top pixel.
            uint8_t b = triwave8((uint8_t)(now / 8));
            leds[0] = CRGB(0, 0, scale8(b, 120));
            break;
        }
        case RING_LOST: {
            // Slower (~4s) magenta breathing at the top pixel. Not a bearing.
            uint8_t b = triwave8((uint8_t)(now / 16));
            leds[0] = CRGB(scale8(b, 100), 0, scale8(b, 100));
            break;
        }
        case RING_NAVIGATING: {
            // Blink at the distance-bucket rate (device.py blink_rate_for).
            if ((now / blinkHalfMs) % 2 == 0) setPair(pointerLed, CRGB(0, 0, 120));
            break;
        }
        case RING_STALE: {
            // Same bearing, but dim breathing instead of a crisp blink:
            // "this is where they WERE." Field-measured 15-25s update gaps
            // at range make this state common, not exceptional.
            uint8_t b = triwave8((uint8_t)(now / 8));
            setPair(pointerLed, CRGB(0, 0, scale8(b, 70)));
            break;
        }
        case RING_ARRIVED_FLASH: {
            // Whole ring, fast flash. Power is fine: global brightness 25 +
            // the 500mA FastLED cap stay in force.
            if ((now / 150) % 2 == 0) {
                for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 120);
            }
            break;
        }
        case RING_ARRIVED_STEADY:
        case RING_POINTING: {
            setPair(pointerLed, CRGB(0, 0, 120));
            break;
        }
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
    runMeshSelfTests();

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

    // BOOT button: cancels the arrival flash. IO0 is a strapping pin but is
    // exactly the on-board button, safe as an input after boot.
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Load stored hard-iron offsets (NVS) into compass.cpp's magOffsetX/Y/Z.
    // Prints whether a calibration exists -- without one the heading is biased
    // by ~250uT of board hard iron and the OLED flag shows "---".
    calBegin();

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

    // Calibration: serial commands ('cal' / 'calclear') + capture sampling.
    // Non-blocking, one I2C read per pass at most.
    calTick();

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

    // Calibration capture owns the whole screen while it runs: the ONLY thing
    // that matters during those 30s is which axis still needs coverage. Each
    // axis must see both extremes of Earth's field, so its spread needs to
    // reach ~100 uT (2 x ~50 uT) -- an axis stuck at "LOW" is the one you
    // haven't rotated through the vertical yet.
    {
        CalStatus cs = calStatus();
        if (cs.active) {
            char cline[24];
            oledClear();
            snprintf(cline, sizeof(cline), "CALIBRATING     %2lus",
                     (unsigned long)((cs.msRemaining + 999) / 1000));
            oledText(0, 0, cline);
            oledText(0, 1, "rotate ALL axes");

            struct { const char *n; float v; } ax[3] = {
                {"X", cs.spreadX}, {"Y", cs.spreadY}, {"Z", cs.spreadZ}
            };
            for (int i = 0; i < 3; i++) {
                const char *verdict = ax[i].v >= 100.0f ? "OK"
                                    : ax[i].v >= 50.0f  ? "..."
                                    :                     "LOW <-";
                snprintf(cline, sizeof(cline), "%s %4.0fuT  %s",
                         ax[i].n, ax[i].v, verdict);
                oledText(0, 3 + i, cline);
            }
            oledText(0, 7, "fig-8 then flip it");
            oledShow();

            Serial.printf("CAL: %2lus left  spread X %.0f  Y %.0f  Z %.0f uT\n",
                          (unsigned long)(cs.msRemaining / 1000),
                          cs.spreadX, cs.spreadY, cs.spreadZ);
            return;   // normal readout resumes when the capture ends
        }
    }

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

    // --- Navigation, through the ported core logic ---
    // distance()/bearing() are the haversine pair, then relativeBearing()
    // subtracts where the device points, then ledForBearing() buckets into the
    // 8 logical sectors. Exactly the Python pipeline (device.py's per-member
    // loop, single-target for now), on live data.
    double distM = 0.0, targetBrg = 0.0;
    float  relBrg = 0.0f;
    bool   navValid = false;

#if MESH_MODE
    // Target = most recently heard mesh member with a real position.
    RosterEntry tgt;
    bool haveTgt = rosterMostRecent(tgt);
    uint32_t tgtAgeS = 0;
    uint8_t  tgtId = 0;

    // Arrival flash bookkeeping: flash for ~8s on ENTERING arrived, then
    // decay to steady. The BOOT button cancels the flash early.
    static bool     wasArrived = false;
    static uint32_t arrivedFlashUntil = 0;

    if (!g.valid) {
        ringMode = RING_NO_FIX;
        wasArrived = false;
    } else if (!haveTgt) {
        ringMode = RING_LOST;   // fix but no member ever heard: nothing to point at
        wasArrived = false;
    } else {
        tgtId   = tgt.id;
        tgtAgeS = (millis() - tgt.lastUpdateMs) / 1000;
        distM     = distance(g.lat, g.lon, tgt.latE7 * 1e-7, tgt.lonE7 * 1e-7);
        targetBrg = bearing(g.lat, g.lon, tgt.latE7 * 1e-7, tgt.lonE7 * 1e-7);
        if (haveHeading) {
            relBrg     = relativeBearing((float)targetBrg, lastHeading);
            pointerLed = ledForBearing(relBrg);
        }
        navValid = haveHeading;

        // State by freshness FIRST, then distance. A 40s-old position can't
        // credibly claim "arrived" -- staleness gates everything. Thresholds:
        // 30s stale / 120s lost; field testing measured 15-25s honest gaps at
        // range, so stale must read as "normal at distance", not as failure.
        if (tgtAgeS > 120) {
            ringMode = RING_LOST;
            wasArrived = false;
        } else if (tgtAgeS > 30) {
            ringMode = RING_STALE;
            wasArrived = false;
        } else if (distM < 10.0) {
            if (!wasArrived) {                       // entering arrived: flash
                wasArrived = true;
                arrivedFlashUntil = millis() + 8000;
            }
            ringMode = (millis() < arrivedFlashUntil) ? RING_ARRIVED_FLASH
                                                      : RING_ARRIVED_STEADY;
        } else {
            wasArrived = false;
            ringMode = RING_NAVIGATING;
            // device.py blink_rate_for(): <50m fast, <200m medium, else slow.
            blinkHalfMs = distM < 50.0 ? 150 : distM < 200.0 ? 400 : 800;
        }
    }

    // BOOT button cancels the arrival flash (checked here at 1Hz is enough --
    // a human press spans several ticks).
    if (ringMode == RING_ARRIVED_FLASH && digitalRead(BUTTON_PIN) == LOW) {
        arrivedFlashUntil = 0;
        ringMode = RING_ARRIVED_STEADY;
    }
#else
    // Range-test builds: fixed waypoint, steady pointer (original behavior).
    navValid = g.valid && haveHeading;
    if (navValid) {
        distM     = distance(g.lat, g.lon, TARGET_LAT, TARGET_LON);
        targetBrg = bearing(g.lat, g.lon, TARGET_LAT, TARGET_LON);
        relBrg    = relativeBearing((float)targetBrg, lastHeading);
        pointerLed = ledForBearing(relBrg);
        ringMode   = RING_POINTING;
    } else {
        ringMode = RING_NO_FIX;
    }
#endif

    // Stack GPS, compass, sensor, and nav state on the OLED. Lines are kept
    // under 21 chars -- that's the 5x7 font's limit across 128px.
    char line[24];
    oledClear();

    RadioStats r = radioStats();
#if MESH_MODE
    // Mesh header: this node's id + own-beacon and processed-packet counts.
    snprintf(line, sizeof(line), "KANDI N%d T%lu R%lu",
             MESH_DEVICE_ID, (unsigned long)r.txCount, (unsigned long)r.rxCount);
#elif IS_SENDER
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

    // Heading + the calibration validation pair. M is |corrected field| --
    // THE post-calibration check: it should sit at ~25-65 uT and stay put at
    // any orientation. If it swings as you rotate, the offsets are wrong (or
    // something ferrous moved near the board). CAL/--- flags whether stored
    // offsets are loaded, i.e. whether the heading deserves any trust.
    // (Pitch/roll moved to serial-only -- the CMP line still has them.)
    float magNorm = 0;
    if (haveMag) {
        float cmx = m.mx - magOffsetX;
        float cmy = m.my - magOffsetY;
        float cmz = m.mz - magOffsetZ;
        magNorm = sqrtf(cmx * cmx + cmy * cmy + cmz * cmz);
    }
    // Heading shown as magnetic/true: the gap between them should be exactly
    // the 13.0 declination constant -- eyeball proof the correction is live.
    // A phone compass set to "true north" should agree with the SECOND number.
    if (c.ok) {
        snprintf(line, sizeof(line), "H%5.1f/%5.1f M%2.0f%s",
                 c.headingMag, c.heading, magNorm,
                 calStatus().calibrated ? "CAL" : "---");
    } else {
        snprintf(line, sizeof(line), "HDG --  (no sample)");
    }
    oledText(0, 4, line);

#if MESH_MODE
    // Page 5: the navigation state line -- THE field-test readout. State,
    // target member id, distance, absolute bearing, position age. The state
    // word must match what the ring is doing; that cross-check is the point.
    {
        const char *stateStr =
            ringMode == RING_NO_FIX         ? "NOFX" :
            ringMode == RING_LOST           ? "LOST" :
            ringMode == RING_STALE          ? "STAL" :
            ringMode == RING_NAVIGATING     ? "NAV"  :
                                              "ARRV";   // both arrived states
        if (haveTgt && g.valid) {
            snprintf(line, sizeof(line), "%s %u %4.0fm B%03.0f %lus",
                     stateStr, tgtId, distM, targetBrg, (unsigned long)tgtAgeS);
        } else {
            snprintf(line, sizeof(line), "%s  (no member yet)", stateStr);
        }
        oledText(0, 5, line);
    }

    if (!r.online) {
        oledText(0, 6, "RADIO OFFLINE");
    } else if (r.rxCount == 0) {
        oledText(0, 6, "RSSI ---   SNR ---");
        oledText(0, 7, "mesh: listening...");
    } else {
        snprintf(line, sizeof(line), "RSSI%5.0f  SNR%5.1f", r.rssi, r.snr);
        oledText(0, 6, line);

        // Sxx = who we heard last; hops left in that packet (3 = direct from
        // them, <3 = it came through a relay); our own relay count; age of
        // the last processed packet (roster age on page 5 can differ -- a
        // no-fix beacon refreshes this line but not the position).
        uint32_t age = (millis() - r.lastRxMillis) / 1000;
        snprintf(line, sizeof(line), "S%u h%u rly%lu age%3lus",
                 r.lastSender, r.lastHops, (unsigned long)r.relayCount,
                 (unsigned long)age);
        oledText(0, 7, line);
    }
#elif IS_SENDER
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
    // Receiver: bottom three pages are the range-test readout (raw sensors
    // still print to serial). Every line always renders -- placeholders
    // before the first packet -- so a quiet link shows a complete, obviously
    // waiting layout instead of a suspicious blank screen.
    if (!r.online) {
        oledText(0, 5, "RADIO OFFLINE");
    } else if (r.rxCount == 0) {
        oledText(0, 5, "D  ----  mx  ----");
        oledText(0, 6, "RSSI ---   SNR ---");
        oledText(0, 7, "RX 0  listening...");
    } else {
        // Distance to sender now + the farthest a packet has ever made it.
        // Max is held on screen even after the link dies -- that's the result
        // of the range test.
        if (r.distValid) {
            snprintf(line, sizeof(line), "D%6.0fm mx%6.0fm", r.distM, r.maxDistM);
        } else {
            snprintf(line, sizeof(line), "D NOFIX  mx%6.0fm", r.maxDistM);
        }
        oledText(0, 5, line);

        snprintf(line, sizeof(line), "RSSI%5.0f  SNR%5.1f", r.rssi, r.snr);
        oledText(0, 6, line);

        // Received count vs sender counter (gap = drops) and seconds since
        // the last packet -- the AGE is what separates "link alive" from
        // "display frozen on stale numbers" at the edge of range.
        uint32_t age = (millis() - r.lastRxMillis) / 1000;
        snprintf(line, sizeof(line), "RX %lu/%lu age %lus",
                 (unsigned long)r.rxCount, (unsigned long)(r.lastCounter + 1),
                 (unsigned long)age);
        oledText(0, 7, line);
    }
#endif

#if IS_SENDER && !MESH_MODE
    // Nav summary on the bottom page: distance, absolute bearing to target,
    // relative bearing, and which logical LED is lit. Cross-checks the ring.
    // (The receiver and mesh modes use page 7 for link liveness instead --
    // during radio testing the screen belongs to the radio. The waypoint
    // pointer still runs on the ring in every mode.)
    if (navValid) {
        snprintf(line, sizeof(line), "T%5.0fm B%03.0f R%03.0f L%d",
                 distM, targetBrg, relBrg, pointerLed);
    } else {
        snprintf(line, sizeof(line), "TGT --  %s",
                 g.valid ? "(no heading)" : "(no fix)");
    }
    oledText(0, 7, line);
#endif

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
        Serial.printf("MAG: %8.2f %8.2f %8.2f uT  |corrected| %.1f uT%s%s\n",
                      m.mx, m.my, m.mz, magNorm,
                      calStatus().calibrated ? " (cal)" : " (UNCAL)",
                      m.overflow ? "   <- OVERFLOW" : "");
    } else {
        Serial.printf("MAG: %s\n", magOk ? "read failed this tick" : "OFFLINE (chip ID failed at boot)");
    }

    // Heading is only as good as the hard-iron offsets, which are still zero --
    // expect it to be badly biased (and to barely move) until the figure-eight
    // calibration fills them in. The offsets are echoed so it's obvious from the
    // log alone which state you're looking at.
    if (c.ok) {
        Serial.printf("CMP: mag %6.1f  true %6.1f (decl %+.1f)  pitch %6.1f  roll %6.1f  "
                      "(offsets %.1f %.1f %.1f uT)\n",
                      c.headingMag, c.heading, MAGNETIC_DECLINATION_DEG,
                      c.pitchDeg, c.rollDeg,
                      magOffsetX, magOffsetY, magOffsetZ);
    } else {
        Serial.printf("CMP: no heading (%s%s)\n",
                      c.haveImu ? "" : "no accel sample ",
                      c.haveMag ? "" : "no mag sample");
    }

#if MESH_MODE
    if (haveTgt && g.valid) {
        static const char *modeNames[] = {"NO_FIX", "LOST", "NAVIGATING",
                                          "STALE", "ARRIVED_FLASH",
                                          "ARRIVED_STEADY", "POINTING"};
        Serial.printf("NAV: [%s] member %u  dist %.1fm  brg %.1f  rel %.1f  "
                      "age %lus  -> LED %d (pair %d+%d)\n",
                      modeNames[ringMode], tgtId, distM, targetBrg, relBrg,
                      (unsigned long)tgtAgeS, pointerLed,
                      pointerLed * 2, pointerLed * 2 + 1);
    } else {
        Serial.printf("NAV: %s\n",
                      !g.valid ? "no GPS fix" : "no member heard yet");
    }
#else
    if (navValid) {
        Serial.printf("NAV: dist %.1fm  brg %.1f  rel %.1f  -> LED %d (pair %d+%d)\n",
                      distM, targetBrg, relBrg, pointerLed,
                      pointerLed * 2, pointerLed * 2 + 1);
    } else {
        Serial.printf("NAV: searching (%s)\n",
                      g.valid ? "fix ok, waiting on heading" : "no GPS fix");
    }
#endif
}
