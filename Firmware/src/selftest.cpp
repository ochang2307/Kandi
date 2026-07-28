#include <Arduino.h>
#include <math.h>
#include "selftest.h"
#include "navigation.h"
#include "ledlogic.h"

// Test cases ported from CoreLogic/nav_test.py and nav_bearing_test.py.
// Expected values are the EXACT outputs of the verified Python (captured by
// running it), not re-derived -- the point is to detect any divergence
// between this port and CoreLogic/, not to re-verify the math itself.

static int passed = 0;
static int failed = 0;

static void report(const char *name, bool ok, double got, double expect) {
    if (ok) passed++; else failed++;
    Serial.printf("[%s] %-28s got %11.6f  expect %11.6f\n",
                  ok ? "PASS" : "FAIL", name, got, expect);
}

// Plain closeness check, for distances and LED indices.
static void checkNear(const char *name, double got, double expect, double tol) {
    report(name, fabs(got - expect) <= tol, got, expect);
}

// Angle closeness with wraparound: 359.999 and 0.0 must count as equal, or
// the "due north" case fails on a last-decimal difference.
static void checkAngle(const char *name, double got, double expect, double tol) {
    double diff = fabs(fmod(got - expect + 540.0, 360.0) - 180.0);
    report(name, diff <= tol, got, expect);
}

bool runSelfTests() {
    passed = failed = 0;
    Serial.println("--- core logic self-tests (vs Python golden values) ---");

    // Reference point used throughout nav_test.py (Taipei City Hall area).
    const double SLAT = 25.0330, SLON = 121.5654;

    // --- distance() ---
    checkNear("dist: same point",
              distance(SLAT, SLON, SLAT, SLON), 0.0, 0.01);
    // Known pair verified against Google Maps (~2090m there; haversine gives
    // 2070.5 -- gmaps measures a walking-ish path, this is the straight line).
    checkNear("dist: to Taipei 101",
              distance(25.033264086415144, 121.56486801276817,
                       25.04448385356689, 121.54846663457496),
              2070.504146854042, 0.01);

    // --- bearing() cardinals: +/-0.01 deg lat/lon steps from the reference ---
    checkAngle("brg: due north",
               bearing(SLAT, SLON, SLAT + 0.01, SLON), 0.0, 0.001);
    checkAngle("brg: due east",
               bearing(SLAT, SLON, SLAT, SLON + 0.01), 89.99788429906016, 0.001);
    checkAngle("brg: due south",
               bearing(SLAT, SLON, SLAT - 0.01, SLON), 180.0, 0.001);
    checkAngle("brg: due west",
               bearing(SLAT, SLON, SLAT, SLON - 0.01), 270.00211570093984, 0.001);
    // Not exactly 45: east-west degrees are shorter than north-south ones at
    // latitude 25, so an equal-degree step isn't an equal-distance step.
    checkAngle("brg: northeast",
               bearing(SLAT, SLON, SLAT + 0.01, SLON + 0.01), 42.175320546747, 0.001);
    checkAngle("brg: to Taipei 101 (NW-ish)",
               bearing(SLAT, SLON, 25.04448385356689, 121.54846663457496),
               306.8193172820985, 0.001);

    // --- relativeBearing() ---
    checkNear("rel: target N, facing N", relativeBearing(0, 0), 0.0, 1e-4);
    checkNear("rel: target N, facing E", relativeBearing(0, 90), 270.0, 1e-4);
    checkNear("rel: target E, facing E", relativeBearing(90, 90), 0.0, 1e-4);

    // --- ledForBearing() boundary cases ---
    checkNear("led: 0 deg -> 0",    ledForBearing(0),   0, 0);
    checkNear("led: 45 deg -> 1",   ledForBearing(45),  1, 0);
    checkNear("led: 90 deg -> 2",   ledForBearing(90),  2, 0);
    checkNear("led: 44 deg -> 1",   ledForBearing(44),  1, 0);
    checkNear("led: 46 deg -> 1",   ledForBearing(46),  1, 0);
    checkNear("led: 70 deg -> 2",   ledForBearing(70),  2, 0);
    checkNear("led: 359 deg -> 0 (wrap)", ledForBearing(359), 0, 0);

    // --- combined: the whole point of the device ---
    // Friend due north, wrist twisted to face east -> LED 6 (left side).
    checkNear("combined: friend N, facing E",
              ledForBearing(relativeBearing(0, 90)), 6, 0);

    Serial.printf("--- self-tests: %d passed, %d FAILED ---\n", passed, failed);
    return failed == 0;
}
