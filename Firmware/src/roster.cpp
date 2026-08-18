#include <string.h>
#include "roster.h"

static RosterEntry roster[ROSTER_SLOTS];

static void fillStats(RosterEntry &e, int32_t latE7, int32_t lonE7,
                      uint8_t hops, float rssi, float snr, uint32_t nowMs) {
    e.latE7 = latE7;
    e.lonE7 = lonE7;
    e.lastUpdateMs = nowMs;
    e.lastHops = hops;
    e.lastRssi = rssi;
    e.lastSnr  = snr;
    if (hops == 0) e.directCount++; else e.relayCount++;
}

RosterResult rosterUpdate(uint8_t id, int32_t latE7, int32_t lonE7,
                          uint8_t hopsTraveled, float rssi, float snr,
                          uint32_t nowMs) {
    // Existing member: refresh in place.
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (roster[i].used && roster[i].id == id) {
            fillStats(roster[i], latE7, lonE7, hopsTraveled, rssi, snr, nowMs);
            return ROSTER_UPDATED;
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
    memset(&roster[victim], 0, sizeof(roster[victim]));
    roster[victim].id = id;
    roster[victim].used = true;
    fillStats(roster[victim], latE7, lonE7, hopsTraveled, rssi, snr, nowMs);
    return ROSTER_NEW;
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

bool rosterGet(uint8_t id, RosterEntry &out) {
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (roster[i].used && roster[i].id == id) {
            out = roster[i];
            return true;
        }
    }
    return false;
}

size_t rosterCount() {
    size_t n = 0;
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (roster[i].used) n++;
    }
    return n;
}

bool rosterByIndex(size_t idx, RosterEntry &out) {
    size_t n = 0;
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (!roster[i].used) continue;
        if (n == idx) { out = roster[i]; return true; }
        n++;
    }
    return false;
}

uint8_t rosterNextId(uint8_t afterId) {
    // Collect used ids in slot order, find afterId, return the next (wrap).
    uint8_t ids[ROSTER_SLOTS];
    size_t n = 0;
    for (size_t i = 0; i < ROSTER_SLOTS; i++) {
        if (roster[i].used) ids[n++] = roster[i].id;
    }
    if (n == 0) return 0;
    for (size_t i = 0; i < n; i++) {
        if (ids[i] == afterId) return ids[(i + 1) % n];
    }
    return ids[0];   // afterId unknown (or 0): start at the first member
}
