#pragma once
#include <stdint.h>

// Hard-iron calibration for the QMC6310N (milestone 3's unfinished half).
//
// Physics: every raw reading is Earth's field (a ~50 uT vector that rotates
// with the board) PLUS a constant hard-iron offset (~250 uT on this board --
// the 18650 springs etc, fixed in the SENSOR frame). Rotate the board through
// all orientations and the raw readings trace a sphere whose CENTER is the
// offset. v1 finds the center per axis as (max + min) / 2 -- crude but
// unbiased if coverage is good, and good coverage is exactly what the live
// spread display is for.
//
// Deliberately NOT corrected in v1: soft-iron distortion (nearby ferrous
// material re-shaping the field), which stretches the sphere into a tilted
// ellipsoid. Fixing that takes a least-squares ellipsoid fit and a 3x3
// correction matrix -- postponed until a constant-magnitude check (the |M|
// readout on the OLED) proves the sphere is actually distorted enough to
// matter.
//
// Serial commands (type into the pio monitor):
//   cal       start a 30s capture -- then rotate the board through EVERYTHING:
//             figure-eights plus full flips, so every axis sees both field
//             extremes. Watch the per-axis spread on the OLED; each needs
//             ~100 uT (2x Earth's field) before the capture means anything.
//   calclear  wipe stored calibration (after hardware changes near the mag).
//
// Offsets persist in NVS (Preferences, namespace "kandi") and are loaded into
// compass.cpp's magOffsetX/Y/Z at boot -- compass.cpp itself is untouched; it
// already subtracts those exact globals.

struct CalStatus {
    bool     active;       // capture running right now
    bool     calibrated;   // offsets loaded (from NVS or a finished capture)
    uint32_t msRemaining;  // capture countdown, 0 when idle
    float    spreadX;      // max-min seen so far this capture, uT
    float    spreadY;
    float    spreadZ;
};

// Load stored offsets from NVS into magOffsetX/Y/Z. Call once in setup(),
// after magBegin().
void calBegin();

// Poll serial for commands; while a capture is active, sample the mag and
// update min/max. Call EVERY loop pass -- it never blocks (one ~0.2ms I2C
// read per pass at most), so the GPS budget is untouched.
void calTick();

CalStatus calStatus();
