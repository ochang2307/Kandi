#pragma once
#include <stdint.h>
#include <stddef.h>

// Managed-flooding mesh -- port of CoreLogic/mesh.py + the per-device pieces
// of network.py, hardened for embedded use.
//
// The Python is the REFERENCE FOR BEHAVIOR: the five-step receive sequence in
// meshHandlePacket() (group check -> dedup -> mark seen -> hop-limit -> relay
// decision) is exactly handle_packet()'s. The STRUCTURE changes because this
// runs on hardware: fixed-size seen cache instead of an unbounded set, an
// explicit wire format instead of a dict, and a scheduled relay queue with the
// SNR-weighted delay the design doc specified but the sim never implemented.
// No dynamic allocation anywhere in this module.
//
// Everything here is pure logic -- no radio, no clock reads. Callers pass
// `nowMs` in. That's what lets the same code run in the boot self-tests as
// N simulated devices (mesh_test.cpp) and on the host.

// ============================== Per-board config =============================
// Set DEVICE_ID uniquely per board before flashing (like IS_SENDER was).
#define MESH_DEVICE_ID  1
#define MESH_GROUP_ID   1

// Testing hook: node ids this board pretends not to hear. Dropped at
// reception BEFORE any protocol step -- it models RF (out of range), not
// policy, so it filters on the TRANSMITTER of each frame (the txNode byte,
// which a relayer overwrites with its own id), NOT the originator. Blocking
// the originator would also kill relayed copies and defeat the whole test.
// Forces a multi-hop topology on a desk where everyone hears everyone:
//   board 1: {3}   board 2: {}   board 3: {1}
// gives the line 1 <-> 2 <-> 3 from network_test.py, with 2 as the only bridge.
#define MESH_BLOCKED_SENDERS  {}

// Design-doc cadence: position beacon every 10s with +/-2s jitter. Shorten
// temporarily for impatient desk tests, but field tests use the real numbers
// (the mesh airtime budget is built around them).
#define MESH_BEACON_MS        10000
#define MESH_BEACON_JITTER_MS 2000

// Design-doc hop limit for freshly originated packets.
#define MESH_HOP_LIMIT        3

// ================================ Wire format ================================
// 19 bytes on air, explicitly serialized byte-by-byte (little-endian) in
// meshSerialize() -- NOT memcpy'd from the struct -- so the format is
// deterministic regardless of compiler padding or platform endianness:
//
//   [0..3]   magic "KNDN"        protocol discriminator + version (bump on change)
//   [4]      sender_id   u8     ORIGINATOR -- never changes as the packet hops
//   [5..6]   packet_id   u16    per-sender sequence number
//   [7]      hop/type    u8     high nibble hop_limit, low nibble msg_type
//   [8]      group_id    u8     (placeholder -- real AES group key comes with bonding)
//   [9..12]  latE7       i32    degrees * 1e7 (u-blox fixed-point; no floats on air)
//   [13..16] lonE7       i32
//   [17]     flags       u8     bit0 = position is a live fix
//   [18]     tx_node     u8     who TRANSMITTED this frame -- the originator on
//                               a beacon, the relayer on a relay. Dedup keys on
//                               (sender_id, packet_id); tx_node exists so the
//                               out-of-range simulation (and later, per-hop
//                               diagnostics) can see who was actually on air.
static const size_t MESH_WIRE_SIZE = 19;

enum MeshMsgType : uint8_t {
    MESH_MSG_POSITION = 0,
    MESH_MSG_SOS      = 1,   // reserved; not sent yet
};

// In-memory packet. Natural types here; nibble packing happens only at the
// serialize/deserialize boundary.
struct MeshPacket {
    uint8_t  senderId;    // originator
    uint16_t packetId;
    uint8_t  hopLimit;    // 0-15 (4 bits on the wire)
    uint8_t  msgType;     // MeshMsgType, 0-15
    uint8_t  groupId;
    int32_t  latE7;
    int32_t  lonE7;
    bool     fixValid;
    uint8_t  txNode;      // transmitter of this frame (relayer stamps its own id)
};

// =============================== Seen cache ==================================
// The Python used a set of (sender_id, packet_id) tuples -- unbounded, exact,
// remembers forever. Fine in a sim; a slow leak on a device that runs for a
// weekend. This is a fixed 32-slot circular buffer instead: markSeen overwrites
// the oldest slot when full, and entries expire after 30s regardless.
// Consequences + why they're safe: see the note above meshHasSeen() in mesh.cpp.
static const size_t   MESH_SEEN_SLOTS     = 32;
static const uint32_t MESH_SEEN_EXPIRE_MS = 30000;

struct SeenEntry {
    uint8_t  senderId;
    uint16_t packetId;
    uint32_t seenAtMs;
    bool     used;        // slot has ever been written
};

// ============================ Pending relay queue ============================
// Relays are SCHEDULED, never sent inline: SNR-weighted delay of 200-2000ms
// (weak SNR = far from the sender = likely network edge = relay sooner).
// The queue is polled from the loop every pass -- no delay() anywhere.
// An entry is cancelled if a duplicate of its packet is heard while waiting:
// that duplicate IS some other node's relay, so ours is redundant.
static const size_t MESH_RELAY_SLOTS = 8;

struct PendingRelay {
    MeshPacket pkt;       // already hop-decremented, ready to serialize
    uint32_t   dueAtMs;
    bool       active;
};

// ================================ Node state =================================
// One struct = one device (mirrors network.py giving each sim device its own
// seen set). Firmware uses a single instance; the boot self-tests make several.
struct MeshNode {
    uint8_t      deviceId;
    uint8_t      groupId;
    SeenEntry    seen[MESH_SEEN_SLOTS];
    size_t       seenNext;                  // circular write index
    PendingRelay relayQueue[MESH_RELAY_SLOTS];
    uint16_t     nextPacketId;              // sequence for own beacons
};

// What meshHandlePacket decided -- the Python return strings, as an enum.
enum MeshAction : uint8_t {
    MESH_DROP_BLOCKED,        // testing hook: simulated out-of-range
    MESH_DROP_FOREIGN,        // "dropped_foreign"
    MESH_DROP_DUPLICATE,      // "dropped_duplicate" (also cancels pending relay)
    MESH_PROCESS_NO_RELAY,    // "processed_no_relay" (hop limit exhausted)
    MESH_PROCESS_AND_RELAY,   // "processed_and_relayed" -> caller schedules relay
};

// ================================== API ======================================
void   meshInit(MeshNode &node, uint8_t deviceId, uint8_t groupId);

// Explicit wire codec. Serialize always writes MESH_WIRE_SIZE bytes; returns
// that size. Deserialize validates magic + length and returns false on any
// mismatch (stale-firmware packets get rejected, not misparsed).
size_t meshSerialize(const MeshPacket &p, uint8_t buf[MESH_WIRE_SIZE]);
bool   meshDeserialize(const uint8_t *buf, size_t len, MeshPacket &out);

// The five-step receive decision, exactly mesh.py's handle_packet(). Mutates
// the node's seen cache (marks new packets seen; that's in the Python too) and
// cancels a matching pending relay on MESH_DROP_DUPLICATE -- but never touches
// the radio. On MESH_PROCESS_AND_RELAY the caller schedules the relay.
MeshAction meshHandlePacket(MeshNode &node, const MeshPacket &pkt, uint32_t nowMs);

// Queue a relay of `pkt` (hop decrement happens in here), delayed by SNR.
// Returns false if the queue is full (packet just doesn't get relayed by us --
// with 8 slots and ~10s beacons that means the channel is already saturated).
bool meshScheduleRelay(MeshNode &node, const MeshPacket &pkt, float snrDb,
                       uint32_t nowMs);

// Pop one due relay into `out`, skipping cancelled entries. Stamps this node's
// id into out.txNode -- we are about to be the transmitter. Call every loop
// pass; returns false when nothing is due yet.
bool meshRelayDue(MeshNode &node, uint32_t nowMs, MeshPacket &out);

// Build this node's own position beacon (stamps id/group/seq, hop limit 3) and
// mark it seen -- network.py's originate() does the same so a packet that
// comes back via relay is dropped as a duplicate, not re-flooded.
MeshPacket meshMakeBeacon(MeshNode &node, int32_t latE7, int32_t lonE7,
                          bool fixValid, uint32_t nowMs);

// The SNR->delay map, exposed for tests: 200ms at/below -20dB SNR rising
// linearly to 2000ms at/above +10dB.
uint32_t meshRelayDelayMs(float snrDb);

// True if `nodeId` is in this build's MESH_BLOCKED_SENDERS list. Callers pass
// the frame's txNode (the transmitter), not the originator -- RF range is
// about who's on air right now.
bool meshSenderBlocked(uint8_t nodeId);
