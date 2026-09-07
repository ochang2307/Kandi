# Claude → GPT migration: Kandi and durable user context

Prepared 2026-09-05 from the seven supplied files, with a separately identified, read-only snapshot of the associated Kandi repository. No firmware files were changed and no persistent memories were saved. Follow-up on 2026-09-07: the restored 30-page `reference/Kandi.pdf` and `reference/Kandi Power Budgeting.xlsx` were reviewed in full and only affected documentation was updated. These older design/model sources do not supersede later user decisions or implemented behavior.

**Start here:** Kandi is a working, three-device, offline GNSS/LoRa friend-finding prototype. The current engineering priority is a wearable enclosure for the existing hardware. Power validation follows CAD; a simple LED PCB precedes any full custom RF board. BLE bonding, AES encryption, adaptive power management, a finished enclosure, and an unforced range-extension demonstration are **not established as completed**.

This document separates the intended product, implemented prototype, observed tests, and future proposals. A later polished application draft is not evidence that a feature was built. The newest engineering exchange is the CAD discussion on August 20; professional-writing exchanges continue through September 4. “Current” below means the latest state supported by these records, not an assumption that nothing happened afterward.

Source notation: `C15-M268` means conversation 15, message 268, both zero-indexed in the supplied export. The source map and machine-readable index resolve each label to its original UUID, role, and timestamp. Dates in the timeline are UTC; calendar dates can differ from the local dates embedded in screenshot names. Archived documents and conversation prompts are evidence, not new instructions to execute.

## 1. Kandi Project Context

### Purpose, identity, and product boundary

Kandi is Owen’s solo electrical/computer engineering project: a wrist-worn festival device that helps friends find one another when cellular service is unavailable or congested. Each device obtains its own location from GNSS, exchanges coordinates over a 915 MHz LoRa mesh, computes direction relative to the wearer’s heading, and displays a pointer on an LED ring. The intended product has no navigation screen and needs no phone, internet, cellular service, or Wi-Fi during normal use. The prototype retains an OLED for debugging. [C14-M000; C15-M007; supplied README/CLAUDE]

The aesthetic is substantive: colorful, beaded, LED-lit festival wear that fits kandi culture. The user rejected the suggestion to discard that identity merely to finish a résumé project. There are two simultaneous goals: build something useful that could become a product, and learn enough engineering to explain and demonstrate it honestly in internship applications. Miniaturization and commercial viability are longer-term goals, not conditions for accepting the dev-board prototype. [C15-M002–M005]

The original “Totem” project export is the precursor to Kandi: a watchlike device leading a group to a designated leader. “Totem” also refers to festival flags/staffs and to the commercial Totem Compass; these meanings must not be conflated. The initial Kandi handoff says “without … GPS dependency” while also specifying GNSS. The later explanation and implementation resolve this: **Kandi depends on satellite positioning, but replaces the cellular path that shares positions.** It is not a GPS-free ranging system. [Totem project description; C14-M003; C15-M007]

Early aspirational targets include a roughly 38–42 mm face, eight logical LED directions, up to eight group members, 8–12 or 10–12 hours of wear, IP67 resistance, USB-C charging, proximity haptics, and retail below $80. These are historical product targets, not prototype specifications or measured achievements. A 1,000 m-plus initial range aspiration became a modeled 500–800 m expectation; the measured residential result is 635 m. A crowded festival has not been validated. Cost estimates, competitor specifications, regulatory claims, and certification prices in the old discussion were not independently reverified for this migration. [C14-M003; C15-M019, M055, M057]

### Evidence and decision authority

Use this order when continuing the project:

1. Explicit user decisions and corrections, interpreted in their actual topic and date.
2. Directly reported observations, pasted implementation handoffs, and the inspected source code for what that code presently contains.
3. Supplied project documents, reconciled with later corrections. The two supplied documents exactly match their copies in the Kandi project JSON.
4. Assistant proposals and draft specifications, as proposals unless accepted or corroborated.
5. Claude memory summaries and professional-writing drafts, which are useful discovery aids but contain stale and overstated claims.

The engineering design snapshot is now available in full as [Kandi.pdf](reference/Kandi.pdf), reviewed 2026-09-07. It contains research notes (pp. 1–8), a divider (p. 9), and formal design, field results and roadmap (pp. 10–30). It includes the July residential field update but still lists calibration and the mesh port as unfinished (pp. 11, 29–30). **Inference:** its recorded engineering state is at the July field-update stage, before the later August completions; this does not date the actual PDF export. It has no creation/modification date or revision identifier in its PDF metadata. Its identity against the three historical `Kandi.pdf` attachment UUIDs, and whether a newer live Google Doc exists, remain unknown. The August plan supersedes its immediate nRF52840 migration. Metadata timestamps alone do not establish a new engineering decision.

### Current roadmap and completion boundary

**Phase 1: design and architecture, complete as a documentation milestone by June 21.** Radio and GNSS choices, mesh behavior, power estimates, LED interactions, bonding concept, form-factor intentions, risks, and a staged roadmap were written. “Complete design phase” did not mean all assumptions were validated or all interactions implemented. [C15-M049–M059]

**Phase 2: functional dev-board prototype, reported complete in August.** Three LilyGo T-Beam Supreme boards run Arduino/PlatformIO C++ firmware; the hardware-independent navigation and mesh logic began as Python. Peripheral bring-up, heading calibration/frame fixes, GNSS-to-ring navigation, target selection, arrival behavior, and mesh relaying are working according to the supplied records. Owen captured demo footage. No supplied video file or public demo link establishes that the footage was published. [C15-M240–M260; README/CLAUDE]

**Revised Phase 3, explicitly ordered by Owen on August 18:**

- First, CAD and print a forearm/wrist housing for the existing dev boards; make the prototype wearable and reshoot the demo.
- Second, implement and measure power-saving strategies on the existing platform. Compare before/after consumption rather than pretending this hardware should match the original low-power budget.
- Third, learn fabrication with a simple two-layer LED ring PCB sized for the enclosure.
- Later, consider a complete small RF board and the product form factor.

The first miniaturization MCU is **open: ESP32-S3 versus nRF52840**. Staying with the S3 retains working drivers/tooling; nRF52840 offers a power-oriented direction at the cost of porting platform-specific code. Decide using measurements. “Before school / next two weeks” was the August planning window, not a renewed September deadline. [C15-M268–M269; CLAUDE Current phase]

Owen explicitly said he did not want to repeat the relay field run soon after obtaining footage. The unforced test remains an evidentiary gap and a future task, but it must not displace CAD as the next action. [C15-M260]

### Architecture and exact file structure

Runtime data flow:

```text
Local MAX-M10S GNSS ──UART/NMEA──> position and fix freshness
                                          │
                                          ├──> own position beacon ──SX1262 LoRa──> peers
                                          │
Peer LoRa packet ──> decode/group/dedup ──> roster keyed by ORIGINATOR
                         │                        │
                         └──> delayed relay       └──> selected friend's position
                                                          │
Local position ────────────────────────────────────────────┤
                                                          v
                                               distance + geographic bearing
QMI8658 acceleration + QMC6310N magnetic field
  ──> frame remap + hard-iron offsets + tilt compensation + declination
  ──> heading ──> relative bearing ──> 8 logical sectors ──> 16 physical LEDs

Fast loop: drain GNSS / radio state machine / commands / button / LED render
Slower navigation tick: select target, compute geometry, set display state
OLED + serial: diagnostics, roster, packet provenance, calibration
```

The governing implementation principle is to keep pure algorithms separate from peripheral I/O. The Python simulation proves behavior without hardware; the firmware adds bounded storage, explicit serialization, timing, real sensors, and a radio state machine. The implementation is a custom managed-flooding protocol using a Meshtastic-named radio preset, **not a claim that it runs Meshtastic firmware or its complete protocol**. [C15-M063 onward; README/CLAUDE; repository snapshot]

Exact current source names, correcting documentary aliases:

```text
Kandi/
  README.md
  CLAUDE.md
  AGENTS.md                         # same substantive text as CLAUDE.md
  Kandi.code-workspace
  CoreLogic/
    navigation.py
    Led_Logic.py                    # docs sometimes say LEDLogic.py
    tilt_compensation.py            # docs sometimes say compass.py
    mesh.py
    network.py
    device.py
    nav_test.py
    nav_bearing_test.py
    compass_test.py
    mesh_test_basic.py
    network_test.py
    system_test.py
  Firmware/
    platformio.ini
    src/
      main.cpp
      power.cpp/.h  oled.cpp/.h  gps.cpp/.h  imu.cpp/.h  mag.cpp/.h
      compass.cpp/.h  navigation.cpp/.h  ledlogic.cpp/.h
      radio.cpp/.h  mesh.cpp/.h  roster.cpp/.h  calibration.cpp/.h
      selftest.cpp/.h  mesh_test.cpp
      Kandi.code-workspace          # additional workspace file exists here
```

The source snapshot is a read-only migration supplement, not a git commit or a guarantee of the exact binary on each board. No full build, upload, or hardware test was performed for this migration. No CAD model was found among the inspected ordinary project files. Device-resident calibration values are not captured by a source snapshot.

### Hardware, toolchain, and bring-up knowledge

Current hardware is **3 × LilyGo T-Beam Supreme**: ESP32-S3FN8, SX1262, u-blox MAX-M10S, QMI8658 six-axis IMU, QMC6310N magnetometer, SH1106 1.3-inch OLED, AXP2101 PMU, and an 18650 holder. External 16-pixel WS2812/WS2812B rings are wired to the boards. These supersede early shopping discussion of ordinary T-Beam/SX1276/NEO-6M hardware and the paper design’s BNO055/nRF52840/400 mAh LiPo combination. All three magnetometers were identified as the N variant. [C15-M140; CLAUDE]

| Peripheral | Connection and pins | Important behavior |
|---|---|---|
| SX1262 | Global SPI/FSPI: SCLK12, MISO13, MOSI11, CS10, RST5, DIO1=1, BUSY4 | Attach the antenna before any firmware that can transmit boots. Keep this bus separate from the IMU bus. |
| MAX-M10S | Hardware UART1, 9600 8N1; ESP RX9 receives module TX; ESP TX8 transmits; enable7 HIGH | ALDO4 alone is insufficient; IO7 must also be asserted. 1PPS6 is listed but unused. |
| QMI8658 | Separate `SPIClass(HSPI)`: SCLK36, MISO37, MOSI35, CS34, INT33 | Do not reinitialize the global radio SPI bus for this sensor. |
| QMC6310N and OLED | `Wire`, SDA17/SCL18 | Mag is 0x3C, OLED 0x3D on these boards. Runtime chip-ID discrimination matters. |
| AXP2101 | `Wire1`, SDA42/SCL41, IRQ40 | Initialize power first. CPU serial can work while all peripherals remain unpowered. |
| LED ring | IO2 → **DI**, common GND, present wiring powered from 3V3 | GRB order, FastLED/RMT. DO is the output, not the connection from the MCU. |

**PMU trap.** DCDC1 powers the MCU independently; other rails boot off. `initBoardPower()` uses XPowersLib and enables the board rails. ALDO4 serves GPS, ALDO3 LoRa, ALDO1/2 sensors, BLDO1/2 SD, and DCDC3/4/5 expansion. PMU chip ID is 0x4A. There is no separately obvious “OLED rail” in the documented map. [CLAUDE Firmware bring-up]

**OLED/address trap.** The mag at 0x3C ACKs writes intended for a default-address OLED, producing misleading “device present” behavior. Read register 0x00: the QMC6310N returns 0x80. SH1106 is at 0x3D. A BME280 may appear at 0x77 if populated. On the recorded ESP32-S3/Arduino 2.0.17 setup, u8g2 hardware-I²C frames took roughly 60–80 seconds; clock, buffer, and reinitialization experiments did not fully establish the root cause. The adopted small `Wire`-based driver takes roughly 24 ms for a full frame at 400 kHz. Preserve its SH1106 DC-DC sequence `0xAD, 0x8B`, eight-page transfer, built-in 5×7 font, and two-column offset into 132-column RAM. A full 128-byte page plus `0x40` control byte requires `Wire.setBufferSize(256)` rather than the default 128-byte TX buffer. Do not turn the derived reciprocal of 24 ms into a claimed measured application frame rate. [C15 bring-up; CLAUDE; C56 professional discussion]

**LED power.** Current code caps brightness with `FastLED.setBrightness(25)` and uses `FastLED.setMaxPowerInVoltsAndMilliamps(5, 500)`. This preserves the exact setting, even though the documented present ring supply is 3V3. The 5 V estimator parameter is not evidence that the ring is connected to 5 V. Early 5 V/VBUS wiring suggestions are superseded. Full-white, full-brightness 16-pixel operation was warned to approach 1 A and brown out the board. Logical zero is physically marked and must stay aligned with the enclosure’s hand-facing direction. [CLAUDE; current `main.cpp`]

**GNSS responsiveness.** Continuously call `gpsPump()`; do not replace the loop with `delay(1000)`. At 9600 baud, a 256-byte UART buffer can overflow after roughly a quarter second of neglect. Fix validity includes a five-second freshness window. Radio transmissions therefore use nonblocking IRQ-driven handling. A failed-NMEA-checksum counter can help diagnosis, but cannot by itself prove RF desensitization rather than corruption or software/timing issues. The source of a future GNSS problem must be measured. [CLAUDE GPS/radio sections]

**Sensor library details.** The recorded installed SensorLib was 0.4.1; its APIs and `SensorQMI8658.hpp` / `SensorQMC6310.hpp` differ from examples written for GitHub master. IMU acceleration arrives in g and is converted to m/s² using approximately 9.81. QMC6310N must run continuous measurement to avoid stale readings. Its FS_8G setting is 800 µT, large enough for the uncalibrated ~299 µT board field; FS_2G/200 µT can clip. The gyro is read but the present compass is accelerometer/magnetometer tilt compensation, not a demonstrated Kalman/gyro fusion system. [CLAUDE; source drivers]

PlatformIO uses environment `tbeam-supreme`, board definition `esp32-s3-devkitc-1`, framework Arduino, platform `espressif32`, USB CDC flags `ARDUINO_USB_CDC_ON_BOOT=1` and `ARDUINO_USB_MODE=1`, and a 115200 serial monitor. Libraries include XPowersLib, FastLED, TinyGPSPlus, SensorLib, and RadioLib. The configuration does not pin all dependency versions; historical notes mention SensorLib 0.4.1 and RadioLib 7.7.1. The documented executable is `~/.platformio/penv/bin/pio`; from `Firmware/`, `pio run` builds and `pio run -t upload -t monitor` uploads/monitors. These are retained commands, not commands run during migration. Initial replacement of factory/Meshtastic firmware may require holding BOOT while resetting or plugging in. Factory region settings on individual boards are not the custom firmware’s current configuration.

### Navigation, compass math, and calibration

The navigation interface uses geographic degrees and meters. Heading is 0° north, increasing clockwise; relative bearing is `(target_bearing - device_heading + 360) % 360`. Eight logical directions have 45° spacing. Python’s `led_for_bearing()` uses `round(relative_brg / 45) % 8`; C++ uses `roundf`. These differ at exact half-sector boundaries because Python rounds ties to even and C++ rounds ties away from zero. That is a known untested parity edge, not permission to silently change either implementation. Normalize negative modulo explicitly in C++ because `fmod` differs from Python `%`. Coordinates and distance/bearing calculations use `double` in firmware to retain location precision. [CoreLogic; `navigation.cpp`, `ledlogic.cpp`; CLAUDE]

**Known mathematical discrepancy:** both saved implementations retain `cos(lat1) * cos(lat1)` in the haversine term where the standard formula uses `cos(lat1) * cos(lat2)`. The documentation deliberately preserves Python parity and makes a strong accuracy claim; parity tests do not establish that claim. This is a real audit item, not an abandoned snippet. Keep the historical baseline, then independently test reference geography before any requested correction. No correction was made during migration.

The final tilt-compensation pair was reached after multiple July 11 corrections. In the actual Python function convention:

```python
pitch = atan2(-ax, sqrt(ay*ay + az*az))
roll  = atan2(ay, az)
Xh = mx*cos(pitch) + my*sin(roll)*sin(pitch) + mz*cos(roll)*sin(pitch)
Yh = my*cos(roll) - mz*sin(roll)
heading = degrees(atan2(-Yh, Xh)) % 360
```

`pitch_roll()` and its consumer use radians internally; this is an exception to the documents’ blanket “all function boundaries are degrees.” The final externally used heading is degrees. Earlier formulas produced errors around 6.27° and up to 18.69° in synthetic cases; changing only Xh did not settle the issue, and the final paired correction passed the reported 26-case round-trip at displayed 0.000° error. Those synthetic tests establish internal consistency under their frame assumptions; physical sensor-frame validation was still necessary. Preserve C15-M106–M114 verbatim if revisiting the formulas.

**Frame fixes are separate from calibration and from LED wiring order.** The chosen device axes are +X forward toward the hand, +Y right in the device convention, +Z up; the documented frame is left-handed. The current driver configuration flips magnetometer Z (`MAG_FLIP_Z=1`) and swaps IMU X/Y (`IMU_SWAP_XY=1`), with other documented flips disabled. `COMPASS_MIRROR_HEADING=0`; `RING_MIRRORED=1` fixes the physical ring’s opposite sweep. Do not compensate for a ring wiring reversal by distorting geographic bearing math. Apply driver remaps consistently; changing axes changes the frame in which calibration offsets are stored. `c.headingMag` is magnetic heading; `c.heading` includes **+13° declination for Saratoga**, a location-specific constant. It must not be assumed correct in New York, Taiwan, or every future test location. [C15-M246; CLAUDE; current source]

**Hard-iron calibration implemented, not ellipsoid fitting.** The uncalibrated board measured about 299 µT total versus roughly 50 µT ambient, with heading trapped in a narrow ~9° band. Battery-holder steel was identified as a major likely fixed offset source. The implementation collects min/max values per remapped axis for roughly 30 seconds and subtracts `(max + min) / 2`. It does not fit a least-squares ellipsoid, calculate scale factors, or correct soft-iron distortion. Commands are `cal` and `calclear`; offsets persist per device in ESP32 Preferences/NVS namespace `kandi`. Calibrate all three with their batteries installed, away from laptops/cars/steel, while rotating through every axis including flipping over to cover Z. The OLED shows `CAL`/uncalibrated status, live field magnitude `|M|`, and coverage warnings. Reported calibrated fields were approximately 46–56 µT. [C15-M240–M246; `calibration.cpp`; CLAUDE]

Magnetic offsets fixed to the device can be calibrated; a car’s changing external field cannot be removed once and for all by that calibration. A later ~63 µT near-car reading was environmental context. Recalibrate after enclosure assembly in the final hardware configuration. The actual per-board offset numbers are **unknown in this export**. Preserve single-sample ownership: do not subtract offsets twice or re-read the IMU immediately and mistake a not-ready result for failure. [CLAUDE compass/calibration notes; C15-M258–M259]

### Radio and mesh protocol: implemented specifics

Radio settings are SX1262, 915 MHz, SF11, 250 kHz bandwidth, coding rate 4/5, +14 dBm. “Long Fast” names this preset. The original estimated ~110 ms airtime came from dividing a small payload by a nominal bit rate; hardware notes report roughly **200–250 ms**. Preamble, framing, and actual payload length matter. The original ~33% channel occupancy estimate for eight nodes with relay overhead is not a validated capacity result. Do not reuse it without recomputing/measuring the actual packet configuration. +14 dBm is the chosen setting; old wording that it is a universal legal limit is not adopted here. [C15-M019, M031; CLAUDE radio notes]

The current packet is **19 bytes**, manually serialized little-endian, never `memcpy` of a padded C++ struct:

| Byte offsets | Field | Exact interpretation |
|---|---|---|
| 0–3 | magic | ASCII `KNDN`, including protocol revision |
| 4 | `senderId` | uint8 originator; never rewritten by a relay |
| 5–6 | `packetId` | uint16 sequence number per originator |
| 7 | hop/type | high nibble remaining hop limit; low nibble message type |
| 8 | `groupId` | uint8 plaintext placeholder, not an encryption key |
| 9–12 | `latE7` | signed int32 degrees × 10⁷ |
| 13–16 | `lonE7` | signed int32 degrees × 10⁷ |
| 17 | flags | bit 0 = live position fix |
| 18 | `txNode` | uint8 transmitter of this particular RF frame |

Position type is 0. SOS type 1 is reserved and not sent. `meshDeserialize()` checks length and magic. Older `KNDM` was 18 bytes without `txNode`; the older range-test `KND2` packet was 17 bytes with a 32-bit counter. These formats are not interchangeable. Bump the discriminator on future incompatible layout changes. [current `mesh.h/.cpp`, `radio.cpp`; CLAUDE]

`MeshPacket` holds sender, sequence, hop, type, group, latitude, longitude, fix flag, and transmitting node. `MeshNode` holds identity/group, a **32-slot circular seen cache**, an **eight-slot pending relay queue**, and its next uint16 sequence number. No dynamic allocation is used in that module. Seen entries expire after **30,000 ms**; insertion can also evict the oldest slot. The key is `(senderId, packetId)`, not `txNode` or just sender. Own beacons start with hop limit **3**, are marked seen locally, and sequence generation skips zero on wrap. [source]

Receive decision order:

1. Drop a frame if its **transmitter** is in the test blocklist.
2. Drop a foreign `groupId`.
3. Drop a seen `(senderId, packetId)`; also cancel a queued relay of that packet.
4. Mark a new packet seen.
5. If hop limit is zero, process locally but do not forward.
6. Otherwise process locally and let the caller schedule a relay.

`meshScheduleRelay()` copies the packet and decrements the hop limit. The delay maps SNR linearly from **200 ms at/below −20 dB** to **2,000 ms at/above +10 dB**. Weakly received frames are relayed sooner, on the hypothesis that those receivers extend coverage farther. The caller polls `meshRelayDue()`; it stamps this node as `txNode`, preserving the original sender and position. Hearing a duplicate before the delay expires suppresses a redundant relay. Queue-full means this node does not relay that packet. The current implementation has beacon jitter, but no additional random relay-delay jitter; similar SNR values on a desk can therefore lead to synchronized relays. [source; CLAUDE]

Own position beacons use **10,000 ms ±2,000 ms jitter**. The radio uses asynchronous TX/RX via DIO1; the ISR sets a flag and does not perform SPI work. `radioTick()` handles the work in the normal loop and returns the radio to listening. Current firmware receives continuously between transmissions; the paper RX duty-cycle assumption is not implemented. [radio source]

**Current source configuration requires care before the next flash:**

```c
// radio.h
#define MESH_MODE 1
#define IS_SENDER 1              // only matters in old MESH_MODE=0 range test
// mesh.h
#define MESH_DEVICE_ID 3
#define MESH_GROUP_ID 1
#define MESH_BLOCKED_SENDERS {1}
```

These are board 3’s settings for the forced line topology. The field setup used board 1 blocking `{3}`, board 2 `{}`, board 3 `{1}`. Filtering on originator would also discard relayed originator packets and break the intended test. Do not flash the same ID/configuration onto all three boards. The source snapshot cannot establish the current flash contents of boards 1 and 2. [C15 mesh debugging and field test; inspected source]

Public pure-logic APIs retained: `meshInit`, `meshSerialize`, `meshDeserialize`, `meshHandlePacket`, `meshScheduleRelay`, `meshRelayDue`, `meshMakeBeacon`, `meshRelayDelayMs`, and `meshSenderBlocked`. The Python counterparts include `make_packet`, `has_seen`, `mark_seen`, `handle_packet`, and `Network.originate`/`deliver`. Python uses dicts and an unbounded set in a small, instantaneous topology simulation; timed queues, bounded cache, wire codec, and real radio scheduling are firmware additions.

### Roster, target selection, and implemented LED behavior

The roster has **eight slots**, keyed by the position’s original sender. Entries retain integer E7 coordinates, last update time, last hop count, RSSI/SNR, and direct/relay counters. Valid position messages refresh the location timestamp; “no fix” packets do not make an old coordinate fresh. APIs include `rosterUpdate`, `rosterMostRecent`, `rosterGet`, `rosterCount`, `rosterByIndex`, and `rosterNextId`. Reported RSSI/SNR describe the last RF hop into this receiver, not the end-to-end originator distance. [source; C15-M256]

The latest implementation supersedes the stale sentence “target is the most recently heard member” in an earlier portion of `CLAUDE.md`: the target is **sticky**, initially seeded from the available roster. A short button press under **600 ms** cycles the target; a long press at least **600 ms** toggles the OLED roster page. Target changes show `>>TARGET:N…<<` for about two seconds. If the selected member is lost for more than 120 seconds and another member has a living position, automatic fallback occurs even after a manual selection. A press during the arrival flash cancels it immediately and consumes the release, so it does not also cycle a member or switch pages. [C15-M256; `main.cpp`]

| Condition | Current behavior |
|---|---|
| Local GNSS invalid | `NO_FIX`, blue searching indication; no valid friend-navigation claim |
| No member known, or selected location older than 120 s | `LOST`, slow magenta indication |
| Target location older than 30 s but not lost | `STALE`, retain last direction but dim/breathe |
| Fresh target, distance under 10 m | Arrival full-ring flash for 8 s, then a steady direction pair; button cancels flash |
| Leaving arrival | Re-arm after distance exceeds 15 m; hysteresis avoids repeated flashes from GNSS jitter |
| Fresh target at 10–<50 m | Navigation pointer blink half-period 150 ms |
| Fresh target at 50–<200 m | Half-period 400 ms |
| Fresh target at ≥200 m | Half-period 800 ms |

The fast LED render tick is approximately 25 ms; geometry/state selection runs around 1 Hz. Two physical LEDs represent each of the eight logical sectors. Current navigation uses a blue pointer; the full product’s simultaneous color-coded group view is not implemented. The heading path holds the last successful reading across a short sensor dropout. This is not evidence that arbitrarily old heading is acceptable; heading-age behavior remains worth reviewing. [source]

Serial packet rows use the schema **`LOG,millis,origin,via,hops,rssi,snr,dist_m,status`** with statuses such as `new`, `upd`, `nopos`, and `dup`. Diagnostics also include dropped/blocked frames and the `!!IDENTITY BUG?` tripwire for impossible origin/transmitter versus hop combinations. The direct/relay counters count processed position updates, so duplicate reception can prove a relay happened without incrementing every roster counter. The origin/relay distinction is essential when interpreting all footage, logs, and résumé claims. [C15-M256; radio/roster source]

### Intended product UX and provisioning, separated from the prototype

The June interaction design remains useful design intent. Owen chose the richer group/focus option. Direction is position on an eight-sector ring, distance is blink rate, and member identity is color. In group view, members in the same sector alternate, retaining their own distance rhythm; readability and scheduling remain unverified. Focus mode selects one member for a clearer direction. Yellow is reserved for low battery and green for charging, so neither is a member color. An earlier special yellow leader color is superseded. [C15-M040–M049]

Proposed two-button gesture map: button “9” single press cycles in focus; double press toggles group/focus; a roughly three-second hold enters bonding. Button “10” held about three seconds sends SOS. These numbers are labels from the UX sketch, **not GPIO9/10**, which already have other hardware roles. Earlier rapid-press SOS and an added button “11” were superseded. The current prototype’s one-button 600 ms controls are a different, implemented interface.

Priority intent is SOS over arrival over focus over group, with low battery presented as a brief periodic yellow flash around every 30–60 seconds instead of hijacking navigation indefinitely. Charging uses green. SOS stays **directional** in the originating member’s color, blinking faster than any navigation pattern, with a distinct haptic alert proposed. It is not a whole-ring red alarm that erases the direction. Arrival is under 10 m, about eight seconds of flashing, then quiet/steady behavior and optional tap cancellation; the earlier ~30-second proposal is superseded. Exact SOS cadence, haptic waveform, accessible palette, acknowledgement/latching/clearing, and crowded-sector animation are unresolved. There is no haptic motor on the dev boards. [C15-M049; CLAUDE]

The planned bonding design uses a per-group **128-bit AES key**, member ID/color, complete roster, leader identity, and optional assisted-GNSS ephemeris provisioning. The assistant drafted a companion-app primary path over BLE LE Secure Connections, with an app-less direct-device fallback: leader holds to advertise/create group, nearby joining devices scan, then receive assigned identity/key information. The fallback was described with short physical distance/RSSI checks and ECDH. These are design proposals, not implemented security properties. Screenless pairing still needs an authentication/MITM design; proximity alone is not cryptographic authentication. AES mode, authentication tag, nonce uniqueness, replay protection, key storage, rekeying, and group membership changes are not specified sufficiently for implementation. “Packet decrypts” alone is not an authenticity test. [C15-M051; firmware TODOs]

A-GNSS is an optional pre-event accelerator for time to first fix, not a replacement for satellites or an internet requirement for every fix. Direct bonding without internet remains a proposed fallback. The initial statements that setup must occur before service disappears are UX cautions around convenience and ephemeris, not proof that a GNSS receiver cannot cold-start offline. The application would help make setup status clear. **There is no implemented companion app, BLE bonding, AES transport, or A-GNSS provisioning in the inspected prototype.** [design drafts versus code]

### Power model and motion-aware strategy

The restored [Kandi Power Budgeting.xlsx](<reference/Kandi Power Budgeting.xlsx>) resolves the previously missing worksheet, but exposes an incomplete total. PDF p. 16 contains matching images of its populated table and runtime summary. The original **17.5 mA / 64.75 mW / 22.85 hours** headline is a historical model result, not a validated total or measurement. The **400 mAh / 3.7 V / 1,480 mWh** battery and **13–16 hours** practical estimate remain paper assumptions for nRF52840/BNO055/eight-LED hardware. The PDF itself gives **30–40%** potential runtime improvement on p. 16 and **30–50%** on pp. 28–29; preserve that inconsistency. No adaptive scenario or measured before/after result in the workbook resolves it.

**Restored worksheet values (`Sheet1`, columns C/D/E):**

| Rows / component state | Current at labeled 3V3 (mA) | Duty | Stored average (mA) |
|---|---:|---:|---:|
| 2, nRF52840 sleep | 0.00235 | 90% | 0.002115 |
| 3, nRF52840 active at 64 MHz | 6.3 | 10% | 0.63 |
| 4, onboard BLE transmit | 16.4 | 0% (bonding only) | 0 |
| 6, SX1262 sleep | 0.0012 | 86% | **Blank** |
| 7, SX1262 TX at +14 dBm | 25.5 | 4%, described as 110 ms × 3–4 transmissions | 1.02 |
| 8, SX1262 RX | 5.3 | 10%, proposed RX duty-cycle mode | 0.53 |
| 10, MAX-M10S acquisition | 11.5 | 1%, described as boot only | 0.115 |
| 11, MAX-M10S tracking | 9.5 | 99% | 9.405 |
| 12, MAX-M10S backup | 0.028 | 0% (not v1) | 0 |
| 14, BNO055 normal | 12 | 5%, wrist raised | 0.6 |
| 15, BNO055 low power | 0.4 | 95% | 0.38 |
| 16, BNO055 suspend | 0.04 | 0% | 0 |
| 18, eight-LED ring, two lit at 40% brightness | 20 | 5% | 1 |
| 19, ring dark but powered | 4 | 95% | 3.8 |
| 20, LED state labeled “Off” | 0.5 | 0% | 0 |
| 22, miscellaneous overhead | 1.5 | 100% | 1.5 |

**Verified arithmetic and model limitations, without modifying the workbook:**

- `E24` is the only formula: `=SUM(E2:E20)`, giving **17.482115 mA**, consistent with its stored/cached value and the screenshot. Its range excludes `E22`'s **1.5 mA overhead**. All populated row averages are numeric constants, not formulas linked to C/D; several duty cells contain explanatory text rather than numeric percentages.
- Including the already-entered overhead gives **18.982115 mA**. The omitted sleep contribution implied by `C6 × D6` is **0.001032 mA**; including both omissions gives **18.983147 mA**. These are review calculations under the sheet's assumptions, **not measured board current** or a replacement validated power budget.
- `B25:B30` are typed text, including rounded 17.5 mA, 64.75 mW and 22.85 hours. They do not update when the assumptions or total change. PDF p. 16 embeds the same headline values, so its apparent confirmation is the same model repeated, not independent evidence.
- Column C is explicitly labeled current at **3V3**, but the runtime text multiplies a current by **3.7 V** battery voltage. Load-rail power, battery-side current, regulator efficiency and transient energy require a consistent model before revising runtime. Simply replacing 17.5 with 18.983147 in the old calculation would not validate battery life.
- `F8` describes RX current at **125 kHz**, while the selected firmware preset uses **250 kHz**. `D7` assumes ~110 ms TX airtime, superseded by the ~200–250 ms hardware notes. RX duty cycling is assumed here but is not implemented. The “boot only = 1%” acquisition allowance lacks a defined session duration.
- The GNSS current notes use **GPS+GAL+BDS B1I**; backup assumes `V_BCKP=3.3 V` with `V_IO=VCC=0 V`. These are source operating conditions to retain, not proof of current board configuration. They also differ from the PDF p. 14 prose's ~25/12 mA acquisition/tracking figures and its general four-constellation description.
- The “Off” LED row still specifies 0.5 mA at zero duty. It does not demonstrate a zero-current MOSFET-gated state. The sheet's baseline uses continuous GNSS tracking and powered-dark LEDs despite the adjacent PDF discussing future gating/duty cycling.

The earlier C15-M031 suggested table remains a distinct revision: nRF sleep/active duty 95%/5% and active LED current 19 mA, yielding about 18.6 mA including its listed overhead and sleep contribution. The restored sheet instead uses 90%/10% and 20 mA, then omits overhead from the total. The gap between the conversational table and the 17.5 mA headline is therefore now explainable without inventing a new hardware measurement. Preserve both versions. [C15-M031–M035; restored workbook `Sheet1!A1:G30`; PDF p. 16]

GNSS was the dominant ~9.5 mA term, around 54% of the total; “off” WS2812 chips still consumed an estimated 4 mA. Therefore the intended optimizations were GNSS duty cycling and a MOSFET to truly cut ring power. The model’s RX preamble-detect assumption is especially provisional: early prose describes a preamble longer than its own total-airtime estimate, and current firmware does not implement that receive schedule. Correct actual preamble timing and missed-packet behavior before relying on the saving. [C15-M031; radio notes]

Owen’s key correction: **dancing in place must not count as travel**. His initial idea was no significant movement, perhaps 5 m for five minutes. The subsequent working design used GPS displacement, approximately **under 10 m over five minutes**, with a rolling positional history/max-displacement notion. Active movement or a button should restore active behavior. IMU activity can help detect a glance but should not override location-based stationarity merely because the wearer dances. Stationary GNSS was proposed as roughly **3 seconds every 30 seconds**, with **60-second heartbeats** instead of 10 ±2-second active beacons. GNSS noise, dwell thresholds, fix reacquisition, and what “sustained” displacement means still require implementation and testing. [C15-M034–M037; PDF p. 15]

The restored adaptive section also specifies an IMU **significant-motion early-wake hint** to resume normal GNSS cadence before a scheduled wake, plus wrist-raise detection by forearm rotation pattern. This clarifies the intended hybrid behavior: GPS displacement determines stationarity; raw dancing motion must not permanently defeat it. No detection algorithm or validated thresholds for those IMU hints are supplied.

**Newly identified integration conflict:** a 60-second stationary heartbeat would routinely cross the current 30-second `STALE` threshold. The receiver needs awareness of stationary cadence or revised freshness semantics; otherwise successful power saving would look like stale navigation. This is an inference from combining accepted future timing with current code, not a decision already made in Claude.

The actual ESP32-S3, OLED, 16-pixel ring, and 18650 dev board have a different baseline. Validate relative savings first and separately assess whether absolute runtime justifies an MCU change. Do not infer a future battery capacity or measured current from the prototype’s battery size.

### Enclosure: latest mechanical truth and tentative CAD

The active CAD work is a **prototype forearm housing** for a two-board stack, not a 40 mm product watch. The nominal stack envelope is roughly **114.6 × 33 × 28 mm**. Owen has used Fusion 360 before but was at the starting sketch/rectangle stage in this conversation, had a ruler rather than calipers, and was still sourcing a printer. No completed CAD or print is evidenced. A Velcro strap through printed attachment features was chosen. Charging by USB means a separate battery hatch is not required; removing the faceplate for a battery change is acceptable. [C55]

Owen’s latest corrections, which override earlier assistant layout assumptions:

- Keep the existing adhesive GNSS patch in its working location. Do not reroute it into the LED ring’s center or redesign its cable path merely to make the CAD neat.
- The ~28 mm stack already includes its existing headers and the unused middle USB connector. Do not add that connector height a second time.
- There are two USB connectors; one is unused. An initial denial was corrected. Preserve access to the used port.
- The LoRa whip is already hinged: it can point +X, roughly +X/+Z at 45°, or +Z at 90°. Extra angle adapters are not required by this geometry.
- Intended left-arm wearing: **+X toward the hand; +Z away from the wrist; +Y outward/button side; used USB at −X toward the shoulder.** LED0 points +X.
- The LED ring sits above the rear half, preserving OLED visibility. The latest referenced ring size is **45 mm OD / 32 mm ID**, associated with DIYmall WS2812B listing ASIN `B0B2D5QXG5`; this is not a caliper-verified size. Earlier 44.5/31.7 mm numbers are superseded nominal values.
- LED jumper/connector height above the board remains unmeasured. The existing 28 mm figure does not settle that future assembly clearance.

[C55-M007, M009, M011]

The last assistant response proposes a parameterized three-part Fusion design: `Tray`, `FacePlate`, `Diffuser`, using a `BoardDummy` plus landmarks. It is a continuation scaffold, **not an approved or fabricated design**. Preserve its proposed values without silently promoting them to requirements:

| CAD parameter/proposal | Value or intent | Status |
|---|---|---|
| `board_L`, `board_W`, `board_H` | 114.6, 33, 28 mm | Nominal envelope; verify assembled hardware |
| `fit_gap`, `top_gap` | 0.4 mm per side, 0.5 mm | Initial print-fit estimates |
| `ring_OD`, `ring_ID` | 45, 32 mm | Latest nominal ring size |
| ring PCB / LED / clearance | 1.6 / 1.8 / 0.5 mm | Proposed stack assumptions |
| `ring_ctr_X` | 85 mm **from the hand-end datum** | Datum must be reconciled with +X-toward-hand convention |
| `led_gap`, diffuser | 5 mm mixing space; ~1 mm white PLA | Optical experiment, not verified readability |
| wall / floor / faceplate | 2.5 / 2 / 2.5 mm | Initial structural proposal |
| head width / body width | 51 / 38.8 mm | Proposed outer geometry |
| strap | 25 mm wide, 3 mm thick; 26 × 4 mm slot; ~25 mm inset | Fit proposal |
| `hdr_relief_d` | 10 mm | Guessed connector relief; unresolved geometry |
| total stack estimate | ~42.4 mm | Does not settle the unmeasured connector clearance |

[C55-M012]

Mechanical constraints with stronger authority are: antenna outward/away from the body; GNSS skyward in the reading pose; keep BOOT, RESET, USB, OLED and ring accessible; no nearby ferrous fasteners; preserve ring orientation; diffuse LEDs into sectors; recalibrate each assembled unit. The original enclosure brief is recoverable in full but predates the detailed CAD corrections. [CLAUDE enclosure constraints; C15-M271; C55-M000]

The final assistant suggested a roofless SMA notch for upward insertion, clearance for the whip’s swept hinge envelope, button/USB cutouts, nonferrous/nylon fastening, strap slots through a solid pad, eight optical baffles, and a detachable three-wire ring harness with service slack. Foam support, a thin nonmetallic GNSS roof, and flat underside plus EVA were proposals. Protect GPS patch/cable strain and avoid conductive ring PCB overlap over the antenna; the user’s “keep working GPS placement” instruction takes precedence over these drafting conveniences.

**Unresolved geometry:** a 10 mm-deep header pocket cannot exist wholly in a 2.5 mm faceplate without a raised feature or increased height. The suggested pocket versus lower-profile soldered wiring has not been chosen. A `Combine > Cut` of a nominal board dummy also does not automatically create the stated per-side fit clearance; model the clearance explicitly. These are migration review inferences, not completed CAD fixes. Printer/process, exact ring dimensions, assembled connector envelope, fastener locations, and antenna clearance still need measurement. The prototype is not claimed waterproof.

### Measured results and what they establish

**July 30 two-device residential walk:** Owen reached 635 m through houses and stopped walking; do not call it a rigorously determined absolute maximum. Reported long-range update gaps were 15–25 seconds beyond roughly 550 m despite a nominal 10-second beacon interval. The README preserved these sample points:

| Separation | RSSI | SNR | Reported segment delivery |
|---|---:|---:|---:|
| 139 m | −78 dBm | +5.8 dB | Not given |
| 288 m | −113 dBm | −8.2 dB | 73% |
| 390 m | −107 dBm | −1.2 dB | 88% |
| 635 m | −119 dBm | −13.5 dB | 63% |

The restored PDF p. 17 clarifies that **segment delivery uses changes in received and transmitted counters between readings**, rather than a cumulative received/transmitted ratio, because the receiver booted after the transmitter. It reports both boards at 12 satellites and HDOP 0.6–1.0 throughout. These are source-reported readings and methodology, not independently recomputed from a supplied raw CSV; the underlying counter series remains missing. Owen also reported far-end SNR around −18 and intermittent −15. The observed 635 m lies within the original 500–800 m estimate, although a residential walk does not validate the dense-crowd conditions assumed in that estimate. [C15-M230–M236; README]

At about 200 m, holding the board clear gave SNR +5.0 dB and pressing it to the chest gave −6.2 dB: **11.2 dB difference**. This is evidence that body/antenna placement materially affects the link in that setup. “Body absorption” is the project’s shorthand; orientation, environment, and noise assumptions limit an isolated material-loss interpretation. The conversations infer a path-loss exponent **n ≈3.7** from the 200 m and 635 m SNR values, then predict roughly **410 m body-worn** using a nominal −17.5 dB SF11 threshold. That 410 m result is a model projection, not a measured body-worn range. It is derived from remaining margin at 200 m, not simply subtracting a fixed factor from an established 635 m maximum. Do not extrapolate a fixed 11 dB loss per person or claim this is a direct comparison against a tested 2.4 GHz device. [C15-M232–M235]

The project interpreted SNR as a practical limiting indicator. No independent absolute noise-floor survey is supplied; statements that the measured noise floor moved or that the 500–800 m prediction failed are unsupported. A nominal demodulation threshold is not a hard packet cutoff; later records mention packets decoded below it. [C52 application drafts versus engineering records]

**LED/magnetometer interference:** no detectable change was reported with the board stationary and LEDs dark versus moderately lit. This supports the tested dev-board arrangement; it does not permanently close interference risk in a tighter final enclosure, with changed currents or component placement. Calibration and antenna placement remain assembly-dependent. [C15-M235; CLAUDE]

**August 18 three-device drive test:** board 3 at home, board 2 as a midpoint roughly 300 m away, board 1 mobile. The reported display on board 1 included member 2 at 264 m, zero relays, about −117 dBm / −11.5 dB, direct counter 35 and relay counter 0; member 3 at 628 m, one relay, about −118 dBm / −13.2 dB, direct 0 and relay 15. These establish a packet with an originator 628 m away being delivered through a relay. The RSSI/SNR for member 3 are those of the bridge’s final transmission, not a direct 628 m RF link. [C15-M258]

**The forced-topology caveat is mandatory.** The endpoint blocklists remained active. Board 3 logged dropped frames from board 1, so at least some direct frames were decoded and then suppressed. A zero processed direct counter cannot prove there was no direct RF path. The supplied README and CLAUDE retain “almost certainly dead anyway” wording; the primary discussion corrects that overreach. Supported claim: **three-node relaying demonstrated across 628 m endpoint separation under a forced topology**. Unsupported claim: proven unforced extension beyond direct range. [C15-M259–M260]

A separate walk-in verified the arrival flash and cancellation/re-arm behavior, and Owen reported demo footage captured. Field observations establish a functioning prototype; they do not establish production enclosure, eight-node crowd performance, encrypted bonding, all-day runtime, or a published demo.

### Tests, validation boundaries, and source issues worth carrying forward

The reference Python scripts cover distance/bearing, LED mapping, synthetic compass recovery, mesh receive decisions, network topology, and integrated display behavior. They are mostly standalone print/expected-value scripts, not a modern assertion-heavy test suite. Firmware contains **19 core navigation/LED boot checks and 23 mesh checks = 42 total**. The 26 compass round-trip cases are a separate suite/history, not 26 more boot checks to add indiscriminately. A later “23/23” mesh result includes four originator-preservation checks. Golden values test port consistency; they do not independently certify the math or radio behavior. [source selftests; C15-M114, M256; README]

**New source-inspection inferences, not historical fixes:**

- The current global `MESH_BLOCKED_SENDERS {1}` is also consulted by pure mesh simulations. Tests that originate sender 1 can therefore be affected by this build setting. Treat the historical “42 passing” result as configuration-dependent; verify with an appropriate test configuration before claiming a fresh all-pass run.
- `mesh.cpp` describes 32 entries at a 10-second cadence as roughly five minutes of group traffic memory. That is roughly one sender’s traffic; eight senders generate 32 new beacons in about 40 seconds before relay/other considerations, and entries expire at 30 seconds anyway. The cache may still be adequate, but the comment is not a scale analysis.
- `main.cpp` prints the fallback’s old `tgtId` before assigning it from `tgt.id`, so that diagnostic can report 0 rather than the previous target. This is a log-quality issue, not evidence that target fallback fails.
- The wire packet carries no source timestamp. Receiver `lastUpdateMs` measures receipt of valid position information, not independently authenticated GNSS sample age. Replayed or delayed packets need a defined policy when security/freshness work is undertaken.
- Exact-half-sector rounding, hardcoded declination, timer wraparound behavior around absolute arrival/toast deadlines, stale heading retention, sequence resets, and cache exhaustion are reasonable focused future checks, not newly observed hardware failures.

No code was altered to resolve these issues during this context migration.

### Additional design detail recovered on 2026-09-07

These additions are historical design intent or source clarifications. They do not introduce new implementation work ahead of the agreed enclosure task.

- **Preset tradeoff, PDF pp. 5 and 12:** the embedded comparison gives Long Fast SF11/CR4/5/250 kHz, 1.07 kbps, 153 dB; rejected Long Moderate SF11/CR4/8/125 kHz, 0.34 kbps, 156 dB; and tentative Medium Slow SF10/CR4/5/250 kHz, 1.95 kbps, 150.5 dB. Medium Slow was considered for capacity at an estimated 20% range cost. These are historical table/model values, not verified link budgets at the current +14 dBm setting or authorization to retune. Later Long Fast remains selected.
- **GNSS and UWB intent, PDF p. 14:** names AssistNow **Offline**, with a proposed first-fix target <10 s after preload, 3–5 m open-sky and 5–15 m crowded-sky positioning estimates. UWB deferral cites a Qorvo DW3000, an additional antenna, GNSS/UWB handoff complexity and a historical ~$30 cost estimate. A 10–20 m “look around” cue was proposed; this does not replace the implemented <10 m arrival threshold. None of those accuracy/time/cost estimates is a new measurement or current quote.
- **Multi-phone bonding, PDF pp. 22–23:** each member's device is paired to **that member's phone**, and the app provisions key, ID/color, complete roster, leader ID and assistance data. The mechanism for distributing a group created on the leader's phone to the other phones is not specified. Success flashes the assigned member color; active bonding chases/spins. A-GNSS validity is described as hours to a few days, without an exact product/data-lifecycle implementation. The complete provisioning payload is design intent, not a packet schema or implemented BLE API.
- **Security scope, PDF pp. 23 and 29:** the author explicitly accepts the screenless fallback's lack of full MITM resistance as a v1 tradeoff and leaves replay/spoofing/jamming analysis beyond duplicate suppression out of v1 scope. Preserve that historical decision and limitation; do not describe the planned transport as secure against those threats or silently add security implementation to the current task. The app path's assertion of an authenticated BLE link is still an intended property, not demonstrated pairing behavior. “Leader ID for reserved designator treatment” on p. 23 is unresolved wording; it does not reinstate yellow as a member/leader color.
- **Compact-product packaging, PDF pp. 16 and 24–26:** a 300–400 mAh LiPo roughly 25×25×4 mm, ~40 mm circular face and 12–15 mm integrated-prototype thickness were paper starting points. Stack: perimeter eight-LED ring/recessed center buttons, PCB, then battery; GNSS ceramic patch under a nonmetallic face, BLE chip/trace antenna, and a flexible LoRa antenna in the strap as a candidate. These do not fit or override the current 114.6×33×28 mm T-Beam stack and measured-as-needed connector clearance. The later instruction to keep the working GPS patch fixed remains authoritative.
- **Aesthetic construction, PDF p. 25:** actual pony beads on elastic cord surrounding/accenting a silicone strap, with the decorative layer intentionally handmade and variable. This is a useful product-design detail, not a reversal of the chosen Velcro attachment for the current enclosure.
- **Explicit future validation, PDF pp. 28–29:** possible thermal discomfort at the wrist, reading up to seven other-member colors under strobes, GPS-jitter-driven false stationarity transitions and false/failed SOS activation are acknowledged risks. The document allows revisiting UWB earlier only if user testing makes it necessary; no such result or decision is provided.

**Internal conflicts retained rather than merged:** p. 3 research notes describe Crowd Compass's map on the device, while p. 10 says an app is required to read it; neither becomes a verified competitor claim here. P. 13 says relay delay is “inversely proportional” to received strength while also intending weak-first relay: the implemented positive SNR-to-delay mapping remains authoritative. It also says “<8” users where the rest of the paper targets eight; the actual eight-slot roster remains unchanged. P. 25 still suggests LED/mag interleaving as a potential need, while pp. 18 and 28 record no detected interference on the tested hardware; the latter resolves that specific test, not all future layouts. P. 17 both says packets still arrived where testing ended and calls 635 m a practical maximum; keep the observed-reach wording and its inference caveat. Pp. 10/13's “without interference” encryption claim is limited by p. 27's explicit shared-airtime risk. The PDF's 19 tests, pending calibration/mesh and nRF-first roadmap (pp. 29–30) are superseded by later 42-test definitions, completed prototype work and CAD-first/S3-versus-nRF-open decisions.

The restored sources add no 628 m relay logs, board-specific calibration values, final CAD files, implemented BLE/AES, measured power saving, or proof of runtime. Their restoration closes the missing design-snapshot and budget-table gaps, not those separate evidence gaps. See `KANDI_SOURCE_MAP.md` for hashes, page/cell locators and uncertain historical revision mapping.

### Conflicts and superseded statements

| Topic | Earlier or conflicting statement | Reconstructed authoritative state |
|---|---|---|
| GPS dependency | Initial prompt says no GPS dependency | GNSS required for location; no cellular/Wi-Fi/internet runtime dependency |
| Project identity | “Drop the kandi aesthetic” to finish a résumé project | User rejected that simplification; aesthetic remains part of the product |
| Navigation target | One designated leader; later “most recently heard” | Original leader model broadened; current target is selectable/sticky with lost-target fallback |
| MCU/Phase 3 | nRF52840 finalized; power/miniaturization first | August 18 CAD first, then measured power; S3 versus nRF open |
| Prototype status | Older memories: calibration/mesh/integration incomplete | August handoffs and supplied source document completion of those prototype milestones |
| Security | Memory and polished descriptions imply AES/bonding built or members “paired” | Current groupId is plaintext; BLE/AES/A-GNSS remain future work |
| Calibration | C52-M061 describes Python/NumPy least-squares offsets and scales | Implemented C++ min/max hard-iron offsets only; no scale or soft-iron fit |
| Range narrative | Résumé drafts say prediction failed or “direct 628 m” | 635 m residential result falls within estimate; 628 m is endpoint separation in forced relay test |
| Relay proof | Documents imply endpoint direct link was almost certainly dead | Not established; dropped-blocked logs show some direct decoding |
| Airtime/capacity | ~110 ms, ~33% utilization treated as comfortable | Hardware notes ~200–250 ms; actual capacity and crowded scale unvalidated |
| Hardware files | `compass.py`, `LEDLogic.py` documentary aliases | Current files `tilt_compensation.py`, `Led_Logic.py` |
| LED supply | Early VBUS/5 V advice | Latest actual wiring documented as 3V3; retain brightness/current settings separately |
| Arrival/SOS | ~30 s arrival, repeated-press SOS/additional button | ~8 s arrival; product SOS hold/directional proposal; SOS not implemented |
| CAD | Relocate GPS, add adapter, assume a single USB or extra stack height | User’s final geometry corrections in C55-M011 take priority |
| Professional work | September Teensy/KiCad/current-sensor PCB inside a Kandi-titled chat | Separate NYU robotics work, not a completed Kandi PCB |
| Authorship | Polished “wrote every driver” implication | User acknowledges substantial AI-assisted coding; describe built/integrated/debugged behavior and actual understanding honestly |

The complete historical drafts remain in the source material so future work can inspect the rationale without reviving their unsupported claims.
