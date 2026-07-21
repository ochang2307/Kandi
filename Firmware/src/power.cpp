#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include "XPowersLib.h"
#include "power.h"

// --- PMU I2C bus ---
// The AXP2101 sits on the board's SECOND I2C bus (SDA=42, SCL=41), separate
// from the 17/18 bus the OLED and magnetometer share.
static const int PMU_SDA = 42;
static const int PMU_SCL = 41;

// XPowersLib quirk: the generic rail methods (enablePowerOutput, setProtected-
// Channel, ...) are PUBLIC on the XPowersLibInterface base but PROTECTED on the
// concrete XPowersAXP2101. So we keep the concrete object for begin() and drive
// the rails through an interface pointer -- the same split LilyGo's code uses.
static XPowersAXP2101      axp;
static XPowersLibInterface *PMU = &axp;

// Rail -> load map for this board, ported from LilyGo's Factory firmware
// (LilyGo-LoRa-Series, the T_BEAM_S3_SUPREME branch of beginPower()).
//
// Every on-board 3.3V load hangs off a *named* AXP2101 rail. The ESP32 runs
// from DCDC1, which the PMU powers automatically at boot -- that's why serial
// works with no PMU init. Every peripheral rail, though, boots OFF and stays
// off until enabled here. LilyGo's code has no rail labelled "OLED"; the OLED
// shares the 17/18 bus with the QMC6310 mag, both fed from the sensor rails
// below, so we bring up the full documented rail set.
bool initBoardPower() {
    // begin() starts Wire1 on the given pins and probes for the chip.
    if (!axp.begin(Wire1, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) {
        Serial.println("AXP2101 not found on Wire1 (SDA 42 / SCL 41)");
        return false;
    }
    Serial.printf("AXP2101 online, chip ID 0x%x\n", PMU->getChipID());

    // Never let the calls below accidentally cut the ESP32's own supply.
    PMU->setProtectedChannel(XPOWERS_DCDC1);

    // GPS (u-blox MAX-M10S)
    PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO4);

    // LoRa (SX1262)
    PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO3);

    // Cold boot only: power-cycle the sensor/SD rails so the QMC mag and SD
    // card come up clean instead of half-addressed on the shared bus.
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        PMU->disablePowerOutput(XPOWERS_ALDO1);
        PMU->disablePowerOutput(XPOWERS_ALDO2);
        PMU->disablePowerOutput(XPOWERS_BLDO1);
        delay(250);
    }

    // Sensors: IMU (QMI8658) + magnetometer (QMC6310). The mag shares the
    // 17/18 I2C bus with the OLED.
    PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO1);
    PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO2);

    // microSD
    PMU->setPowerChannelVoltage(XPOWERS_BLDO1, 3300);
    PMU->enablePowerOutput(XPOWERS_BLDO1);
    PMU->setPowerChannelVoltage(XPOWERS_BLDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_BLDO2);

    // M.2 / expansion interface rails
    PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
    PMU->enablePowerOutput(XPOWERS_DCDC3);
    PMU->setPowerChannelVoltage(XPOWERS_DCDC4, XPOWERS_AXP2101_DCDC4_VOL2_MAX);
    PMU->enablePowerOutput(XPOWERS_DCDC4);
    PMU->setPowerChannelVoltage(XPOWERS_DCDC5, 3300);
    PMU->enablePowerOutput(XPOWERS_DCDC5);

    // Rails this board doesn't use -- keep them off.
    PMU->disablePowerOutput(XPOWERS_DCDC2);
    PMU->disablePowerOutput(XPOWERS_DLDO1);
    PMU->disablePowerOutput(XPOWERS_DLDO2);
    PMU->disablePowerOutput(XPOWERS_VBACKUP);

    // Charge-status LED, matching LilyGo factory behavior.
    PMU->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

    // Liveness readout for bring-up.
    Serial.printf("  VBUS %s, battery %s, Vbat %umV\n",
                  PMU->isVbusIn() ? "present" : "absent",
                  PMU->isBatteryConnect() ? "connected" : "none",
                  PMU->getBattVoltage());
    return true;
}
