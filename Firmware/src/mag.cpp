#include <Arduino.h>
#include <Wire.h>
#include <SensorQMC6310.hpp>
#include "mag.h"

// Shared with the OLED. Pins confirmed against LilyGo's utilities.h
// (I2C_SDA 17 / I2C_SCL 18 in the T_BEAM_S3_SUPREME block).
static const int MAG_SDA = 17;
static const int MAG_SCL = 18;

static SensorQMC6310 mag;

bool magBegin() {
    // QMC6310N_SLAVE_ADDRESS is 0x3C. The 'U' variant of this part answers at
    // 0x1C instead -- this board is populated with the N, which is exactly why
    // it collides with the usual OLED address.
    if (!mag.begin(Wire, QMC6310N_SLAVE_ADDRESS, MAG_SDA, MAG_SCL)) {
        return false;
    }

    // SensorLib's begin() calls Wire.begin() internally, which resets the bus
    // clock to the 100kHz default. Put it back to 400kHz or every OLED frame
    // silently becomes 4x slower (~24ms -> ~96ms) -- not a hang, just a sluggish
    // display that would be annoying to trace back to this line.
    Wire.setClock(400000);

    // The part boots in SUSPEND and measures nothing until told otherwise.
    //   CONTINUOUS_MEASUREMENT - free-running; readData() always has a sample
    //   FS_8G                  - Earth's field is only ~0.25-0.65 G, so this is
    //                            mostly headroom for hard-iron offset from the
    //                            board's own magnets/traces without saturating
    //   100 Hz                 - we sample at 1Hz now, but milestone 3's compass
    //                            wants headroom; still cheap
    //   OSR_8                  - oversampling averages down noise, which matters
    //                            a lot for a stable heading
    if (!mag.configMagnetometer(OperationMode::CONTINUOUS_MEASUREMENT,
                                MagFullScaleRange::FS_8G,
                                100.0f,
                                MagOverSampleRatio::OSR_8,
                                MagDownSampleRatio::DSR_1)) {
        Serial.println("QMC6310 found but configuration failed");
        return false;
    }

    SensorInfo info = mag.getSensorInfo();
    Serial.printf("QMC6310N online at 0x%02X, sensitivity %.6f G/LSB\n",
                  info.i2c_address, mag.getSensitivity());
    return true;
}

bool magRead(MagData &out) {
    out.ok = false;

    MagnetometerData data;
    if (!mag.readData(data)) {
        return false;
    }

    // SensorLib reports Gauss; microtesla is the conventional unit for compass
    // work and keeps the numbers human-sized (Earth's field is ~25-65 uT).
    // Axis flips (mag.h) apply HERE, at the physical->device frame boundary:
    // everything downstream (calibration capture included) sees the remapped
    // frame, so offsets stay consistent with what the compass consumes.
    float px = MagnetometerUtils::gaussToMicroTesla(data.magnetic_field.x);
    float py = MagnetometerUtils::gaussToMicroTesla(data.magnetic_field.y);
    float pz = MagnetometerUtils::gaussToMicroTesla(data.magnetic_field.z);
#if MAG_SWAP_XY
    { float t = px; px = py; py = t; }
#endif
    out.mx = px * (MAG_FLIP_X ? -1.0f : 1.0f);
    out.my = py * (MAG_FLIP_Y ? -1.0f : 1.0f);
    out.mz = pz * (MAG_FLIP_Z ? -1.0f : 1.0f);
    out.overflow = data.overflow;
    out.ok = true;

#if MAG_DEBUG_RAW
    Serial.printf("  [mag raw] %d %d %d  overflow=%d\n",
                  data.raw.x, data.raw.y, data.raw.z, (int)data.overflow);
#endif

    return true;
}
