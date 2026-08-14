#pragma once
#include <stdint.h>
#include <stddef.h>

// Group roster: last-known position per mesh member, fed by radio.cpp on
// every processed position packet that carries a valid fix. This is the
// hardware version of device.py's `positions` dict -- fixed-size array, no
// dynamic allocation, one slot per sender id.
//
// v1 navigation targets the MOST RECENTLY HEARD member (single-target);
// per-member colors / group view come later with bonding.

static const size_t ROSTER_SLOTS = 8;

struct RosterEntry {
    uint8_t  id;            // mesh sender id
    int32_t  latE7;         // last-known position, degrees * 1e7
    int32_t  lonE7;
    uint32_t lastUpdateMs;  // millis() when that position arrived. AGE drives
                            //   the navigating/stale/lost display states.
    bool     used;
};

// Record a position report. Only call with real fixes -- a no-fix beacon
// keeps the member's previous position (and its age keeps growing, which is
// exactly what the stale display should reflect).
void rosterUpdate(uint8_t id, int32_t latE7, int32_t lonE7, uint32_t nowMs);

// The member with the newest position, or false if none heard yet.
bool rosterMostRecent(RosterEntry &out);
