# CLAUDE.md — Kandi

## What this project is

Kandi is a wrist-worn festival wearable that guides group members to each
other without cell service, Wi-Fi, or internet. Devices share GPS positions
over a 915MHz LoRa mesh and display direction via an LED ring — screenless,
glance-readable, styled after kandi bracelets. Solo project by Owen (ECE
undergrad) — both a real product ambition and an internship portfolio piece.

Full engineering design doc lives outside this repo (link in README). It is
the source of truth for design decisions; consult it before proposing
architecture changes.

## Current phase

Phase 2: dev-board prototype. All core logic is written and unit-tested in
platform-independent Python (done pre-hardware). Hardware just arrived:
**3× LilyGo T-Beam Supreme** boards. Next work is bring-up and porting the
proven Python logic to the boards (Arduino/PlatformIO, C++).

Goal of this phase: a two-device "find your friend" field demo + video,
with a third board proving mesh relay.

## Repo structure

Core logic lives in `CoreLogic/`. All of it is **verified and working** —
do not rewrite or "improve" these modules without being asked; changes must
keep their tests passing.

- `navigation.py` — haversine `distance()` + initial `bearing()` between
  GPS coords. Verified against real-world coordinates.
- (compass file) — `pitch_roll()`, `tilt_compensate()`, `heading_from()`.
  Tilt-compensated compass: recovers device heading from raw accel + mag at
  any orientation. Verified by synthetic round-trip tests to <0.5° error.
  The tilt_compensate formulas were bug-hunted carefully — do not alter
  them.
- `LEDLogic.py` — `relative_bearing()` (target bearing − device heading)
  and `led_for_bearing()` (angle → LED index).
- `mesh.py` — packet dict structure (`make_packet`), duplicate suppression
  (`has_seen`/`mark_seen`, keyed on (sender_id, packet_id) tuples), and
  `handle_packet()`: the per-device receive state machine
  (group check → dedup → mark seen → hop-limit → relay).
- `network.py` — multi-device flooding simulation with configurable
  topology. Validates relay-across-non-adjacent-nodes, loop termination via
  dedup, hop-limit expiry.
- `device.py` — `device_tick()`: integration loop. Member positions in →
  full LED display state out. Plus `blink_rate_for()` distance buckets.
- `*test*.py` files — runnable test scripts (plain prints, eyeball-verified
  expected values in comments). Run them after any change to the module
  they cover. `python3 <testfile>` from inside `CoreLogic/`.

## Architecture principle (important)

Core logic is platform-independent and hardware-free. On hardware, only the
I/O edges change: real NMEA feeds navigation, real IMU registers feed the
compass, real SX1262 TX replaces the `transmit()` placeholder, real AES
decrypt replaces the group_id check. The algorithms themselves port as-is.
Preserve this separation when writing firmware — keep ported core logic in
its own files/functions, hardware drivers thin and separate.

## Locked-in design decisions (from the design doc — don't relitigate)

- Radio: SX1262, 915MHz, Meshtastic "Long Fast" preset (SF11, 250kHz,
  CR 4/5). ~110ms airtime per position packet.
- Mesh: managed flooding. Hop limit 3, dedup via (sender, packet_id) cache,
  SNR-weighted relay delay (delay not yet implemented in sim — TODO on
  hardware). Group separation by shared AES key (v1 sim fakes this with
  group_id match).
- Position updates every 10s ±2s jitter; 60s heartbeat when stationary
  (GPS-displacement-based stationarity, NOT IMU-based — dancing in place
  must not defeat power saving).
- UWB deferred to v2. GNSS-only + haptic "you're close" for final approach.
- Angles: ALL angles at function boundaries are degrees, 0–360, 0 = north,
  clockwise. Radians are internal-only. Any new angle-returning function
  follows this.
- LED indices 0–7 internally (0 = top/12-o'clock). Physical rings are
  16-LED WS2812 — drive in pairs or switch divisor to 22.5; keep the
  logical 8-sector model unless testing finer resolution.
- Reserved colors: yellow = low battery, green = charging. Never assign to
  members. SOS stays directional (never whole-ring), blinks faster than
  any navigation state. Full state-priority hierarchy is in the design doc.

## Hardware target

LilyGo T-Beam Supreme (×3): ESP32-S3FN8, LoRa SX1262, u-blox MAX-M10S
GNSS, QMI8658 6-axis IMU, QMC6310N magnetometer, SH1106 1.3" OLED, AXP2101 PMU, 18650 holder.
Dev environment: Arduino or PlatformIO (PlatformIO preferred for structure).

Pin map (from the product listing — VERIFY against the LilyGo wiki for this
board revision before trusting):
- SX1262: SCLK IO12, MISO IO13, MOSI IO11, CS IO10, RST IO05, DIO1 IO01,
  BUSY IO04
- GPS: RX IO09, TX IO08, WAKEUP IO07, 1PPS IO06
- OLED (I2C): SDA IO17, SCL IO18 — shared with QMC6310 magnetometer
- QMI8658 IMU (SPI): SCLK IO36, MISO IO37, MOSI IO35, CS IO34, INT IO33
- PMU AXP2101 (I2C): SDA IO42, SCL IO41, IRQ IO40
- External LED ring (WS2812): pin TBD at bring-up — pick a free GPIO,
  document it here.

Safety: NEVER transmit without the LoRa antenna attached — it can destroy
the RF output stage. Any firmware that could TX at boot must be flashed
with the antenna on.

## Working style

Owen is learning as he builds — that's half the point of the project.
Default to explaining what code does and why, not just producing it.
For core-logic changes (math, protocol behavior), propose + explain and let
him drive; for boilerplate, scaffolding, and driver plumbing, writing it
outright is fine. Incremental steps with a runnable test after each beat
big-bang changes. When something fails, reason from the failure pattern
before touching code.

Keep responses and commit messages concise. No em dashes in prose he'll
paste elsewhere is not a requirement, but he dislikes overly polished
formulaic writing.

## Near-term milestones (in order)

1. Toolchain bring-up: flash a blink/hello to one T-Beam via PlatformIO.
2. Peripheral bring-up, one board: OLED text, GPS NMEA parse to lat/lon,
   IMU + mag raw reads, LED ring lit.
3. Port compass to C++; add hard-iron calibration routine (figure-eight
   capture) — this is new work, not ported, and needs real mag data.
4. Port navigation + LED logic (direct translation of the Python).
5. LoRa link: two boards exchanging position packets (port mesh.py logic;
   real SX1262 TX/RX replaces transmit() placeholder).
6. Integration: device_tick on hardware — board A's ring points at board B.
7. Park field test + demo video. Then third board for mesh relay testing.