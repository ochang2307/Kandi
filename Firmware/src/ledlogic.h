#pragma once

// Bearing -> LED sector logic. Direct port of CoreLogic/LEDLogic.py.
//
// The logical model is 8 sectors (indices 0-7, 0 = top/12-o'clock, clockwise),
// per the design doc. The physical ring is 16 WS2812s: map each logical index
// to its LED pair at the DISPLAY layer (in main.cpp, when pixels get written),
// not in here -- these functions stay hardware-free, same as the Python.
//
// Angle convention: degrees, 0-360, 0 = north, clockwise, at every boundary.

// How far clockwise the target sits from where the device is pointing.
// (target bearing is world-referenced, device heading comes from the compass;
// the difference is what the wearer actually needs.)
float relativeBearing(float targetBearing, float deviceHeading);

// Which of the 8 logical LEDs points at the target. Each sector is 45 degrees
// wide, centered on its LED: 0 deg -> 0 (top), 90 -> 2 (right), 359 -> 0.
int ledForBearing(float relativeBrg);
