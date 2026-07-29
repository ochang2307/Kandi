# Kandi — Offline Mesh-Networked Festival Wearable

A wrist-worn device that guides you to your friends at large festivals when
cell networks fail — no cellular, Wi-Fi, or internet dependency. Devices in a
group locate each other over a 915MHz LoRa mesh and display direction via a
ring of LEDs: glance-readable, screenless, and styled after kandi bracelets
rather than looking like a gadget.

## The problem

At large festivals (EDC-scale, 100k+ attendees), cellular networks saturate
within the first hour. Groups get separated with no way to reunite. Existing
products compromise: Totem Compass uses 2.4GHz (heavily absorbed by crowds)
in a bulky pendant; Crowd Compass got the radio right (sub-GHz LoRa) but
requires a phone app. Apple's offline finding only works at ~10-30m.

**Kandi's thesis: sub-GHz LoRa range + screenless wrist UX + cultural fit.**

## Status

- ✅ **Phase 1 — Design & architecture: complete.** Full engineering design
  doc covering radio selection (SX1262, LoRa "Long Fast", 915MHz), mesh
  protocol (managed flooding), GNSS (u-blox MAX-M10S + A-GNSS), power budget
  (~13-16hr projected on 400mAh), LED UX, bonding flow, and risk analysis.
  → [Design doc](https://docs.google.com/document/d/1ndvAmwJ7iS_GCs7uOu1srA6J-rN4ECKuPedQrF4hdPo/edit?usp=sharing)
- ✅ **Phase 2 core logic: complete.** The full navigation and networking
  stack, implemented and unit-tested in platform-independent Python and
  validated in simulation before any hardware existed.
- ✅ **Phase 2 hardware bring-up: complete.** All peripherals working on
  LilyGo T-Beam Supreme dev boards — power management, display, GNSS, IMU,
  magnetometer, addressable LED ring, and a confirmed LoRa link between two
  boards.
- ✅ **Navigation pipeline on hardware.** Live GPS fix → distance and bearing
  to a target → LED ring indication, running on-device.
- 🔄 **In progress:** magnetometer hard-iron calibration, porting the mesh
  protocol to firmware, and field range validation.

## Hardware

Prototyping on 3× **LilyGo T-Beam Supreme** — chosen because it carries the
exact parts specified in the design doc, making it a validation platform
rather than a stand-in:

| Subsystem | Part |
|---|---|
| MCU | ESP32-S3FN8 |
| LoRa radio | Semtech SX1262 (915MHz) |
| GNSS | u-blox MAX-M10S (GPS/GLONASS/Galileo/BeiDou) |
| IMU | QMI8658 6-axis |
| Magnetometer | QMC6310N |
| Display | SH1106 128×64 OLED |
| Power management | AXP2101 PMU, 18650 cell |
| Direction display | 16-LED WS2812 ring (external) |

Firmware is PlatformIO / Arduino-framework C++.

## Architecture

Core logic is written to be **hardware-independent**: the navigation math,
compass algorithms, and mesh protocol know nothing about any specific sensor
or radio. Hardware drivers are thin and separate. This let the entire
algorithmic stack be built and unit-tested in Python weeks before the dev
boards arrived, then ported to C++ largely unchanged — only the I/O edges
differ (real NMEA replaces synthetic coordinates, real SPI/I2C reads replace
synthetic sensor vectors, real LoRa transmit replaces a simulated one).

## Core logic (Python — reference implementation & test bed)

| Module | What it does | Verified by |
|---|---|---|
| `navigation.py` | Haversine distance + initial bearing between GPS coordinates | Real-world coordinate pairs and cardinal-direction cases |
| `compass.py` | **Tilt-compensated compass** — recovers true device heading from raw accelerometer + magnetometer at any wrist orientation | Synthetic round-trip tests: generates the sensor readings a known orientation would produce, then confirms the algorithm recovers that heading to <0.5° across all pitch/roll/heading combinations |
| `LEDLogic.py` | Maps target bearing + device heading → correct LED index | Boundary, rounding, and wraparound cases |
| `mesh.py` | Per-device mesh logic: packet structure, duplicate suppression, hop-limited relay decisions, group separation | All four receive-path outcomes |
| `network.py` | Multi-device flooding simulation with configurable topology | Relay across non-adjacent nodes, loop termination via dedup, hop-limit expiry |
| `device.py` | Integration loop: member positions in → full LED display state out | End-to-end pipeline under changing heading |

## Firmware (C++ — on hardware)

| Module | What it does |
|---|---|
| `power.cpp` | AXP2101 PMU init and peripheral rail management (every peripheral rail boots off) |
| `oled.cpp` | Hand-rolled SH1106 driver — framebuffer, page writes, built-in font |
| `gps.cpp` | MAX-M10S over hardware UART, NMEA parsed with TinyGPSPlus |
| `imu.cpp` | QMI8658 accelerometer + gyroscope over SPI |
| `mag.cpp` | QMC6310N magnetometer over I2C |
| `compass.cpp` | Ported tilt-compensated compass, running on live sensor data |
| `navigation.cpp` | Ported haversine distance + bearing |
| `ledlogic.cpp` | Ported bearing → LED index mapping |
| `radio.cpp` | SX1262 LoRa via RadioLib, non-blocking (interrupt-driven) |

Every ported module ships with the **original Python test vectors as
boot-time self-tests** — 19 cases covering distance, bearing, relative
bearing, LED mapping, and wraparound, all asserted against the verified
Python values on every power-up. A translation error surfaces at boot rather
than in a field test.

## Engineering notes

A few problems worth documenting, since debugging them was most of the work:

**Magnetometer squatting on the display's address.** The QMC6310N answers at
I2C `0x3C` — the address essentially every OLED library assumes by default.
Display writes were ACKed cleanly and did nothing. Resolved by disambiguating
at runtime via chip-ID read rather than hardcoding either address.

**Display library pathology.** On ESP32-S3, u8g2's hardware-I2C path left the
bus in a state where a full frame took 60-80 seconds — indistinguishable from
a hang. Raw `Wire` transfers on the same bus completed a full frame in ~24ms,
so the panel is driven by a purpose-written SH1106 driver instead (including
its 132-column RAM offset quirk and a Wire TX buffer overflow at full width).

**GNSS has two independent switches.** The PMU rail is necessary but not
sufficient — the module also has its own active-high enable pin that boots
low. Rail on with enable low produces a completely silent UART that looks
exactly like a dead rail or swapped RX/TX. A raw-byte debug mode now
distinguishes "not talking" from "talking but unparsed" in one toggle.

**Characterizing hard-iron distortion before correcting it.** Raw
magnetometer readings measured ~299 µT total against Earth's ~50 µT — a
~250 µT constant offset, mostly the steel in the 18650 holder. Since Earth's
*horizontal* component here is only ~20 µT, the offset predicted that heading
could only swing ±4.7° through a full rotation. A spin test confirmed it:
heading stayed trapped in a 9° band. Diagnosing this quantitatively, before
writing any calibration, meant the calibration target was known rather than
guessed.

**Retiring a risk with measurement.** The design doc flagged possible
magnetometer interference from LED ring switching currents. Measured with the
ring dark versus lit: no detectable shift. Risk closed, and the firmware
avoids an unnecessary LED-off sampling window.

## Roadmap

Magnetometer calibration → mesh protocol on hardware → two-device
"find your friend" field demo → miniaturized integrated unit → wrist form
factor → scaled field testing at progressively larger events.
