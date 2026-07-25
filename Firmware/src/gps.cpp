#include <Arduino.h>
#include <TinyGPS++.h>
#include "gps.h"

// --- Pins (verified against LilyGo's LoRaBoards.cpp / utilities.h,
//     T_BEAM_S3_SUPREME branch, not just the product listing) ---
//
// Names are from the ESP32's point of view, which is the opposite of the GPS
// module's: our RX pin is wired to the module's TX. Getting this backwards is
// the classic UART bug -- the symptom is total silence, identical to a dead
// power rail, which is exactly why GPS_DEBUG_RAW exists.
static const int GPS_RX_PIN  = 9;   // ESP32 receives here  <- GPS TX
static const int GPS_TX_PIN  = 8;   // ESP32 transmits here -> GPS RX
static const int GPS_EN_PIN  = 7;   // module enable / wakeup, active HIGH
static const int GPS_PPS_PIN = 6;   // 1 pulse-per-second output (unused for now)

static const uint32_t GPS_BAUD = 9600;   // MAX-M10S default

// UART1. The ESP32-S3 has three hardware UARTs; we deliberately take one
// instead of sharing the console. See the note in gpsBegin().
static HardwareSerial GPSSerial(1);

static TinyGPSPlus gps;

bool gpsBegin() {
    // The ALDO4 rail that feeds this module is already up (initBoardPower()).
    // This is a *second*, independent switch: the module's own enable line,
    // which also boots low. Rail on + enable low still equals a silent GPS.
    pinMode(GPS_EN_PIN, OUTPUT);
    digitalWrite(GPS_EN_PIN, HIGH);

    // 1PPS is a hardware timing pulse the module raises once per second once it
    // has a fix. Not used yet, but claim it as an input so nothing else drives it.
    pinMode(GPS_PPS_PIN, INPUT);

    // Bind UART1 to our pins. Argument order is (baud, config, rxPin, txPin) --
    // rx before tx, which reads backwards if you're used to writing "TX/RX".
    // SERIAL_8N1 = 8 data bits, no parity, 1 stop bit: standard NMEA framing.
    GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    // Wait briefly for the first byte. This proves the link (power + enable +
    // pin orientation + baud) without waiting on satellites, which can take
    // minutes on a cold start and needs a view of the sky.
    uint32_t start = millis();
    while (millis() - start < 2000) {
        if (GPSSerial.available()) return true;
        delay(10);
    }
    return false;
}

void gpsPump() {
    // encode() is a byte-at-a-time state machine: it accumulates an NMEA
    // sentence, verifies the checksum, and commits the parsed fields when a
    // complete sentence lands. Feeding it partial data is fine and expected.
    while (GPSSerial.available()) {
        char c = GPSSerial.read();
#if GPS_DEBUG_RAW
        Serial.write(c);     // raw passthrough: bytes go out exactly as received
#endif
        gps.encode(c);
    }
}

GpsStatus gpsStatus() {
    GpsStatus s;

    s.chars = gps.charsProcessed();
    s.sats  = gps.satellites.isValid() ? gps.satellites.value() : 0;

    // isValid() means "we have parsed a position at some point" -- it stays
    // true forever after the first fix, even if the antenna is unplugged. age()
    // is the ms since that value was last refreshed, so pairing the two gives
    // "we have a fix AND it is current" rather than "we once had a fix".
    s.valid = gps.location.isValid() && gps.location.age() < 5000;

    s.lat  = s.valid ? gps.location.lat() : 0.0;
    s.lon  = s.valid ? gps.location.lng() : 0.0;
    s.hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 0.0;

    return s;
}
