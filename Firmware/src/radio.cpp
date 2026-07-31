#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "radio.h"
#include "gps.h"          // sender stamps its fix in; receiver compares fixes
#include "navigation.h"   // receiver computes true GPS distance per packet
#include "mesh.h"         // mesh mode: managed-flooding logic (pure, no radio)

// --- Pins (verified against LilyGo utilities.h, T_BEAM_S3_SUPREME block) ---
// This is the radio's own SPI bus (FSPI / the global `SPI` object) -- fully
// separate wires from the sensor/SD bus (36/37/35) the IMU runs on.
static const int RADIO_SCLK = 12;
static const int RADIO_MISO = 13;
static const int RADIO_MOSI = 11;
static const int RADIO_CS   = 10;
static const int RADIO_RST  = 5;
static const int RADIO_DIO1 = 1;    // interrupt out: fires on TX-done / RX-done
static const int RADIO_BUSY = 4;

// Module arg order is (CS, IRQ, RST, GPIO) -- from LilyGo's Factory.ino for
// the SX1262: new Module(CS, DIO1, RST, BUSY). Uses the global SPI bus.
static SX1262 radio = new Module(RADIO_CS, RADIO_DIO1, RADIO_RST, RADIO_BUSY);

// --- Locked-in "Long Fast" preset (design doc; do not tune) ---
static const float   LORA_FREQ_MHZ = 915.0f;
static const float   LORA_BW_KHZ   = 250.0f;
static const uint8_t LORA_SF       = 11;
static const uint8_t LORA_CR       = 5;     // RadioLib takes the denominator: 4/5
static const int8_t  LORA_TX_DBM   = 14;

// Position packet, 17 bytes packed -- a step toward the real mesh packet.
// The magic filters out other 915MHz LoRa traffic that happens to share these
// settings, and its version digit is bumped ("KND2") whenever the layout
// changes so a board running stale firmware gets rejected instead of
// misparsed.
//
// Coordinates ride as int32 in 1e-7 degrees (the u-blox native format):
// fixed width on the air regardless of platform double size, ~1.1cm
// resolution -- far below GPS error -- and 8 bytes for the pair instead
// of 16.
struct __attribute__((packed)) TestPacket {
    char     magic[4];    // "KND2"
    uint32_t counter;
    int32_t  latE7;       // degrees * 1e7; 0 when fixValid == 0
    int32_t  lonE7;
    uint8_t  fixValid;    // 1 = latE7/lonE7 are a live fix, 0 = ignore them
};
static const char PACKET_MAGIC[4] = {'K', 'N', 'D', '2'};

static RadioStats stats = {};
static uint32_t   txCounter = 0;      // next counter value to send
static bool       txInFlight = false; // startTransmit() issued, TX-done pending
static uint32_t   lastTxStart = 0;

#if MESH_MODE
static MeshNode meshNode;
static uint32_t nextBeaconAt = 0;
static bool     txWasRelay   = false;   // what kind of TX is in flight (for stats)

// Next beacon: design-doc cadence with jitter, so three boards booted together
// don't lock into colliding on air forever. esp_random() is the hardware RNG.
static void scheduleNextBeacon() {
    int32_t jitter = (int32_t)(esp_random() % (2 * MESH_BEACON_JITTER_MS + 1))
                     - MESH_BEACON_JITTER_MS;
    nextBeaconAt = millis() + MESH_BEACON_MS + jitter;
}
#endif

// DIO1 interrupt: just raise a flag. All real work (SPI transfers!) happens in
// radioTick() -- SPI inside an ISR would collide with whatever transfer the
// main loop is mid-way through on the other bus.
static volatile bool dio1Flag = false;
static void IRAM_ATTR onDio1() { dio1Flag = true; }

bool radioBegin() {
    // The radio owns the global SPI bus, pinned to its GPIOs -- same call as
    // LilyGo's setupBoards(): SPI.begin(SCLK, MISO, MOSI). (CS is handed to
    // RadioLib, which drives it per-transfer.)
    SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI);

    // begin() resets the chip, checks it responds, and configures the LoRa
    // modem. Sync word stays RadioLib's private-network default (0x12) --
    // Meshtastic uses 0x2B, so we're deliberately invisible to Meshtastic
    // nodes even on the same preset. Remaining defaults: preamble 8 symbols,
    // TCXO 1.6V (this module's DIO3-fed oscillator; LilyGo's own examples run
    // the plain defaults), CRC on.
    int16_t state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                                RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_DBM);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("SX1262 begin failed, code %d (rail? wiring?)\n", state);
        return false;
    }

    // On this module DIO2 drives the TX/RX antenna switch -- without this the
    // radio "works" but the antenna is never connected to the right path:
    // TX radiates almost nothing and RX hears almost nothing. Straight from
    // LilyGo's setupRfSwitch() for USING_SX1262.
    radio.setDio2AsRfSwitch(true);

    // Overcurrent protection for the PA, LilyGo's value. +14dBm draws well
    // under this; it's a ceiling, not a target.
    radio.setCurrentLimit(140);

    radio.setDio1Action(onDio1);

#if MESH_MODE
    meshInit(meshNode, MESH_DEVICE_ID, MESH_GROUP_ID);

    // Every mesh node listens from the start; TX interleaves when due.
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("SX1262 startReceive failed, code %d\n", state);
        return false;
    }

    // First beacon soon after boot (small + jittered) so a fresh board
    // announces itself without three simultaneous boots colliding.
    nextBeaconAt = millis() + 2000 + (esp_random() % 2000);

    Serial.printf("SX1262 online: MESH NODE %d group %d, %.0fMHz SF%u BW%.0fk\n",
                  MESH_DEVICE_ID, MESH_GROUP_ID, LORA_FREQ_MHZ, LORA_SF, LORA_BW_KHZ);
    {
        const uint8_t blocked[] = MESH_BLOCKED_SENDERS;
        if (sizeof(blocked) > 0) {
            Serial.print("MESH: simulating out-of-range for senders:");
            for (size_t i = 0; i < sizeof(blocked); i++) Serial.printf(" %d", blocked[i]);
            Serial.println();
        }
    }
#elif IS_SENDER
    Serial.printf("SX1262 online: SENDER, %.0fMHz SF%u BW%.0fk CR4/%u +%ddBm\n",
                  LORA_FREQ_MHZ, LORA_SF, LORA_BW_KHZ, LORA_CR, LORA_TX_DBM);
#else
    // Receiver arms RX once here; re-armed after every packet in radioTick().
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("SX1262 startReceive failed, code %d\n", state);
        return false;
    }
    Serial.printf("SX1262 online: RECEIVER, %.0fMHz SF%u BW%.0fk CR4/%u\n",
                  LORA_FREQ_MHZ, LORA_SF, LORA_BW_KHZ, LORA_CR);
#endif

    stats.online = true;
    return true;
}

#if MESH_MODE
// Serialize + hand a packet to the radio. Half-duplex: only ever called when
// no TX is in flight; the radio implicitly leaves RX mode. Returns whether the
// transmit started.
static bool meshTxStart(const MeshPacket &p, bool isRelay) {
    uint8_t wire[MESH_WIRE_SIZE];
    meshSerialize(p, wire);
    int16_t state = radio.startTransmit(wire, MESH_WIRE_SIZE);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("MESH: startTransmit failed, code %d\n", state);
        radio.startReceive();   // fall back to listening rather than going deaf
        return false;
    }
    txInFlight = true;
    txWasRelay = isRelay;
    return true;
}

static const char *meshActionStr(MeshAction a) {
    switch (a) {
        case MESH_DROP_BLOCKED:      return "dropped_blocked(sim-range)";
        case MESH_DROP_FOREIGN:      return "dropped_foreign";
        case MESH_DROP_DUPLICATE:    return "dropped_duplicate";
        case MESH_PROCESS_NO_RELAY:  return "processed_no_relay";
        case MESH_PROCESS_AND_RELAY: return "processed_and_relayed";
    }
    return "?";
}
#endif

void radioTick() {
    if (!stats.online) return;

#if MESH_MODE
    // ---- 1. Service DIO1 events FIRST, before starting any new TX. The chip's
    // own IRQ flags say what actually happened (TX_DONE vs RX_DONE) -- more
    // robust than inferring from our own state, since both events route
    // through the same pin.
    if (dio1Flag) {
        dio1Flag = false;
        uint32_t irq = radio.getIrqFlags();

        if (txInFlight && (irq & RADIOLIB_SX126X_IRQ_TX_DONE)) {
            radio.finishTransmit();
            txInFlight = false;
            if (txWasRelay) {
                stats.relayCount++;
                Serial.println("MESH: TX relay done");
            } else {
                stats.txCount++;
                Serial.printf("MESH: TX beacon done  #%u\n",
                              (unsigned)(meshNode.nextPacketId - 1));
            }
            radio.startReceive();   // half-duplex: straight back to listening
        } else if (irq & RADIOLIB_SX126X_IRQ_RX_DONE) {
            uint8_t wire[MESH_WIRE_SIZE] = {0};
            size_t  len = radio.getPacketLength();
            int16_t st  = radio.readData(wire, len > sizeof(wire) ? sizeof(wire) : len);

            MeshPacket pkt;
            if (st == RADIOLIB_ERR_NONE && meshDeserialize(wire, len, pkt)) {
                float snr  = radio.getSNR();
                float rssi = radio.getRSSI();
                MeshAction act = meshHandlePacket(meshNode, pkt, millis());

                Serial.printf("MESH: RX from %u #%u hops %u  %s  RSSI %.1f  SNR %.1f\n",
                              pkt.senderId, pkt.packetId, pkt.hopLimit,
                              meshActionStr(act), rssi, snr);

                if (act == MESH_PROCESS_NO_RELAY || act == MESH_PROCESS_AND_RELAY) {
                    // New information from a group member: update the readout.
                    stats.rxCount++;
                    stats.lastCounter  = pkt.packetId;
                    stats.lastSender   = pkt.senderId;
                    stats.lastHops     = pkt.hopLimit;
                    stats.rssi         = rssi;
                    stats.snr          = snr;
                    stats.lastRxMillis = millis();
                    stats.senderHasFix = pkt.fixValid;

                    GpsStatus g = gpsStatus();
                    stats.distValid = pkt.fixValid && g.valid;
                    if (stats.distValid) {
                        stats.distM = distance(g.lat, g.lon,
                                               pkt.latE7 * 1e-7, pkt.lonE7 * 1e-7);
                        if (stats.distM > stats.maxDistM) stats.maxDistM = stats.distM;
                        Serial.printf("MESH: member %u at %.1f m\n",
                                      pkt.senderId, stats.distM);
                    }
                }
                if (act == MESH_PROCESS_AND_RELAY) {
                    // Decision says relay; the QUEUE says when (SNR-weighted,
                    // 200-2000ms). Actual TX happens in step 2 of a later
                    // pass -- never inline, never blocking.
                    meshScheduleRelay(meshNode, pkt, snr, millis());
                }
                if (act == MESH_DROP_DUPLICATE) {
                    stats.dupCount++;
                }
            } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
                stats.crcErrors++;
                Serial.printf("MESH: RX CRC error (n=%lu)\n",
                              (unsigned long)stats.crcErrors);
            } else {
                Serial.printf("MESH: RX rejected (code %d, len %u)\n",
                              st, (unsigned)len);
            }
            if (!txInFlight) radio.startReceive();   // re-arm
        } else {
            // Spurious/other IRQ (e.g. stale flag after a lost race between an
            // RX completing and a TX starting). Just make sure we're listening.
            if (!txInFlight) radio.startReceive();
        }
    }

    // ---- 2. TX opportunities, only when the radio is idle (half-duplex: a
    // transmit would stomp a reception in progress; the queue and beacon can
    // both wait a pass). Due relays outrank the beacon -- forwarding someone
    // at the network edge matters more than repeating ourselves on schedule.
    if (!txInFlight) {
        MeshPacket out;
        if (meshRelayDue(meshNode, millis(), out)) {
            Serial.printf("MESH: TX relay of %u #%u (hops left %u)\n",
                          out.senderId, out.packetId, out.hopLimit);
            meshTxStart(out, true);
        } else if ((int32_t)(millis() - nextBeaconAt) >= 0) {
            GpsStatus g = gpsStatus();
            int32_t latE7 = g.valid ? (int32_t)llround(g.lat * 1e7) : 0;
            int32_t lonE7 = g.valid ? (int32_t)llround(g.lon * 1e7) : 0;
            MeshPacket beacon = meshMakeBeacon(meshNode, latE7, lonE7,
                                               g.valid, millis());
            meshTxStart(beacon, false);
            scheduleNextBeacon();
        }
    }

#elif IS_SENDER
    // TX-done: DIO1 fired after a startTransmit(). Clean up, count it.
    if (dio1Flag) {
        dio1Flag = false;
        radio.finishTransmit();   // clears IRQ, puts radio back in standby
        txInFlight = false;
        stats.txCount++;
        Serial.printf("LORA: TX done  #%lu\n", (unsigned long)(txCounter - 1));
    }

    // Kick off the next packet every 3s -- but never stack a second TX on top
    // of one still in flight (SF11 airtime is a real fraction of a second).
    if (!txInFlight && millis() - lastTxStart >= 3000) {
        lastTxStart = millis();

        // Stamp in our current fix. No fix yet is NOT a reason to stay quiet:
        // the flag rides along as false and the link stays testable indoors.
        GpsStatus g = gpsStatus();
        TestPacket pkt;
        memcpy(pkt.magic, PACKET_MAGIC, 4);
        pkt.counter  = txCounter;
        pkt.fixValid = g.valid ? 1 : 0;
        pkt.latE7    = g.valid ? (int32_t)llround(g.lat * 1e7) : 0;
        pkt.lonE7    = g.valid ? (int32_t)llround(g.lon * 1e7) : 0;

        // Non-blocking: hands the packet to the radio and returns in ~a ms.
        // The airtime burns inside the SX1262 while the loop keeps running.
        int16_t state = radio.startTransmit((uint8_t *)&pkt, sizeof(pkt));
        if (state == RADIOLIB_ERR_NONE) {
            txInFlight = true;
            txCounter++;
        } else {
            Serial.printf("LORA: startTransmit failed, code %d\n", state);
        }
    }
#else
    // RX-done: a packet is sitting in the radio's buffer.
    if (dio1Flag) {
        dio1Flag = false;

        uint8_t buf[sizeof(TestPacket)] = {0};
        size_t  len = radio.getPacketLength();
        int16_t state = radio.readData(buf, len > sizeof(buf) ? sizeof(buf) : len);

        if (state == RADIOLIB_ERR_NONE
            && len == sizeof(TestPacket)
            && memcmp(buf, PACKET_MAGIC, 4) == 0) {
            TestPacket *pkt = (TestPacket *)buf;
            stats.rxCount++;
            stats.lastCounter  = pkt->counter;
            stats.rssi         = radio.getRSSI();
            stats.snr          = radio.getSNR();
            stats.lastRxMillis = millis();
            stats.senderHasFix = pkt->fixValid != 0;

            // True GPS distance, only when BOTH ends hold a fix. distM/maxDistM
            // keep their last values otherwise -- distValid is what says
            // whether this packet refreshed them.
            GpsStatus g = gpsStatus();
            stats.distValid = stats.senderHasFix && g.valid;
            if (stats.distValid) {
                stats.distM = distance(g.lat, g.lon,
                                       pkt->latE7 * 1e-7, pkt->lonE7 * 1e-7);
                if (stats.distM > stats.maxDistM) stats.maxDistM = stats.distM;
            }

            // One line per packet, fixed field order -- grep "LORA: RX" and
            // you have the RSSI-vs-distance dataset for the range plot.
            if (stats.distValid) {
                Serial.printf("LORA: RX #%lu  dist %.1f m  RSSI %.1f dBm  SNR %.1f dB  (n=%lu)\n",
                              (unsigned long)pkt->counter, stats.distM,
                              stats.rssi, stats.snr, (unsigned long)stats.rxCount);
            } else {
                Serial.printf("LORA: RX #%lu  dist --%s  RSSI %.1f dBm  SNR %.1f dB  (n=%lu)\n",
                              (unsigned long)pkt->counter,
                              stats.senderHasFix ? " (rx no fix)" : " (tx no fix)",
                              stats.rssi, stats.snr, (unsigned long)stats.rxCount);
            }
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // Arrived but corrupted -- starts happening at the edge of range.
            stats.crcErrors++;
            Serial.printf("LORA: RX CRC error (n=%lu)\n",
                          (unsigned long)stats.crcErrors);
        } else {
            Serial.printf("LORA: RX rejected (code %d, len %u)\n",
                          state, (unsigned)len);
        }

        // Re-arm. readData drops the radio to standby; without this it would
        // hear exactly one packet per boot.
        radio.startReceive();
    }
#endif
}

RadioStats radioStats() {
    return stats;
}
