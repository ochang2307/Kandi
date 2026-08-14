#include <string.h>
#include "roster.h"

static RosterEntry roster[ROSTER_SLOTS];

void rosterUpdate(uint8_t id, int32_t latE7, int32_t lonE7, uint32_t nowMs) {
    // Existing member: refresh in place.
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (roster[i].used && roster[i].id == id) {
            roster[i].latE7 = latE7;
            roster[i].lonE7 = lonE7;
            roster[i].lastUpdateMs = nowMs;
            return;
        }
    }
    // New member: first free slot, else evict the longest-silent one (a full
    // roster of 8 with a stranger arriving means the quietest entry is the
    // least useful thing we know).
    size_t victim = 0;
    uint32_t oldest = 0;
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (!roster[i].used) { victim = i; break; }
        uint32_t age = nowMs - roster[i].lastUpdateMs;
        if (age >= oldest) { oldest = age; victim = i; }
    }
    roster[victim].id = id;
    roster[victim].latE7 = latE7;
    roster[victim].lonE7 = lonE7;
    roster[victim].lastUpdateMs = nowMs;
    roster[victim].used = true;
}

bool rosterMostRecent(RosterEntry &out) {
    bool found = false;
    uint32_t newest = 0;
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (!roster[i].used) continue;
        if (!found || (int32_t)(roster[i].lastUpdateMs - newest) >= 0) {
            newest = roster[i].lastUpdateMs;
            out = roster[i];
            found = true;
        }
    }
    return found;
}
