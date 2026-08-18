#pragma once
#include <stdint.h>
#include <stddef.h>

// Group roster: last-known position + link stats per mesh member, fed by
// radio.cpp. This is the hardware version of device.py's `positions` dict --
// fixed-size array, no dynamic allocation, one slot per sender id.
//
// Entries are keyed on the packet's ORIGINATOR (sender_id), which the relay
// path never mutates (verified: meshScheduleRelay copies the packet and
// decrements only hopLimit; meshRelayDue stamps only txNode; pinned by a
// boot self-test in mesh_test.cpp). So a position is always attributed to
// whoever measured it, no matter how many hops it rode.

static const size_t ROSTER_SLOTS = 8;

struct RosterEntry {
    uint8_t  id;            // originating mesh member
    int32_t  latE7;         // last-known position, degrees * 1e7
    int32_t  lonE7;
    uint32_t lastUpdateMs;  // millis() of the newest POSITION. Age drives the
                            //   navigating/stale/lost states.
    bool     used;

    // Relay visibility (field-test instrumentation). Hops traveled =
    // MESH_HOP_LIMIT - received hop_limit: 0 = heard the originator's own
    // transmission, >=1 = this position arrived via relay.
    uint8_t  lastHops;      // hops traveled by the newest packet
    float    lastRssi;      // dBm of that packet
    float    lastSnr;       // dB of that packet
    uint16_t directCount;   // packets from this member heard directly (0 hops)
    uint16_t relayCount;    // packets from this member that arrived relayed
};

enum RosterResult : uint8_t {
    ROSTER_NEW,       // first time this member was heard
    ROSTER_UPDATED,   // existing member refreshed
};

// Record a position report + its link stats. Only call with real fixes -- a
// no-fix beacon keeps the member's previous position (its age keeps growing,
// which is exactly what the stale display should reflect).
RosterResult rosterUpdate(uint8_t id, int32_t latE7, int32_t lonE7,
                          uint8_t hopsTraveled, float rssi, float snr,
                          uint32_t nowMs);

// The member with the newest position, or false if none heard yet.
bool rosterMostRecent(RosterEntry &out);

// Look up one member by id.
bool rosterGet(uint8_t id, RosterEntry &out);

// How many members are known.
size_t rosterCount();

// Iterate members for display: idx 0..rosterCount()-1, slot order.
bool rosterByIndex(size_t idx, RosterEntry &out);

// Target cycling: the next known member id after `afterId` (slot order,
// wraps). Returns 0 if the roster is empty. afterId 0 returns the first.
uint8_t rosterNextId(uint8_t afterId);
