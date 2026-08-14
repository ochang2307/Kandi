#include <Arduino.h>
#include <math.h>
#include "compass.h"

// Hard-iron offsets, microtesla. Zero = no correction applied yet.
float magOffsetX = 0.0f;
float magOffsetY = 0.0f;
float magOffsetZ = 0.0f; 

// --- Direct translations of CoreLogic/tilt_compensation.py ---
//
// Python used doubles; these use float because the ESP32-S3's FPU is
// single-precision (doubles are emulated in software). float carries ~7
// significant digits, which is four orders of magnitude finer than the <0.5
// degree tolerance these formulas were verified to. The terms themselves are
// unchanged: same operands, same order, same signs.

// Python:
//   pitch = math.atan2(-ax, math.sqrt((ay*ay)+(az*az)))
//   roll  = math.atan2(ay, az)
//
// Gravity always points down in the world, so whichever way it leans in the
// device's axes tells you how the device is tilted. Pitch compares the forward
// axis against the magnitude of the other two; roll is just the left-vs-up
// ratio. atan2 (rather than atan) is what keeps both correct through all four
// quadrants instead of folding at +/-90 degrees.
PitchRoll pitchRoll(float ax, float ay, float az) {
    PitchRoll out;
    out.pitch = atan2f(-ax, sqrtf((ay * ay) + (az * az)));
    out.roll  = atan2f(ay, az);
    return out;
}

// Python:
//   Xh = (mx * cos(pitch)
//         + my * sin(roll) * sin(pitch)
//         + mz * cos(roll) * sin(pitch))
//   Yh = (my * cos(roll)
//         - mz * sin(roll))
//
// This rotates the measured field back to the horizontal plane. The asymmetry
// is real and load-bearing, not a missing line: Xh is the forward axis, which
// picks up a contribution from all three sensor axes once the board is pitched.
// Yh is the left axis, which pitch rotates *within* -- so it only ever mixes my
// and mz. Adding a "matching" third term to Yh would break the round trip.
HorizontalField tiltCompensate(float mx, float my, float mz,
                               float pitch, float roll) {
    // Hoisted so each trig call happens once instead of two or three times.
    // Purely a cost saving -- the expressions below are otherwise identical to
    // the Python, term for term.
    const float cosPitch = cosf(pitch);
    const float sinPitch = sinf(pitch);
    const float cosRoll  = cosf(roll);
    const float sinRoll  = sinf(roll);

    HorizontalField out;
    out.Xh = (mx * cosPitch
              + my * sinRoll * sinPitch
              + mz * cosRoll * sinPitch);
    out.Yh = (my * cosRoll
              - mz * sinRoll);
    return out;
}

// Python:
//   heading = math.atan2(-Yh, Xh)
//   heading = math.degrees(heading)
//   heading = (heading + 360) % 360
//
// The negated Yh is what makes the result clockwise-from-north (the project
// convention) rather than the counter-clockwise sense atan2 gives by default.
// atan2f returns (-180, 180]; adding 360 before the modulo is what maps the
// negative half onto 180-360. fmodf matches Python's % exactly here because the
// operand is always positive by that point.
float headingFrom(float Xh, float Yh) {
    float heading = atan2f(-Yh, Xh);
    heading = heading * RAD_TO_DEG;
    heading = fmodf(heading + 360.0f, 360.0f);
    return heading;
}

CompassData compassHeading() {
    CompassData out;
    out.ok = false;
    out.heading = 0.0f;
    out.headingMag = 0.0f;
    out.pitchDeg = 0.0f;
    out.rollDeg = 0.0f;

    out.haveImu = imuRead(out.imu);
    out.haveMag = magRead(out.mag);

    if (!out.haveImu || !out.haveMag) {
        return out;
    }

    // Hard-iron correction first: the offsets live in the SENSOR frame, so they
    // have to come off before any rotation is applied. Subtracting after tilt
    // compensation would be subtracting a fixed vector from a rotating one.
    const float mx = out.mag.mx - magOffsetX;
    const float my = out.mag.my - magOffsetY;
    const float mz = out.mag.mz - magOffsetZ;

    PitchRoll pr = pitchRoll(out.imu.ax, out.imu.ay, out.imu.az);
    HorizontalField h = tiltCompensate(mx, my, mz, pr.pitch, pr.roll);

    // Declination correction happens HERE, at the consumption boundary --
    // headingFrom() stays purely magnetic and byte-identical to the verified
    // Python. East declination positive: true = magnetic + declination,
    // re-normalized to the project's 0-360 convention.
    out.headingMag = headingFrom(h.Xh, h.Yh);
    out.heading    = fmodf(out.headingMag + MAGNETIC_DECLINATION_DEG + 360.0f,
                           360.0f);
    out.pitchDeg   = pr.pitch * RAD_TO_DEG;
    out.rollDeg    = pr.roll * RAD_TO_DEG;
    out.ok = true;
    return out;
}
