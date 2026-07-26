#pragma once
#include <stdint.h>

// Thin driver for the on-board QMI8658 6-axis IMU (3-axis accel + 3-axis gyro).
//
// This is the board's first SPI peripheral. Unlike the OLED and magnetometer,
// which are addressed devices sharing one pair of I2C wires, SPI selects a chip
// with a dedicated chip-select line: the bus (SCLK/MISO/MOSI) is shared, but
// each device gets its own CS pin pulled LOW to claim it. See imu.cpp.
//
// Only the accelerometer matters for Kandi -- tilt compensation needs a gravity
// vector. The gyro is read too because it costs nothing and confirms the sensor
// is fully alive, but nothing downstream consumes it yet.

// --- Raw debug ---
// Set to 1 to also print unscaled LSB counts straight from the sensor registers
// alongside the converted values. A sensor that reports a valid chip ID but
// returns all-zero or stuck counts is misconfigured; one that fails to init at
// all is unpowered or miswired. This toggle tells those two apart.
#define IMU_DEBUG_RAW 0

struct ImuData {
    bool  ok;              // true if this sample was actually read
    float ax, ay, az;      // acceleration, m/s^2 (includes gravity)
    float gx, gy, gz;      // angular rate, degrees/sec
};

// Bring up the SPI bus and the sensor. Returns false if the chip ID read fails
// (SensorLib verifies WHO_AM_I inside begin()).
bool imuBegin();

// Fetch one sample. Returns false when no new data is ready; `out.ok` mirrors it.
bool imuRead(ImuData &out);
