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
- External LED ring (WS2812): 16-LED ring, data-in IO2. Free GPIO, not an
  ESP32-S3 strapping pin. Driven with FastLED (RMT); ring powered from 3V3.

Safety: NEVER transmit without the LoRa antenna attached — it can destroy
the RF output stage. Any firmware that could TX at boot must be flashed
with the antenna on.

## Firmware bring-up notes (hard-won — read before touching power/I2C/OLED/GPS)

These cost real debugging time during OLED bring-up. Firmware lives in
`Firmware/` (PlatformIO). Thin hardware drivers so far: `power.cpp` (PMU),
`oled.cpp`/`oled.h` (display), `gps.cpp`/`gps.h` (GNSS).

**Power-on order: init the PMU FIRST.** The AXP2101 gates every peripheral
3.3V rail, and they boot OFF. The ESP32 itself runs from an always-on rail
(DCDC1), so serial prints fine with zero PMU setup — but the OLED, GPS, IMU,
mag, and LoRa are all dark until their rails are enabled. `initBoardPower()`
in `power.cpp` does this (`lewisxhe/XPowersLib`). The PMU is on a SEPARATE
I2C bus: **Wire1, SDA 42 / SCL 41** (OLED/mag are on Wire, 17/18). Rail map
ported from LilyGo's Factory firmware, `T_BEAM_S3_SUPREME` branch: ALDO4=GPS,
ALDO3=LoRa, ALDO1/ALDO2=sensors (IMU+mag), BLDO1/2=SD, DCDC3/4/5=expansion.
No rail is individually labeled "OLED" — bring up the whole set. Chip ID
reads back 0x4A.

**The 17/18 I2C bus addresses are a trap:**
- `0x3C` = **QMC6310N magnetometer**, NOT the OLED. The QMC6310**N** variant
  answers at 0x3C — which is also the default OLED address most libraries
  assume. Display writes there ACK cleanly but do nothing (and wedged u8g2).
  This was the single biggest gotcha on this board.
- `0x3D` = **SH1106 OLED.** Detect at runtime, don't hardcode: read register
  0x00 from each of 0x3C/0x3D — the mag returns chip-id `0x80`, the OLED does
  not (LilyGo's own disambiguation). `main.cpp` does this in setup.
- `0x77` = BME280 (if populated).
- Pins on both buses (17/18 and 42/41) and the SH1106 model are confirmed
  against LilyGo source, not just the product listing.

**Do NOT use u8g2 on this board.** On ESP32-S3 + arduino-esp32 2.0.17, u8g2's
HW-I2C path puts the bus into a state where every transfer takes ~1 second (a
full frame ~60–80s — it looks like a hang). Ruled out clock speed, Wire buffer
size, and the internal re-`Wire.begin`; root cause never fully pinned down.
Raw `Wire` transfers on the same bus are fast and clean: a full 8-page frame
is ~24ms at 400kHz (measured; the bus is happy up to 400kHz). So we drive the
panel with a thin hand-rolled driver, `oled.cpp`/`oled.h`: own init sequence
(SH1106 DC-DC on via `0xAD,0x8B`), framebuffer pushed as 8 page writes,
built-in 5x7 font. Fits the thin-hardware-driver principle above.
- SH1106 quirk: 132-column RAM, visible columns are 2–129, so pages are
  written with a **2-column offset** (`0x02`/`0x10`).
- Wire caveat: a full 128-byte page + the `0x40` control byte = 129 bytes,
  which overflows the default 128-byte ESP32 Wire TX buffer (drops the last
  column). Harmless while text stays left of column 127; call
  `Wire.setBufferSize(256)` before drawing full-width.

**WS2812 ring (LED_PIN = IO2).** Driven with `fastled/FastLED`, RMT-backed on
the S3 — so `FastLED.show()` does NOT block interrupts/I2C, and the OLED keeps
updating alongside it. Power safety is non-negotiable: 16 WS2812s at full-white
draw ~1A and will brown out the board. ALWAYS `FastLED.setBrightness(25)` +
`FastLED.setMaxPowerInVoltsAndMilliamps(5, 500)` before writing any pixel, and
never write full-brightness white. Color order for WS2812/WS2812B is GRB.

**GPS (u-blox MAX-M10S).** Working: 11 sats / HDOP 1.0 / correct coords on
first field check. `mikalhart/TinyGPSPlus` parses; `gps.cpp` is the driver.
- **Two independent switches, not one.** The ALDO4 rail (already in
  `initBoardPower()`) is necessary but NOT sufficient — the module also has its
  own enable pin, **IO7, active HIGH**, which boots low. Rail on + enable low is
  a totally silent UART that looks exactly like a dead rail. LilyGo drives this
  in `beginGPS()`, separately from the PMU block.
- **Pin names are from the ESP32's side:** GPS_RX IO9 = *we receive* (wired to
  the module's TX), GPS_TX IO8 = we transmit. Swapping them also produces
  silence, indistinguishable from the two failures above — hence the
  `GPS_DEBUG_RAW` toggle at the top of `gps.h`: set it to 1 to echo raw bytes
  to serial and settle "is it talking at all?" before blaming the parser.
  1PPS is IO6 (input, unused so far). 9600 baud, SERIAL_8N1.
- **Use a hardware UART, and pump it every loop pass.** `HardwareSerial(1)` —
  `Serial` is USB CDC on this build (`ARDUINO_USB_CDC_ON_BOOT=1`), not a UART,
  so it can't reach IO8/9 at all. At 9600 baud the module pushes ~960 B/s into
  a 256-byte driver buffer, so **anything blocking the loop >250ms overruns it
  and shreds sentences**. This is why `loop()` uses a `millis()` tick instead of
  `delay(1000)`. Watch out when LoRa TX lands (~110ms airtime): if fixes get
  flaky then, `gps.failedChecksum()` is the counter that proves it.
- Verified against LilyGo's `LoRaBoards.cpp`, `T_BEAM_S3_SUPREME` branch. Note
  that file has per-board `#ifdef` blocks — read the raw source and check which
  branch you're in. (ALDO4=GPS, ALDO3=LoRa. A summarized read of that file got
  this backwards.)
- HDOP describes satellite *geometry*, not absolute error — real accuracy is
  still ~2-3m at HDOP 1.0. Consistent with the design doc deferring final
  approach to UWB + haptics.

**Toolchain:** `pio` isn't on PATH — use `~/.platformio/penv/bin/pio`. Env is
`[env:tbeam-supreme]`, board `esp32-s3-devkitc-1` (generic S3; flashes and
runs fine). Build + flash + watch serial in one shot:
`~/.platformio/penv/bin/pio run -t upload -t monitor` (Ctrl-C quits monitor).

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
   — DONE.
2. Peripheral bring-up, one board: OLED text, GPS NMEA parse to lat/lon,
   IMU + mag raw reads, LED ring lit.
   — PMU power-up, OLED text, WS2812 ring (chase demo), and GPS (lat/lon +
     sats/HDOP to OLED and serial) DONE (see firmware notes above). IMU and
     mag still to do.
3. Port compass to C++; add hard-iron calibration routine (figure-eight
   capture) — this is new work, not ported, and needs real mag data.
4. Port navigation + LED logic (direct translation of the Python).
5. LoRa link: two boards exchanging position packets (port mesh.py logic;
   real SX1262 TX/RX replaces transmit() placeholder).
6. Integration: device_tick on hardware — board A's ring points at board B.
7. Park field test + demo video. Then third board for mesh relay testing.