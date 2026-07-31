#pragma once
#include <stdint.h>

// Thin driver for the on-board SX1262 LoRa radio (RadioLib).
//
// !!! SAFETY: NEVER let this transmit without the antenna attached. TX into an
// !!! open output can reflect enough power to destroy the RF stage. Any build
// !!! with IS_SENDER=1 transmits within seconds of boot -- antenna ON before
// !!! flashing it.
//
// The radio owns the GLOBAL SPI bus (FSPI, pins 12/13/11), started here the
// same way LilyGo's Factory firmware does: SPI.begin(SCLK, MISO, MOSI). This
// is exactly why imu.cpp built its own SPIClass(HSPI) instead of touching the
// global -- the two buses are physically separate and must stay that way.
//
// Radio parameters are the design doc's locked-in "Long Fast" preset:
// 915 MHz, SF11, 250 kHz, CR 4/5, +14 dBm. Do not tune them for range or
// speed -- the mesh math (airtime, relay delays) is built around this preset.

// --- Mode toggle ---
// MESH_MODE 1: every board is a symmetric mesh node -- beacons its own
// position (design-doc cadence), listens continuously, and relays per
// mesh.cpp's managed-flooding rules. Per-board identity comes from
// MESH_DEVICE_ID / MESH_BLOCKED_SENDERS in mesh.h -- set those before
// flashing each board.
// MESH_MODE 0: the older two-board range test (IS_SENDER picks the role).
#define MESH_MODE 1

// --- Role toggle (range test only; ignored when MESH_MODE=1) ---
// 1 = this board transmits a counter packet every 3s.
// 0 = this board listens continuously and reports what it hears.
// Flash one board with each; everything else is identical.
#define IS_SENDER 1

struct RadioStats {
    bool     online;      // radio answered at init and is configured
    uint32_t txCount;     // packets fully sent (counted at TX-done, not start)
    uint32_t rxCount;     // valid packets received (magic + CRC checked)
    uint32_t crcErrors;   // packets that arrived but failed CRC -- edge-of-range sign
    uint32_t lastCounter; // counter value inside the newest packet
    float    rssi;        // dBm, of the newest packet
    float    snr;         // dB, of the newest packet

    // Range-test fields (receiver role). Compare rxCount against
    // lastCounter+1 to see dropped-packet gaps (counter starts at 0).
    uint32_t lastRxMillis;   // millis() when the newest packet landed; 0 = never.
                             //   The AGE derived from this is the liveness
                             //   signal -- everything else here freezes at its
                             //   last value when the link dies.
    bool     senderHasFix;   // fix-valid flag inside the newest packet
    bool     distValid;      // both sides had a fix; distM/maxDistM updated
    double   distM;          // GPS distance to sender at the newest packet
    double   maxDistM;       // farthest distance a packet has ever arrived from

    // Mesh mode. txCount above becomes "own beacons sent"; rxCount becomes
    // "packets processed" (new information -- duplicates/blocked/foreign are
    // serial-logged but not counted here).
    uint32_t relayCount;     // relays actually transmitted (not just scheduled)
    uint32_t dupCount;       // duplicates heard (each one is flooding working)
    uint8_t  lastSender;     // device id the newest processed packet came from
    uint8_t  lastHops;       // hop limit REMAINING in that packet (3 = direct)
};

// Bring up SPI + the SX1262 and configure the Long Fast preset. Returns false
// if the radio doesn't respond (rail off, SPI miswired) or any config step is
// rejected. Receiver role starts listening immediately.
bool radioBegin();

// Radio state machine, called EVERY loop pass (like gpsPump). Non-blocking:
// all TX/RX runs interrupt-driven via DIO1, so the SF11 airtime (order 100ms+
// per packet) is spent by the radio alone -- the loop never waits on it, and
// the GPS UART keeps draining. If fixes get flaky anyway, suspect RF noise,
// not loop timing.
void radioTick();

// Snapshot of counters + last-packet quality. Cheap; safe every tick.
RadioStats radioStats();
