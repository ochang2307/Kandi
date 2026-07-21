#include <Arduino.h>
#include <Wire.h>
#include "power.h"
#include "oled.h"

int counter = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Kandi board 1 boot");

    // Power up the board rails FIRST. The OLED (and GPS, IMU, mag, LoRa) sit
    // on AXP2101-switched 3.3V rails that boot OFF.
    if (!initBoardPower()) {
        Serial.println("PMU init failed -- OLED will stay dark");
    }
    delay(100);

    // OLED + magnetometer share this bus: SDA=17, SCL=18.
    Wire.begin(17, 18);
    Wire.setClock(400000);   // full 8-page frame ~24ms at 400kHz (measured clean)

    // Scan the 17/18 bus and disambiguate 0x3C/0x3D: on this board the QMC6310N
    // magnetometer and the SH1106 OLED both live in that address pair. Read
    // register 0x00 -- the mag returns its chip id 0x80, the OLED does not.
    // (The OLED is at 0x3D here; the mag squats on the usual 0x3C.)
    uint8_t oledAddr = 0;
    Serial.print("I2C scan (17/18):");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() != 0) continue;
        Serial.printf(" 0x%02x", addr);
        if (addr == 0x3C || addr == 0x3D) {
            Wire.beginTransmission(addr);
            Wire.write((uint8_t)0x00);
            Wire.endTransmission();
            Wire.requestFrom((int)addr, 1);
            uint8_t id = Wire.available() ? Wire.read() : 0xFF;
            if (id == 0x80) { Serial.print("(mag)"); }
            else            { Serial.print("(oled)"); oledAddr = addr; }
        }
    }
    Serial.println();

    if (oledAddr) {
        Serial.printf("OLED at 0x%02x, init\n", oledAddr);
        oledBegin(oledAddr);
    } else {
        Serial.println("no OLED found on 17/18");
    }
}

void loop() {
    oledClear();
    oledText(0, 1, "KANDI");
    char line[24];
    snprintf(line, sizeof(line), "alive %d", counter);
    oledText(0, 3, line);
    oledShow();

    Serial.println(line);
    counter++;
    delay(1000);
}
