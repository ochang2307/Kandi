#pragma once

// Boot-time self-tests for the ported core logic (navigation + LED logic).
// Each case's expected value is a GOLDEN value
// captured from the verified Python running the same inputs -- so a FAIL here
// means the C++ has diverged from CoreLogic/, and it shows up on the bench at
// boot instead of as a wrong-way LED in a field test.
//
// Costs a few ms of setup() time and some flash. Cheap insurance; leave it in
// at least until the field demo.

// Runs every case, prints one PASS/FAIL line each to serial, then a summary.
// Returns true only if every case passed.
bool runSelfTests();
