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
