#pragma once

// Bring up the AXP2101 PMU on the T-Beam S3 Supreme and enable the on-board
// 3.3V peripheral rails (OLED, GPS, IMU/mag, LoRa, SD). Returns true if the
// PMU answered on its I2C bus.
//
// Call once in setup() BEFORE initializing any peripheral (OLED, GPS, ...):
// those chips are dark until their rail is powered. The ESP32 itself runs
// from an always-on rail, which is why serial works with no PMU init.
bool initBoardPower();
