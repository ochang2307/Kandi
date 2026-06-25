from navigation import distance, bearing
from LEDLogic import relative_bearing, led_for_bearing

# --- relative_bearing tests ---
print("Target N, facing N:", relative_bearing(0, 0),   "(expect 0)")
print("Target N, facing E:", relative_bearing(0, 90),  "(expect 270)")
print("Target E, facing E:", relative_bearing(90, 90), "(expect 0)")

# --- led_for_bearing tests (0-7, top LED = 0) ---
print("0 deg   ->", led_for_bearing(0),   "(expect LED 0)")
print("45 deg  ->", led_for_bearing(45),  "(expect LED 1)")
print("90 deg  ->", led_for_bearing(90),  "(expect LED 2)")
print("44 deg  ->", led_for_bearing(44),  "(expect LED 1 - rounds up)")
print("46 deg  ->", led_for_bearing(46),  "(expect LED 1)")
print("70 deg  ->", led_for_bearing(70),  "(expect LED 2)")
print("359 deg ->", led_for_bearing(359), "(expect LED 0 - wraps)")

# --- combined: the whole point of the device ---
# friend due north, but wrist twisted to face east
rb = relative_bearing(0, 90)          # -> 270
print("Friend N, facing E -> LED", led_for_bearing(rb), "(expect 6, left side)")