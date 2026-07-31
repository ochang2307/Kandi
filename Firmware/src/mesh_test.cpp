#include <Arduino.h>
#include <string.h>
#include "selftest.h"
#include "mesh.h"

// Boot self-tests for the mesh port.
//
// Part 1 re-runs CoreLogic/network_test.py's three scenarios: the "network"
// is a handful of MeshNode instances plus an adjacency matrix, and deliver()
// recurses exactly like network.py's Network.deliver() -- a relay decision
// triggers delivery of the decremented packet to the relayer's neighbors.
// (The sim relays instantly, bypassing the timed queue: these scenarios test
// the DECISION logic. Part 2 tests the queue and cache mechanics the Python
// sim never had.)

static int passed = 0;
static int failed = 0;

static void check(const char *name, bool ok, long got, long expect) {
    if (ok) passed++; else failed++;
    Serial.printf("[%s] %-30s got %ld  expect %ld\n",
                  ok ? "PASS" : "FAIL", name, got, expect);
}

// ------------------------- The mini network sim ------------------------------
// Ids are 1-based like the Python; index 0 is unused.
static const int SIM_MAX = 5;

struct SimNet {
    MeshNode nodes[SIM_MAX];
    bool     link[SIM_MAX][SIM_MAX];
    int      processedCount[SIM_MAX];   // times each node PROCESSED (not dropped)
};

static void simInit(SimNet &net, int count, uint8_t groupId) {
    memset(&net, 0, sizeof(net));
    for (int i = 1; i <= count; i++) meshInit(net.nodes[i], i, groupId);
}

static void simConnect(SimNet &net, int a, int b) {
    net.link[a][b] = net.link[b][a] = true;   // radio links are bidirectional
}

// network.py Network.deliver(): run the receive decision; on a relay decision,
// flood the decremented packet to the receiver's neighbors.
static void simDeliver(SimNet &net, int toId, const MeshPacket &pkt, uint32_t now) {
    MeshAction act = meshHandlePacket(net.nodes[toId], pkt, now);
    if (act == MESH_PROCESS_NO_RELAY || act == MESH_PROCESS_AND_RELAY) {
        net.processedCount[toId]++;
    }
    if (act != MESH_PROCESS_AND_RELAY) return;

    MeshPacket relayed = pkt;
    relayed.hopLimit = pkt.hopLimit - 1;
    for (int n = 1; n < SIM_MAX; n++) {
        if (net.link[toId][n]) simDeliver(net, n, relayed, now);
    }
}

// network.py Network.originate(): mark own packet seen, flood to neighbors.
static void simOriginate(SimNet &net, int fromId, const MeshPacket &pkt, uint32_t now) {
    meshHandlePacket(net.nodes[fromId], pkt, now);   // marks it seen on the originator
    for (int n = 1; n < SIM_MAX; n++) {
        if (net.link[fromId][n]) simDeliver(net, n, pkt, now);
    }
}

static MeshPacket simPacket(uint8_t sender, uint16_t id, uint8_t hops, uint8_t group) {
    MeshPacket p = {};
    p.senderId = sender;
    p.packetId = id;
    p.hopLimit = hops;
    p.msgType  = MESH_MSG_POSITION;
    p.groupId  = group;
    return p;
}

bool runMeshSelfTests() {
    passed = failed = 0;
    Serial.println("--- mesh self-tests (network.py scenarios + embedded bits) ---");

    // ---- Wire codec round-trip (no Python analogue; the dict never left RAM)
    {
        MeshPacket in = {};
        in.senderId = 7; in.packetId = 0xBEEF; in.hopLimit = 3;
        in.msgType = MESH_MSG_POSITION; in.groupId = 42;
        in.latE7 = 372755809; in.lonE7 = -1220247906; in.fixValid = true;

        uint8_t wire[MESH_WIRE_SIZE];
        meshSerialize(in, wire);
        MeshPacket out = {};
        bool ok = meshDeserialize(wire, MESH_WIRE_SIZE, out);
        ok = ok && out.senderId == in.senderId && out.packetId == in.packetId
                && out.hopLimit == in.hopLimit && out.msgType == in.msgType
                && out.groupId == in.groupId && out.latE7 == in.latE7
                && out.lonE7 == in.lonE7 && out.fixValid == in.fixValid;
        check("codec: round-trip", ok, ok, 1);

        wire[0] = 'X';   // corrupt magic
        check("codec: bad magic rejected", !meshDeserialize(wire, MESH_WIRE_SIZE, out), 1, 1);
        meshSerialize(in, wire);
        check("codec: bad length rejected", !meshDeserialize(wire, MESH_WIRE_SIZE - 1, out), 1, 1);
    }

    // ---- network_test.py scenario 1: line 1<->2<->3, relay crosses the gap
    {
        SimNet net;
        simInit(net, 3, 1);
        simConnect(net, 1, 2);
        simConnect(net, 2, 3);
        simOriginate(net, 1, simPacket(1, 100, 3, 1), 1000);
        check("line: packet reached node 3", net.processedCount[3] == 1,
              net.processedCount[3], 1);
    }

    // ---- scenario 2: triangle loop -- dedup stops infinite rebroadcast.
    // The Python proof was "the program terminates". Here recursion depth is
    // bounded the same way; the checkable claim is each node processed EXACTLY
    // once, everything after that dropped as duplicate.
    {
        SimNet net;
        simInit(net, 3, 1);
        simConnect(net, 1, 2);
        simConnect(net, 2, 3);
        simConnect(net, 3, 1);
        simOriginate(net, 1, simPacket(1, 200, 5, 1), 1000);
        check("loop: node 2 processed once", net.processedCount[2] == 1,
              net.processedCount[2], 1);
        check("loop: node 3 processed once", net.processedCount[3] == 1,
              net.processedCount[3], 1);
    }

    // ---- scenario 3: line of 4, hop_limit=1 dies before node 4
    {
        SimNet net;
        simInit(net, 4, 1);
        simConnect(net, 1, 2);
        simConnect(net, 2, 3);
        simConnect(net, 3, 4);
        simOriginate(net, 1, simPacket(1, 300, 1, 1), 1000);
        check("hops: node 2 got it", net.processedCount[2] == 1, net.processedCount[2], 1);
        check("hops: node 4 never did", net.processedCount[4] == 0, net.processedCount[4], 0);
    }

    // ---- group separation (mesh.py's first check)
    {
        SimNet net;
        simInit(net, 2, 1);
        net.nodes[2].groupId = 9;   // stranger at the same festival
        simConnect(net, 1, 2);
        simOriginate(net, 1, simPacket(1, 400, 3, 1), 1000);
        check("group: foreign packet dropped", net.processedCount[2] == 0,
              net.processedCount[2], 0);
    }

    // ---- Part 2: embedded mechanics the Python sim never had ----

    // Seen-cache expiry: a duplicate arriving after 30s is treated as new.
    {
        MeshNode n;
        meshInit(n, 1, 1);
        MeshPacket p = simPacket(2, 500, 0, 1);
        meshHandlePacket(n, p, 1000);                       // seen at t=1s
        MeshAction again = meshHandlePacket(n, p, 1000 + 31000);
        check("cache: entry expires at 30s", again != MESH_DROP_DUPLICATE, again,
              MESH_PROCESS_NO_RELAY);
    }

    // Seen-cache eviction: the 33rd distinct packet overwrites the oldest slot.
    {
        MeshNode n;
        meshInit(n, 1, 1);
        MeshPacket first = simPacket(2, 1, 0, 1);
        meshHandlePacket(n, first, 1000);
        for (uint16_t i = 2; i <= 33; i++) {                // 32 more packets
            meshHandlePacket(n, simPacket(2, i, 0, 1), 1001);
        }
        MeshAction again = meshHandlePacket(n, first, 1002);
        check("cache: oldest evicted at 33rd", again != MESH_DROP_DUPLICATE, again,
              MESH_PROCESS_NO_RELAY);
    }

    // SNR->delay map endpoints + monotonicity (weak SNR relays sooner).
    {
        check("delay: -20dB -> 200ms", meshRelayDelayMs(-20.0f) == 200,
              meshRelayDelayMs(-20.0f), 200);
        check("delay: +10dB -> 2000ms", meshRelayDelayMs(10.0f) == 2000,
              meshRelayDelayMs(10.0f), 2000);
        check("delay: weak < strong", meshRelayDelayMs(-15.0f) < meshRelayDelayMs(5.0f),
              meshRelayDelayMs(-15.0f), meshRelayDelayMs(5.0f));
    }

    // Scheduled relay: not due before its delay, due after, hop decremented.
    {
        MeshNode n;
        meshInit(n, 1, 1);
        MeshPacket p = simPacket(2, 600, 3, 1);
        meshScheduleRelay(n, p, 10.0f, 1000);               // strong SNR -> 2000ms
        MeshPacket out;
        check("queue: not due early", !meshRelayDue(n, 2999, out), 0, 0);
        bool due = meshRelayDue(n, 3000, out);
        check("queue: due on time", due, due, 1);
        check("queue: hop decremented", out.hopLimit == 2, out.hopLimit, 2);
    }

    // Duplicate heard mid-wait cancels the pending relay.
    {
        MeshNode n;
        meshInit(n, 1, 1);
        MeshPacket p = simPacket(2, 700, 3, 1);
        meshHandlePacket(n, p, 1000);                       // decision: relay
        meshScheduleRelay(n, p, 10.0f, 1000);
        MeshAction dup = meshHandlePacket(n, p, 1500);      // someone else's relay
        check("cancel: duplicate detected", dup == MESH_DROP_DUPLICATE, dup,
              MESH_DROP_DUPLICATE);
        MeshPacket out;
        check("cancel: relay never fires", !meshRelayDue(n, 5000, out), 0, 0);
    }

    Serial.printf("--- mesh self-tests: %d passed, %d FAILED ---\n", passed, failed);
    return failed == 0;
}
