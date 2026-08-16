#pragma once
#include <stdint.h>

// Thin driver for the on-board QMC6310N 3-axis magnetometer.
//
// This is the 0x3C squatter documented in CLAUDE.md: it sits on the SAME I2C
// bus as the OLED (SDA 17 / SCL 18) and answers at the address most display
// libraries assume is theirs. The OLED is at 0x3D. Both are already brought up
// elsewhere; this driver only touches the magnetometer.
//
// The chip does NOT measure by default -- it boots in suspend mode and returns
// stale/zero data until explicitly put into continuous-measurement mode. That
// configuration happens in magBegin().

// --- Raw debug ---
// Set to 1 to also print unscaled LSB counts and the overflow flag. Same
// diagnostic purpose as IMU_DEBUG_RAW: distinguishes a sensor that never
// answered from one that answered but is sitting in suspend or saturating.
#define MAG_DEBUG_RAW 0

// --- Axis remap: physical mag frame -> device frame ---
// The compass math requires the mag and accel to agree on one frame (X fwd,
// Y right, Z up -- the accel defines it: +Z reads ~+9.8 flat). The two chips
// are different packages in different orientations on the PCB, so agreement
// is luck, not physics. A mismatched axis is INVISIBLE flat (mz drops out of
// tilt compensation at zero pitch/roll) and wrecks headings under tilt.
//
// Diagnosis (board flat, calibrated): corrected mz should read ~-40 uT here
// (Earth's field points steeply DOWN; down = negative on an up-axis).
// Reading ~+40 instead = Z flipped -> set MAG_FLIP_Z 1.
//
// !! After changing any flip: stored calibration offsets were captured in
// !! the old frame -- run 'calclear' + 'cal' again on every board.
// Order of operations: swap first, then flips.
#define MAG_SWAP_XY 0
#define MAG_FLIP_X 0
#define MAG_FLIP_Y 0
#define MAG_FLIP_Z 1   // confirmed 2026-08: flat corrected mz read +40 (field
                       // points DOWN here) -> mag Z was inverted vs accel Z

struct MagData {
    bool  ok;              // true if this sample was actually read
    float mx, my, mz;      // magnetic field, microtesla
    bool  overflow;        // true if the field exceeded the configured range
};

// Configure the sensor into continuous-measurement mode. Returns false if the
// chip ID read fails (SensorLib verifies it inside begin()).
bool magBegin();

// Fetch one sample. Returns false if the read failed; `out.ok` mirrors it.
bool magRead(MagData &out);
