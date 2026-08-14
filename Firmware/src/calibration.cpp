#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "calibration.h"
#include "compass.h"   // magOffsetX/Y/Z -- compass.cpp subtracts these already
#include "mag.h"

static const uint32_t CAPTURE_MS    = 30000;
static const float    SPREAD_GOOD   = 100.0f;  // ~2x Earth's field: axis covered
static const float    SPREAD_POOR   = 50.0f;   // below this the axis was barely moved

// NVS: namespace "kandi", one float per axis + a validity flag. Preferences
// wraps the ESP32 NVS partition -- survives reboot and reflash (it lives in
// its own flash partition, untouched by `pio run -t upload`).
static Preferences prefs;

static bool     captureActive = false;
static bool     haveCal       = false;
static uint32_t captureStart  = 0;
static float    mnv[3], mxv[3];        // running min/max per axis, uT
static uint32_t sampleCount   = 0;

// Serial command line assembly (USB CDC): accumulate until newline.
static char   cmdBuf[24];
static size_t cmdLen = 0;

static void startCapture() {
    if (captureActive) {
        Serial.println("CAL: capture already running");
        return;
    }
    for (int i = 0; i < 3; i++) {
        mnv[i] = 1e9f;
        mxv[i] = -1e9f;
    }
    sampleCount   = 0;
    captureStart  = millis();
    captureActive = true;
    Serial.println("CAL: capturing for 30s -- rotate through EVERYTHING:");
    Serial.println("CAL: figure-eights, then flip the board and repeat.");
    Serial.println("CAL: watch the OLED -- every axis needs ~100 uT spread.");
}

static void applyOffsets(float ox, float oy, float oz) {
    magOffsetX = ox;
    magOffsetY = oy;
    magOffsetZ = oz;
}

static void finishCapture() {
    captureActive = false;

    float ox = (mxv[0] + mnv[0]) * 0.5f;
    float oy = (mxv[1] + mnv[1]) * 0.5f;
    float oz = (mxv[2] + mnv[2]) * 0.5f;
    float sx = mxv[0] - mnv[0];
    float sy = mxv[1] - mnv[1];
    float sz = mxv[2] - mnv[2];

    Serial.printf("CAL: done. %lu samples\n", (unsigned long)sampleCount);
    Serial.printf("CAL: spread   X %.1f  Y %.1f  Z %.1f uT (want >= %.0f each)\n",
                  sx, sy, sz, SPREAD_GOOD);
    Serial.printf("CAL: offsets  X %.1f  Y %.1f  Z %.1f uT\n", ox, oy, oz);

    // Radius sanity: half the average spread should be near Earth's ~25-65 uT.
    float radius = (sx + sy + sz) / 6.0f;
    Serial.printf("CAL: implied field radius ~%.1f uT (Earth: 25-65)\n", radius);

    if (sx < SPREAD_POOR || sy < SPREAD_POOR || sz < SPREAD_POOR) {
        Serial.println("CAL: WARNING -- an axis has <50 uT spread. That axis never");
        Serial.println("CAL: saw both field extremes; the offset for it is a guess.");
        Serial.println("CAL: Stored anyway. Redo with fuller rotation ('cal').");
    }

    prefs.begin("kandi", false);
    prefs.putFloat("mox", ox);
    prefs.putFloat("moy", oy);
    prefs.putFloat("moz", oz);
    prefs.putBool("mcal", true);
    prefs.end();

    applyOffsets(ox, oy, oz);
    haveCal = true;
    Serial.println("CAL: stored to NVS and applied. |M| on the OLED should now");
    Serial.println("CAL: hold ~25-65 uT at ANY orientation -- that's the check.");
}

static void clearCal() {
    prefs.begin("kandi", false);
    prefs.clear();
    prefs.end();
    applyOffsets(0, 0, 0);
    haveCal = false;
    Serial.println("CAL: cleared. Offsets zeroed; heading is UNTRUSTWORTHY until 'cal'.");
}

static void handleCommand(const char *cmd) {
    if (strcmp(cmd, "cal") == 0) {
        startCapture();
    } else if (strcmp(cmd, "calclear") == 0) {
        clearCal();
    } else if (cmdLen > 0) {
        Serial.printf("CAL: unknown command '%s' (know: cal, calclear)\n", cmd);
    }
}

void calBegin() {
    prefs.begin("kandi", true);   // read-only
    bool stored = prefs.getBool("mcal", false);
    if (stored) {
        float ox = prefs.getFloat("mox", 0);
        float oy = prefs.getFloat("moy", 0);
        float oz = prefs.getFloat("moz", 0);
        applyOffsets(ox, oy, oz);
        haveCal = true;
        Serial.printf("CAL: loaded offsets from NVS: %.1f %.1f %.1f uT\n", ox, oy, oz);
    } else {
        Serial.println("CAL: no stored calibration -- heading is biased until you");
        Serial.println("CAL: run 'cal' (type it into this monitor + enter).");
    }
    prefs.end();
}

void calTick() {
    // --- serial command polling, every pass ---
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\n' || ch == '\r') {
            if (cmdLen > 0) {
                cmdBuf[cmdLen] = '\0';
                handleCommand(cmdBuf);
                cmdLen = 0;
            }
        } else if (cmdLen < sizeof(cmdBuf) - 1) {
            cmdBuf[cmdLen++] = ch;
        }
    }

    if (!captureActive) return;

    // --- capture sampling, every pass ---
    // magRead() returns the chip's latest continuous-mode sample (100Hz ODR),
    // RAW -- offsets only get subtracted inside compassHeading(), so a capture
    // is never polluted by a previous calibration.
    MagData m;
    if (magRead(m) && !m.overflow) {
        if (m.mx < mnv[0]) mnv[0] = m.mx;
        if (m.mx > mxv[0]) mxv[0] = m.mx;
        if (m.my < mnv[1]) mnv[1] = m.my;
        if (m.my > mxv[1]) mxv[1] = m.my;
        if (m.mz < mnv[2]) mnv[2] = m.mz;
        if (m.mz > mxv[2]) mxv[2] = m.mz;
        sampleCount++;
    }

    if (millis() - captureStart >= CAPTURE_MS) {
        finishCapture();
    }
}

CalStatus calStatus() {
    CalStatus s;
    s.active     = captureActive;
    s.calibrated = haveCal;
    s.msRemaining = 0;
    s.spreadX = s.spreadY = s.spreadZ = 0;
    if (captureActive) {
        uint32_t elapsed = millis() - captureStart;
        s.msRemaining = elapsed < CAPTURE_MS ? CAPTURE_MS - elapsed : 0;
        if (sampleCount > 0) {
            s.spreadX = mxv[0] - mnv[0];
            s.spreadY = mxv[1] - mnv[1];
            s.spreadZ = mxv[2] - mnv[2];
        }
    }
    return s;
}
