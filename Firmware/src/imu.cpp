#include <Arduino.h>
#include <SPI.h>
#include <SensorQMI8658.hpp>
#include "imu.h"

// --- Pins (verified against LilyGo utilities.h, T_BEAM_S3_SUPREME block) ---
//
// IMPORTANT: this is the *sensor* SPI bus, which is NOT the radio's bus. The
// SX1262 has its own separate set of pins (SCLK 12 / MISO 13 / MOSI 11, CS 10).
// Two physically independent SPI buses live on this board; mixing them up will
// break LoRa later. See the SPIClass note below.
static const int IMU_SCK  = 36;
static const int IMU_MISO = 37;
static const int IMU_MOSI = 35;
static const int IMU_CS   = 34;   // chip select -- LOW = "I am talking to you"
static const int IMU_INT  = 33;   // data-ready interrupt (unused so far)

// The microSD slot hangs off this SAME bus with its own CS on IO47. Only one
// device may drive MISO at a time, so we park the SD card's CS HIGH
// (deselected) before talking to the IMU. Left floating, an un-initialized SD
// card can wake up and corrupt our transactions.
static const int SDCARD_CS = 47;

// Our own SPI instance on the HSPI peripheral, mirroring LilyGo's
// `SPIClass SDCardSPI(HSPI)` for this bus. We deliberately do NOT use the
// global `SPI` object: that one defaults to FSPI and is what the SX1262 driver
// will claim in milestone 5. Calling begin() on the global with sensor pins
// would silently re-point the radio's bus at the wrong GPIOs.
static SPIClass IMUSPI(HSPI);

static SensorQMI8658 imu;

// SensorLib reports acceleration in g (it scales raw counts by range/32768).
// Kandi's compass math wants a gravity vector in SI units, so convert once here
// rather than leaving unit juggling to every caller.
static const float G_TO_MPS2 = 9.80665f;

bool imuBegin() {
    // Deselect the SD card before any traffic goes out on the shared bus.
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    pinMode(IMU_INT, INPUT);

    // begin() starts the SPI peripheral on our pins, configures CS, and reads
    // WHO_AM_I to confirm a QMI8658 is actually answering. A false here means
    // the chip ID check failed: unpowered (ALDO1/ALDO2), miswired, or the bus
    // is being held by another device.
    if (!imu.begin(IMUSPI, IMU_CS, IMU_MOSI, IMU_MISO, IMU_SCK)) {
        return false;
    }

    // Accel: +/-8g full scale at 1000Hz. 8g is generous for a wrist device but
    // leaves headroom for the sharp impulses of a hand swing without clipping,
    // and gravity (1g) still resolves finely on a 16-bit ADC.
    imu.configAccelerometer(SensorQMI8658::ACC_RANGE_8G,
                            SensorQMI8658::ACC_ODR_1000Hz,
                            SensorQMI8658::LPF_MODE_0);

    // Gyro: not used by any Kandi math yet. Enabled so the sensor runs in its
    // normal 6DOF mode and to prove both halves of the chip respond.
    imu.configGyroscope(SensorQMI8658::GYR_RANGE_1024DPS,
                        SensorQMI8658::GYR_ODR_896_8Hz,
                        SensorQMI8658::LPF_MODE_0);

    imu.enableAccelerometer();
    imu.enableGyroscope();

    Serial.printf("QMI8658 online, chip ID 0x%02X\n", imu.getChipID());
    return true;
}

bool imuRead(ImuData &out) {
    out.ok = false;

    // Non-blocking: if the sensor hasn't finished a conversion we return
    // immediately rather than waiting, so the GPS UART keeps getting drained.
    if (!imu.getDataReady()) {
        return false;
    }

    float ax, ay, az, gx, gy, gz;
    if (!imu.getAccelerometer(ax, ay, az) || !imu.getGyroscope(gx, gy, gz)) {
        return false;
    }

    // Physical -> device frame remap (imu.h): swap first, then flips.
#if IMU_SWAP_XY
    { float t = ax; ax = ay; ay = t; }
    { float t = gx; gx = gy; gy = t; }
#endif
#if IMU_FLIP_X
    ax = -ax; gx = -gx;
#endif
#if IMU_FLIP_Y
    ay = -ay; gy = -gy;
#endif
#if IMU_FLIP_Z
    az = -az; gz = -gz;
#endif

    out.ax = ax * G_TO_MPS2;
    out.ay = ay * G_TO_MPS2;
    out.az = az * G_TO_MPS2;
    out.gx = gx;
    out.gy = gy;
    out.gz = gz;
    out.ok = true;

#if IMU_DEBUG_RAW
    int16_t accelRaw[3], gyroRaw[3];
    if (imu.getAccelRaw(accelRaw) && imu.getGyroRaw(gyroRaw)) {
        Serial.printf("  [imu raw] accel %6d %6d %6d  gyro %6d %6d %6d\n",
                      accelRaw[0], accelRaw[1], accelRaw[2],
                      gyroRaw[0], gyroRaw[1], gyroRaw[2]);
    }
#endif

    return true;
}
