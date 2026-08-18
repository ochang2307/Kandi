# Kandi — Offline Mesh-Networked Festival Wearable

A wrist-worn device that guides you to your friends at large festivals when
cell networks fail — no cellular, Wi-Fi, or internet dependency. Devices in a
group locate each other over a 915MHz LoRa mesh and display direction via a
ring of LEDs: glance-readable, screenless, and styled after kandi bracelets
rather than looking like a gadget.

**[Demo](#demo)** · **[Status](#status)** · **[What's next](#whats-next)** ·
[Hardware](#hardware) · [Measured results](#measured-results) ·
[Engineering notes](#engineering-notes)

## The problem

At large festivals (EDC-scale, 100k+ attendees), cellular networks saturate
within the first hour. Groups get separated with no way to reunite. Existing
products compromise: Totem Compass uses 2.4GHz (heavily absorbed by crowds)
in a bulky pendant; Crowd Compass got the radio right (sub-GHz LoRa) but
requires a phone app. Apple's offline finding only works at ~10-30m.

**Kandi's thesis: sub-GHz LoRa range + screenless wrist UX + cultural fit.**

## Demo

Three devices navigating to each other in the field — LED ring direction
finding, live distance, and a position relayed through a midpoint node.
!WILL BE UPLOADED SOON!
<!-- demo video link goes here -->

## Status

- ✅ **Phase 1 — Design & architecture: complete.** Full engineering design
  doc covering radio selection (SX1262, LoRa "Long Fast", 915MHz), mesh
  protocol (managed flooding), GNSS (u-blox MAX-M10S + A-GNSS), power budget
  (~13-16hr projected on 400mAh), LED UX, bonding flow, and risk analysis.
  → [Design doc](https://docs.google.com/document/d/1ndvAmwJ7iS_GCs7uOu1srA6J-rN4ECKuPedQrF4hdPo/edit?tab=t.4cz32g8cwkpv)
- ✅ **Phase 2 core logic: complete.** The full navigation and networking
  stack, implemented and unit-tested in platform-independent Python and
  validated in simulation before any hardware existed.
- ✅ **Phase 2 hardware bring-up: complete.** All peripherals working on
  LilyGo T-Beam Supreme dev boards — power management, display, GNSS, IMU,
  magnetometer, addressable LED ring, and a LoRa link between two boards.
- ✅ **Navigation pipeline on hardware.** Live GPS fix → distance and bearing
  to a target → LED ring indication, with the ported navigation stack passing
  19 boot-time self-tests against its Python golden values.
- ✅ **First field validation.** 635 m link range through residential
  obstruction against a 500–800 m link-budget prediction; body absorption
  measured at 11.2 dB.
- ✅ **Mesh protocol on hardware.** Managed flooding ported to firmware and
  verified on a three-board desk topology with simulated out-of-range links:
  positions crossing a gap via relay, duplicate suppression terminating the
  flood, hop limits enforced, and SNR-weighted relay delay (edge nodes relay
  first, redundant relays cancel on overhearing a duplicate).
- ✅ **Magnetometer calibration.** Hard-iron capture with live per-axis
  coverage feedback, offsets persisted to NVS per board. Corrected field
  magnitude holds 46–56 µT through full rotation on all three boards — against
  a raw hard-iron offset ~5× Earth's field.
- ✅ **"Find your friend" pipeline working end-to-end.** Two boards point
  their LED rings at each other live: mesh position packets → per-member
  roster → great-circle bearing → tilt-compensated, declination-corrected
  compass heading → ring sector. Heading holds within ~2–3° through tilt.
  Ring states per the design doc: distance-bucketed blink while navigating,
  a distinct "remembered, not live" indication when a member's position goes
  stale (>30 s), lost (>120 s), and arrival within GPS resolution.
- ✅ **Arrival lifecycle verified.** Walk-in test: full-ring flash fires on
  crossing the 10 m threshold, decays to a steady indication after 8 s, and
  re-arms behind a 15 m hysteresis gap so GPS noise at the threshold can't
  re-fire it — the arrival threshold sits at the GNSS relative-error floor
  by design, which is the measured case for deferring final approach to UWB
  in v2.
- ✅ **Three-board mesh relay confirmed in the field.** A drive test with one
  node held back as a bridge relayed a member's position across a **628 m** gap
  to a device receiving nothing from it directly — heard zero times direct,
  fifteen times relayed, at −118 dBm / −13.2 dB SNR (≈4 dB above the SF11
  demodulation floor). Navigation, distance-bucketed blink, and the arrival
  flash all held at range, and the logs captured a live lost-then-reacquire as
  a node came back into range.
- ✅ **Phase 2 complete — working three-device prototype.** Every layer of the
  design doc runs on hardware end-to-end: offline GPS-over-LoRa-mesh position
  sharing, multi-hop relay at real range, and glance-readable LED direction
  finding, demonstrated across three devices in the field and captured on
  video.
- 🔜 **Phase 3, in progress — wearable enclosure, then measured power, then
  PCB work.** Detail in the next section.

## What's next

The design doc's phase 3 called for migrating to an nRF52840 immediately.
That is deferred: a custom RF board is the most expensive and highest-risk
step available, and there is cheaper work that produces more evidence first.
Revised order:

**1. Enclosure CAD — a wearable prototype (next two weeks).**
A 3D-printed forearm housing for the current dev boards. The T-Beam Supreme
is ~115 × 33 × 28 mm, so the honest form factor at this stage is Pip-Boy
style rather than a bracelet — this is a prototype housing, not an attempt at
the product form factor. Several constraints are already fixed by
measurement: the LoRa antenna sits on the outward face (the 11.2 dB body
absorption figure makes that a 2× range decision), the GPS antenna faces up
in the wrist-raised reading pose, fasteners near the magnetometer must be
non-ferrous, and every board gets recalibrated after assembly since the
enclosure changes the magnetic environment. Deliverable: units that can
actually be worn, and a demo reshot with them on.

**2. Power validation — measured, not modeled.**
The design doc specifies adaptive duty cycling (GPS-displacement
stationarity detection, GNSS duty-cycling while stationary, LED power
gating) and projects 13–16 hr with a 30–40% improvement. None of that is
implemented or measured yet. The plan is to build it and then measure
before/after current draw, converting the modeled figure into a real one the
same way the range estimate was converted into a measured 635 m. Stated
plainly: an ESP32-S3 with an OLED will not approach the doc's 17.5 mA budget
number, which was computed for different silicon — the goal is validating
the strategy and quantifying the delta it buys, not defending a number.

**3. PCB work, via a stepping stone.**
Rather than jumping straight to a custom RF board, the intermediate step is a
simple 2-layer PCB LED ring sized to the enclosure face. No RF, low risk, and
it covers the entire fab workflow — schematic, layout, ordering, assembly —
before that workflow has to carry a 915 MHz design. A full custom board
(MCU + SX1262 + MAX-M10S) remains the longer-term target.

**Open question: which MCU.** The design doc chose the nRF52840, but that
predates the ESP32-S3 firmware that now exists and works. Staying on the S3
preserves every driver and all the toolchain knowledge; moving to the
nRF52840 buys power efficiency at the cost of rewriting each
platform-specific driver. That trade should be decided with the power
measurements from step 2 in hand, not before.

### Deferred, deliberately

Specified and consciously not built yet: group view / focus mode (a minimal
target-cycling stand-in exists), the full button gesture map, BLE bonding
(`group_id` is currently a compile-time constant), SOS, and haptics — the
last blocked on custom hardware, since the dev board has no motor.

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
| `radio.cpp` | SX1262 LoRa via RadioLib, non-blocking (interrupt-driven), mesh + range-test modes |
| `mesh.cpp` | Ported managed-flooding mesh: deterministic wire format, fixed-size seen cache with expiry, SNR-weighted scheduled relay queue |
| `calibration.cpp` | Magnetometer hard-iron capture with live coverage UI, offsets persisted in NVS |
| `roster.cpp` | Last-known position per mesh member; feeds the live navigation target |

Every ported module ships with the **original Python test vectors as
boot-time self-tests** — 42 cases across two suites: distance, bearing,
relative bearing, LED mapping, and wraparound asserted against the verified
Python values, plus the mesh scenarios (relay across a gap, loop termination
via dedup, hop-limit expiry, seen-cache expiry/eviction, SNR-weighted delay,
and originator-identity-survives-relay) re-run against in-memory device
instances on every power-up. A translation error surfaces at boot rather
than in a field test.

## Measured results

First field test on dev-board hardware, 915MHz SF11 (Long Fast), residential
neighborhood with houses in the signal path:

| Distance | RSSI | SNR | Segment delivery |
|---|---|---|---|
| 139 m | −78 dBm | +5.8 dB | — |
| 288 m | −113 dBm | −8.2 dB | 73% |
| 390 m | −107 dBm | −1.2 dB | 88% |
| 635 m | −119 dBm | −13.5 dB | 63% |

- **635 m** confirmed range, inside the design doc's 500–800 m prediction.
- **SNR was the binding constraint, not signal strength.** SF11 demodulates
  to about −17.5 dB; far-end readings ranged −13.5 to −18, so this is near the
  practical limit for the environment rather than where the walk happened to end.
- **Path loss exponent n ≈ 3.7**, derived from the 200 m and 635 m readings.
- **11.2 dB body absorption** at a fixed 200 m separation (held clear vs.
  pressed to chest), projecting ~410 m body-worn. This makes antenna
  orientation a first-order form-factor constraint.
- **Position freshness degrades before the link does.** Past ~550 m, gaps
  between received updates reached 15–25 s against a 10 s transmit cadence —
  validating the stale-bearing LED state specified months earlier.

A later three-board drive test added the relay leg. One node was parked as a
bridge ~300 m out while a second was driven to 628 m from a stationary,
serial-logged third:

| Link | Distance | RSSI | SNR | Hops |
|---|---|---|---|---|
| Endpoint → bridge (direct) | 264 m | −117 dBm | −11.5 dB | 0 |
| Far endpoint's position, delivered via bridge | 628 m | −118 dBm | −13.2 dB | 1 |

- **A position crossed a 628 m gap through a midpoint** to a device that
  received it zero times directly and fifteen times relayed — managed flooding
  extending reach past the direct link, at ~4 dB of SNR margin over the SF11
  floor. Navigation, distance-bucketed blink, and the arrival flash all held at
  range; the logs also caught a live lost-then-reacquire as the bridge came
  back into range.
- **Caveat: the topology was forced, not purely distance-driven.** The
  compile-time 1↔3 blocklist used for bench testing was still active during
  this run, so the endpoints were prevented from hearing each other rather
  than being proven out of range. The earlier link budget says they very
  likely were out of range anyway — 635 m measured maximum, ~410 m body-worn,
  and the direct 1↔3 link was already down to −21 dB SNR at a shorter
  distance — but that is inference, not measurement. An unforced rerun with
  empty blocklists at the same distance is planned; until then the supported
  claim is "relay across a 628 m gap via a midpoint," not "the mesh
  self-organized around a dead link."

## Engineering notes

A few problems worth documenting:

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

**Simulating "out of range" taught a protocol lesson.** To force multi-hop on
a desk where all three boards hear each other, each board can be built with a
list of node IDs it pretends not to hear. The first implementation filtered on
the packet's *originator* — which also silently discarded relayed copies of
that originator's packets, the exact traffic the test existed to observe. The
fix distinguishes the frame's transmitter from the packet's originator on the
wire, because radio range is a property of who is on the air, not whose data
is inside. The desk test then showed positions crossing the simulated gap
with one hop consumed.

**Two right-handed chips, one left-handed convention.** Tilt-compensated
headings were correct with the board flat and wildly wrong under tilt — the
signature of the accelerometer and magnetometer disagreeing about the shared
sensor frame, which is invisible at zero tilt (the vertical mag axis and the
horizontal accel axes drop out of the math). The compass algorithm, verified
in simulation, embodies a left-handed frame; both physical chips are
right-handed, so each required exactly one reflection to match — and the two
chips were also mounted with different in-plane orientations. Diagnosed with
a four-pose capture in which the gravity-referenced poses were decisive
(pointing a board at "north" is sloppy; gravity is not): whichever axis
catches ±1 g, or the ~40 µT downward field component, names itself. The fix
is two compile-time remaps at the driver boundary; the verified math is
untouched. A key discriminator along the way: corrected field *magnitude*
stays constant through tilt when the problem is frames (reflections preserve
magnitude) and swings when the problem is calibration.

**Retiring a risk with measurement.** The design doc flagged possible
magnetometer interference from LED ring switching currents. Measured with the
ring dark versus lit: no detectable shift. Risk closed, and the firmware
avoids an unnecessary LED-off sampling window.

**Measuring instead of extrapolating.** Rather than treating the design doc's
500–800 m estimate as a claim to defend, the field test derived a path loss
exponent (n ≈ 3.7) from two independent readings and used it to project the
body-worn case, and identified SNR rather than RSSI as the binding constraint
by comparing measurements against the SF11 demodulation floor. The prediction
held, but the reasoning is now anchored in measurement rather than the link
budget alone.

**Quantifying the core design argument.** Choosing sub-GHz LoRa over 2.4 GHz
rests on water absorbing 2.4 GHz more strongly than 915 MHz. Measuring 11.2 dB
of loss from a single body turned that cited principle into a number — and
surfaced an unanticipated constraint, since an 11 dB gap between the outward
and skin-facing sides of a wristband is a 2× range difference decided purely
by antenna placement.
