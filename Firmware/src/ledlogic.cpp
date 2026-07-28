#include <math.h>
#include "ledlogic.h"

// Same Python-% helper as navigation.cpp: fmodf keeps the dividend's sign,
// Python's % never goes negative. The +360 in the callers already covers any
// realistic input, but this makes the guarantee unconditional.
static inline float norm360f(float deg) {
    float m = fmodf(deg, 360.0f);
    if (m < 0.0f) m += 360.0f;
    return m;
}

// Python: relbear = (target_bearing - device_heading + 360) % 360
float relativeBearing(float targetBearing, float deviceHeading) {
    return norm360f(targetBearing - deviceHeading + 360.0f);
}

// Python: led_index = round(relative_brg / 45) % 8
//
// roundf() rounds half AWAY from zero where Python's round() rounds half to
// even -- they only disagree when relative_brg is an exact multiple of 22.5,
// a knife-edge no real sensor reading lands on. All verified test cases
// (44 -> 1, 46 -> 1, 359 -> 0) are unaffected.
//
// The % 8 is what wraps sector 8 back to 0: 337.5-360 rounds to 8, which is
// the top LED again.
int ledForBearing(float relativeBrg) {
    return (int)roundf(relativeBrg / 45.0f) % 8;
}
