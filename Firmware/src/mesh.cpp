#include <string.h>
#include "mesh.h"

static const char MESH_MAGIC[4] = {'K', 'N', 'D', 'M'};

// The per-build blocked list. The define expands to an initializer list; an
// empty one makes a zero-length array, which sizeof handles fine.
static const uint8_t BLOCKED[] = MESH_BLOCKED_SENDERS;

bool meshSenderBlocked(uint8_t senderId) {
    for (size_t i = 0; i < sizeof(BLOCKED); i++) {
        if (BLOCKED[i] == senderId) return true;
    }
    return false;
}

void meshInit(MeshNode &node, uint8_t deviceId, uint8_t groupId) {
    memset(&node, 0, sizeof(node));
    node.deviceId = deviceId;
    node.groupId  = groupId;
    node.nextPacketId = 1;
}

// ============================ Wire codec =====================================
// Little-endian, field by field. Deliberately NOT a struct memcpy: packed
// structs happen to have no padding today, but the wire format must survive a
// compiler upgrade, a different MCU, or a desktop test harness byte-for-byte.

static void putU16(uint8_t *b, uint16_t v) {
    b[0] = v & 0xFF;
    b[1] = (v >> 8) & 0xFF;
}
static void putI32(uint8_t *b, int32_t v) {
    uint32_t u = (uint32_t)v;
    b[0] = u & 0xFF;
    b[1] = (u >> 8) & 0xFF;
    b[2] = (u >> 16) & 0xFF;
    b[3] = (u >> 24) & 0xFF;
}
static uint16_t getU16(const uint8_t *b) {
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}
static int32_t getI32(const uint8_t *b) {
    return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8)
                     | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

size_t meshSerialize(const MeshPacket &p, uint8_t buf[MESH_WIRE_SIZE]) {
    memcpy(&buf[0], MESH_MAGIC, 4);
    buf[4] = p.senderId;
    putU16(&buf[5], p.packetId);
    // hop_limit and msg_type share a byte, 4 bits each -- both are small
    // (hops 0-3 in practice, a handful of message types) and every byte is
    // ~0.3ms of SF11 airtime.
    buf[7] = (uint8_t)((p.hopLimit & 0x0F) << 4) | (p.msgType & 0x0F);
    buf[8] = p.groupId;
    putI32(&buf[9],  p.latE7);
    putI32(&buf[13], p.lonE7);
    buf[17] = p.fixValid ? 0x01 : 0x00;
    return MESH_WIRE_SIZE;
}

bool meshDeserialize(const uint8_t *buf, size_t len, MeshPacket &out) {
    if (len != MESH_WIRE_SIZE) return false;
    if (memcmp(buf, MESH_MAGIC, 4) != 0) return false;
    out.senderId = buf[4];
    out.packetId = getU16(&buf[5]);
    out.hopLimit = (buf[7] >> 4) & 0x0F;
    out.msgType  = buf[7] & 0x0F;
    out.groupId  = buf[8];
    out.latE7    = getI32(&buf[9]);
    out.lonE7    = getI32(&buf[13]);
    out.fixValid = (buf[17] & 0x01) != 0;
    return true;
}

// ============================ Seen cache =====================================
// vs the Python set: the set answers "have I EVER seen (sender, id)?" exactly.
// The circular buffer answers "have I seen it RECENTLY?" approximately -- an
// entry can vanish by expiry (30s) or eviction (33rd packet overwrites the
// oldest). Both are the right trade here:
//   - a flood settles in a few seconds (3 hops x <=2s relay delay), so 30s of
//     memory covers any legitimate duplicate many times over;
//   - expiry is REQUIRED once packet_ids wrap (u16): without it, a months-old
//     cache entry would eat a future packet that legitimately reuses the id;
//   - eviction under load is self-limiting: 32 slots at ~10s beacon cadence is
//     ~5 minutes of group traffic memory; if packets are churning faster than
//     that, the dup you might re-relay is the least of the channel's problems.

static bool seenFresh(const SeenEntry &e, uint32_t nowMs) {
    return e.used && (nowMs - e.seenAtMs) < MESH_SEEN_EXPIRE_MS;
}

static bool meshHasSeen(const MeshNode &node, const MeshPacket &pkt, uint32_t nowMs) {
    for (size_t i = 0; i < MESH_SEEN_SLOTS; i++) {
        const SeenEntry &e = node.seen[i];
        if (seenFresh(e, nowMs)
            && e.senderId == pkt.senderId && e.packetId == pkt.packetId) {
            return true;
        }
    }
    return false;
}

static void meshMarkSeen(MeshNode &node, const MeshPacket &pkt, uint32_t nowMs) {
    SeenEntry &e = node.seen[node.seenNext];
    e.senderId = pkt.senderId;
    e.packetId = pkt.packetId;
    e.seenAtMs = nowMs;
    e.used     = true;
    node.seenNext = (node.seenNext + 1) % MESH_SEEN_SLOTS;
}

// ========================= Pending relay queue ===============================

uint32_t meshRelayDelayMs(float snrDb) {
    // Weak SNR = we're far from whoever sent this = we extend the network's
    // reach the most = we go FIRST. Strong SNR = we're standing next to them =
    // our relay adds little = wait, and probably get cancelled by hearing
    // someone else's. Linear map, clamped: -20dB -> 200ms, +10dB -> 2000ms.
    const float SNR_MIN = -20.0f, SNR_MAX = 10.0f;
    const float DELAY_MIN = 200.0f, DELAY_MAX = 2000.0f;
    float snr = snrDb;
    if (snr < SNR_MIN) snr = SNR_MIN;
    if (snr > SNR_MAX) snr = SNR_MAX;
    float t = (snr - SNR_MIN) / (SNR_MAX - SNR_MIN);
    return (uint32_t)(DELAY_MIN + t * (DELAY_MAX - DELAY_MIN));
}

bool meshScheduleRelay(MeshNode &node, const MeshPacket &pkt, float snrDb,
                       uint32_t nowMs) {
    for (size_t i = 0; i < MESH_RELAY_SLOTS; i++) {
        PendingRelay &r = node.relayQueue[i];
        if (r.active) continue;
        r.pkt = pkt;
        r.pkt.hopLimit = pkt.hopLimit - 1;   // the decrement from handle_packet
        r.dueAtMs = nowMs + meshRelayDelayMs(snrDb);
        r.active  = true;
        return true;
    }
    return false;   // queue full; this node just sits this relay out
}

// Called when a duplicate arrives mid-wait: someone else already relayed this
// packet, so transmitting our copy would only burn airtime.
static void meshCancelRelay(MeshNode &node, const MeshPacket &pkt) {
    for (size_t i = 0; i < MESH_RELAY_SLOTS; i++) {
        PendingRelay &r = node.relayQueue[i];
        if (r.active
            && r.pkt.senderId == pkt.senderId && r.pkt.packetId == pkt.packetId) {
            r.active = false;
        }
    }
}

bool meshRelayDue(MeshNode &node, uint32_t nowMs, MeshPacket &out) {
    for (size_t i = 0; i < MESH_RELAY_SLOTS; i++) {
        PendingRelay &r = node.relayQueue[i];
        if (r.active && (int32_t)(nowMs - r.dueAtMs) >= 0) {
            out = r.pkt;
            r.active = false;
            return true;
        }
    }
    return false;
}

// ========================= The receive decision ==============================
// mesh.py handle_packet(), same five steps in the same order, plus the blocked
// check in front (which models RF, not protocol -- a blocked sender's packet
// "never arrived").
//
// Python:
//   if packet["group_id"] != device["group_id"]: return "dropped_foreign"
//   if has_seen(seen_set, packet):               return "dropped_duplicate"
//   mark_seen(seen_set, packet)
//   if packet["hop_limit"] <= 0:                 return "processed_no_relay"
//   ...decrement + transmit...                   return "processed_and_relayed"
//
// One embedded addition: on the duplicate branch we also cancel any pending
// relay of that packet. The sim couldn't express this (its relays were
// instantaneous); with real 200-2000ms relay delays, a duplicate arriving
// mid-wait is exactly the signal that our relay became redundant.
MeshAction meshHandlePacket(MeshNode &node, const MeshPacket &pkt, uint32_t nowMs) {
    if (meshSenderBlocked(pkt.senderId)) {
        return MESH_DROP_BLOCKED;
    }
    if (pkt.groupId != node.groupId) {
        return MESH_DROP_FOREIGN;
    }
    if (meshHasSeen(node, pkt, nowMs)) {
        meshCancelRelay(node, pkt);
        return MESH_DROP_DUPLICATE;
    }
    meshMarkSeen(node, pkt, nowMs);
    if (pkt.hopLimit == 0) {
        return MESH_PROCESS_NO_RELAY;
    }
    return MESH_PROCESS_AND_RELAY;
}

// ============================== Origination ==================================

MeshPacket meshMakeBeacon(MeshNode &node, int32_t latE7, int32_t lonE7,
                          bool fixValid, uint32_t nowMs) {
    MeshPacket p;
    p.senderId = node.deviceId;
    p.packetId = node.nextPacketId++;
    if (node.nextPacketId == 0) node.nextPacketId = 1;   // skip 0 on u16 wrap
    p.hopLimit = MESH_HOP_LIMIT;
    p.msgType  = MESH_MSG_POSITION;
    p.groupId  = node.groupId;
    p.latE7    = latE7;
    p.lonE7    = lonE7;
    p.fixValid = fixValid;

    // network.py originate(): mark our own packet seen so a copy that comes
    // back via someone's relay is dropped, not re-processed and re-flooded.
    meshMarkSeen(node, p, nowMs);
    return p;
}
