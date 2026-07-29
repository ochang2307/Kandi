#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "radio.h"

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

// Test packet: 4-byte magic + counter. The magic filters out other 915MHz
// LoRa traffic that happens to share these settings -- without it, any
// stranger's packet would bump our counter display.
struct __attribute__((packed)) TestPacket {
    char     magic[4];   // "KND1"
    uint32_t counter;
};
static const char PACKET_MAGIC[4] = {'K', 'N', 'D', '1'};

static RadioStats stats = {};
static uint32_t   txCounter = 0;      // next counter value to send
static bool       txInFlight = false; // startTransmit() issued, TX-done pending
static uint32_t   lastTxStart = 0;

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

#if IS_SENDER
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

void radioTick() {
    if (!stats.online) return;

#if IS_SENDER
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

        TestPacket pkt;
        memcpy(pkt.magic, PACKET_MAGIC, 4);
        pkt.counter = txCounter;

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
            stats.lastCounter = pkt->counter;
            stats.rssi = radio.getRSSI();
            stats.snr  = radio.getSNR();
            Serial.printf("LORA: RX #%lu  RSSI %.1f dBm  SNR %.1f dB  (n=%lu)\n",
                          (unsigned long)pkt->counter, stats.rssi, stats.snr,
                          (unsigned long)stats.rxCount);
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
