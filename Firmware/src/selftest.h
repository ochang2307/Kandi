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

// Mesh-protocol self-tests (mesh_test.cpp): the network.py flooding scenarios
// (line relay, loop dedup, hop-limit expiry) run against N in-memory MeshNode
// instances, plus tests for the embedded parts the Python sim never had --
// seen-cache expiry/eviction, the SNR->delay map, and duplicate-cancels-relay.
// Same golden-value philosophy, same PASS/FAIL output.
bool runMeshSelfTests();
