#pragma once
#include <stdint.h>

// Thin driver for the on-board u-blox MAX-M10S GNSS receiver.
//
// The module is a plain NMEA-over-UART device: once powered it just talks,
// unprompted, at 9600 baud forever. All we do is open a hardware UART, shovel
// every byte into TinyGPSPlus, and read the parsed fields back out. No polling,
// no command protocol.
//
// Two separate things must be on before a single byte arrives:
//   1. the ALDO4 3.3V rail  -- done in initBoardPower() (power.cpp)
//   2. the module enable pin -- IO7, driven HIGH in gpsBegin() below
// Miss either and the UART stays silent. Both are handled; see gps.cpp.

// --- Raw NMEA debug ---
// Set to 1 to echo every byte the GPS sends straight to USB serial, before the
// parser sees it. Answers "is the module talking at all?" without implicating
// TinyGPSPlus. If this prints $GPGGA/$GNRMC lines, wiring and power are fine
// and any problem is in parsing; if it prints nothing, the problem is upstream
// (rail, enable pin, or RX/TX swapped).
#define GPS_DEBUG_RAW 0

struct GpsStatus {
    bool     valid;   // true when we hold a fix with a fresh position
    uint32_t sats;    // satellites currently used in the solution
    double   lat;     // degrees, valid only when `valid`
    double   lon;     // degrees, valid only when `valid`
    double   hdop;    // horizontal dilution of precision; lower is better, <2 is good
    uint32_t chars;   // total bytes received ever -- 0 means the module is silent
};

// Enable the module and open the UART. Returns false if no bytes arrived within
// a couple of seconds (i.e. the module looks dead) -- a fix is NOT required.
bool gpsBegin();

// Drain the UART into the parser. Must be called often (see note in gps.cpp
// about the 9600-baud vs. 256-byte-buffer race); never blocks.
void gpsPump();

// Snapshot of the current solution. Cheap; safe to call every loop.
GpsStatus gpsStatus();
