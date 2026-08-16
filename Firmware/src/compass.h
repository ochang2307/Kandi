#pragma once
#include "imu.h"
#include "mag.h"

// Tilt-compensated compass -- a direct port of CoreLogic/tilt_compensation.py.
//
// The problem it solves: a magnetometer reports Earth's field in the *device's*
// own axes. Tip the board and those axes rotate, so the raw mx/my swing wildly
// even though the device is still pointing the same way. Tilt compensation uses
// the accelerometer's gravity vector to work out how the board is tipped, then
// rotates the magnetic reading back to level before computing a heading.
//
// The three functions below are the verified Python versions translated
// term-for-term. They were bug-hunted carefully and round-trip to <0.5 degrees
// against synthetic data (CoreLogic/compass_test.py). Do not "simplify" them --
// in particular tiltCompensate() is deliberately asymmetric (Xh has three terms,
// Yh has two) and that is correct.
//
// Angle convention (project rule): heading is DEGREES, 0-360, 0 = north,
// increasing clockwise. Radians are internal only -- pitch and roll cross the
// pitchRoll() -> tiltCompensate() boundary in radians because that pair is one
// internal pipeline stage, not a public angle output.

// --- Hard-iron calibration offsets (microtesla) ---
// Subtracted from the raw magnetometer reading before tilt compensation.
// The board's own steel (18650 holder springs, components) adds a CONSTANT
// vector to every sample in the sensor frame -- measured at ~250uT on this
// board, against an Earth field of only 25-65uT. Left uncorrected it pins the
// heading to a fixed direction regardless of which way you turn.
//
// Zero for now, which is a no-op. The figure-eight calibration routine fills
// these in: rotating through all orientations traces a sphere whose CENTER is
// the offset.
extern float magOffsetX;
extern float magOffsetY;
extern float magOffsetZ;

// Board tilt derived from the gravity vector. RADIANS -- feeds straight into
// tiltCompensate(); use compassHeading() if you want degrees.
struct PitchRoll {
    float pitch;
    float roll;
};

// The magnetic field rotated back to horizontal. Unitless in the sense that
// only the ratio matters -- headingFrom() takes the arctangent of the pair.
struct HorizontalField {
    float Xh;
    float Yh;
};

// Accelerometer (any consistent unit; only direction matters) -> tilt, radians.
PitchRoll pitchRoll(float ax, float ay, float az);

// Magnetometer + tilt -> the level-frame horizontal field components.
HorizontalField tiltCompensate(float mx, float my, float mz,
                               float pitch, float roll);

// Level-frame components -> heading in degrees, 0-360, 0 = north.
float headingFrom(float Xh, float Yh);

// --- Magnetic declination ---
// The magnetometer measures against MAGNETIC north; bearing() in
// navigation.cpp works in TRUE north (great-circle math on GPS coordinates).
// Mixing the two references puts a constant angular error on every LED
// indication, so compassHeading() adds this correction as its final step.
//
// LOCATION-SPECIFIC: 13.0 deg East is Saratoga / SF Bay Area. East
// declination is positive (magnetic north sits east of true north here, so
// true = magnetic + declination). CHANGE THIS if the device is tested
// anywhere else -- and note declination also drifts a fraction of a degree
// per year as the pole wanders. A production device would derive it from the
// GPS fix via a lookup (the WMM model, or a coarse per-region table) instead
// of hardcoding.
static const float MAGNETIC_DECLINATION_DEG = 13.0f;

// --- Heading mirror toggle ---
// Set to 1 if the OLED heading DECREASES when you rotate the board clockwise
// (viewed from above). That symptom means the magnetometer's physical Y axis
// is flipped relative to the frame the verified math assumes -- magnitude
// and calibration are unaffected (|M| can't catch a mirror), but every
// heading comes out reflected about north.
//
// Applied at the output boundary in compassHeading() as (360 - heading);
// pitchRoll/tiltCompensate/headingFrom stay byte-identical to the verified
// Python. NOTE: an output-side mirror is exact when flat and approximate
// under heavy tilt -- if headings misbehave only at extreme tilt after
// enabling this, the flip belongs on the mag driver's Y axis instead.
#define COMPASS_MIRROR_HEADING 0

// One full pipeline pass: sample both sensors, apply the hard-iron offsets,
// and return the heading. Non-blocking -- if either sensor has no fresh sample
// this returns with ok = false rather than waiting (the GPS UART overruns if
// the loop stalls past ~250ms).
//
// The raw samples come back too, so callers that also want to display accel/mag
// don't have to read the sensors a second time. That matters: imuRead() gates on
// a data-ready flag, so an immediate second call would usually return nothing.
struct CompassData {
    bool  ok;          // true only when both sensors gave a fresh sample
    bool  haveImu;
    bool  haveMag;
    float heading;     // TRUE-north heading, declination applied -- use this
                       //   against bearing(). degrees 0-360 (valid only if ok)
    float headingMag;  // MAGNETIC heading, straight from headingFrom() --
                       //   display/diagnostics only  (valid only if ok)
    float pitchDeg;    // degrees, for display/diagnostics (valid only if ok)
    float rollDeg;     // degrees, for display/diagnostics (valid only if ok)
    ImuData imu;       // the accel/gyro sample used
    MagData mag;       // the magnetic sample used -- RAW, before offsets
};

CompassData compassHeading();
