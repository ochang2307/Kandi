#pragma once

// Great-circle navigation -- a direct port of CoreLogic/navigation.py.
//
// Both functions were verified against real-world coordinates (Google Maps
// distance to Taipei 101, cardinal-direction sanity checks) before the port,
// and runSelfTests() re-checks the C++ against those same golden values at
// every boot. Do not swap in a different distance formula.
//
// Angle convention (project rule): bearing() returns DEGREES, 0-360,
// 0 = north, increasing clockwise. Radians are internal only.
//
// These use double, not float, on purpose: a float carries ~7 significant
// digits, and a latitude like 25.033264 already spends all of them left of
// the part that matters -- float coordinates quantize position to ~1-2m and
// make short distances/bearings garbage. The S3 emulates double in software,
// but this math runs once per position update, not per loop pass.

// Distance between two GPS coordinates in METERS (haversine, R = 6371000).
double distance(double lat1, double lon1, double lat2, double lon2);

// Initial great-circle bearing FROM point 1 TO point 2.
// Degrees, 0-360, 0 = north, clockwise.
double bearing(double lat1, double lon1, double lat2, double lon2);
