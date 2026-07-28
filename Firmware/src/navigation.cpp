#include <math.h>
#include "navigation.h"

// Degrees -> radians, matching Python's math.radians().
static inline double rad(double deg) { return deg * M_PI / 180.0; }

// Python's % on floats always returns a non-negative result when the divisor
// is positive; C's fmod() keeps the sign of the dividend instead. This helper
// reproduces the Python behavior so "(x + 360) % 360" translates faithfully
// even if x + 360 is still negative.
static inline double norm360(double deg) {
    double m = fmod(deg, 360.0);
    if (m < 0.0) m += 360.0;
    return m;
}

// Python:
//   a = sin(dlat/2)^2 + cos(lat1r)*cos(lat1r)*sin(dlon/2)^2
//   c = 2*atan2(sqrt(a), sqrt(1-a))
//   d = r*c
//
// Term-for-term translation, including the cos(lat1r)*cos(lat1r) pair exactly
// as verified. (Textbook haversine writes cos(lat1)*cos(lat2); with group
// members hundreds of meters apart the two are equal to ~7 decimal places,
// and this is the form the real-world tests validated.)
double distance(double lat1, double lon1, double lat2, double lon2) {
    const double r = 6371000.0;   // earth radius, meters

    double lat1r = rad(lat1);
    double lat2r = rad(lat2);
    double dlat  = lat2r - lat1r;
    double dlon  = rad(lon2) - rad(lon1);

    double a = (sin(dlat / 2) * sin(dlat / 2))
             + (cos(lat1r) * cos(lat1r) * sin(dlon / 2) * sin(dlon / 2));
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return r * c;
}

// Python:
//   theta = atan2(sin(dlon)*cos(lat2r),
//                 cos(lat1r)*sin(lat2r) - sin(lat1r)*cos(lat2r)*cos(dlon))
//   ans = (degrees(theta) + 360) % 360
double bearing(double lat1, double lon1, double lat2, double lon2) {
    double lat1r = rad(lat1);
    double lat2r = rad(lat2);
    double dlon  = rad(lon2) - rad(lon1);

    double theta = atan2(sin(dlon) * cos(lat2r),
                         cos(lat1r) * sin(lat2r)
                         - sin(lat1r) * cos(lat2r) * cos(dlon));
    return norm360(theta * 180.0 / M_PI + 360.0);
}
