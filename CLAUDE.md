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

**Phase 2 (dev-board prototype): COMPLETE (2026-08).** On 3× LilyGo T-Beam
Supreme boards (Arduino/PlatformIO, C++): all peripherals up, all core logic
ported and self-testing at boot, compasses calibrated and frame-corrected, the
full "find your friend" pipeline working, mesh relay demonstrated in the field
across a 628m gap via a midpoint, and demo footage captured. See the milestone
list at the bottom for exact status of each piece.

**Phase 3 is NOT the design doc's phase 3.** The doc calls for an immediate
nRF52840 migration. The revised plan front-loads cheaper, higher-value work and
defers the custom RF board. In order:

1. **Enclosure CAD** (next two weeks, before school). A 3D-printed wrist-worn
   housing for the CURRENT dev-board hardware. Pip-Boy style — the T-Beam
   Supreme is ~115 × 33 × 28 mm, so forearm-length is the honest form factor
   at this stage. Explicitly a prototype housing, NOT an attempt at the product
   form factor; don't let scope creep in that direction. Deliverable: wearable
   units + a reshot demo video with the devices actually worn. Constraints have
   their own section below.
2. **Power validation.** Implement the adaptive duty cycling the design doc
   already specifies (GPS-displacement stationarity, GNSS duty-cycling while
   stationary, LED power gating), then MEASURE before/after current draw. The
   goal is converting the doc's modeled "13–16 hr / 30–40% improvement" into
   measured numbers, the same way the range claims were converted. Say up front
   that ESP32-S3 + OLED will NOT approach the 17.5 mA budget figure — that
   number was computed for different silicon. The aim is validating the
   strategy and quantifying the delta, not hitting the number.
3. **PCB work, with a stepping stone.** Before a full custom RF board, do a
   simple 2-layer PCB LED ring sized to the enclosure face — no RF, low risk,
   and it teaches the whole fab workflow (schematic → layout → fab → assembly).
   A full custom board (MCU + SX1262 + MAX-M10S) stays a longer-term ambition.

**Open decision — MCU for the first miniaturization.** The design doc picked
the nRF52840, but that choice predates the ESP32-S3 firmware that now exists.
Staying on the S3 preserves every driver and all the toolchain knowledge;
switching to the nRF52840 gains power efficiency but means rewriting every
platform-specific driver (PMU, OLED, GPS UART, IMU/mag, radio glue, NVS,
FastLED/RMT). Treat this as open rather than settled — decide it with the
power measurements from step 2 in hand.

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
  CR 4/5). ~110ms airtime per position packet. (NOTE: measured airtime on
  hardware is ~200–250ms — the 110ms figure looks low; see the LoRa firmware
  notes. Recheck before relying on it for mesh timing.)
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

## Enclosure design constraints (phase 3, step 1)

A 3D-printed forearm-worn housing for the dev boards. These constraints are
not style preferences — each one is either a measured result or a thing that
has already broken once.

- **LoRa antenna on the OUTWARD face, away from the wrist.** Field-measured
  body absorption is 11.2 dB, roughly a 2× range difference decided purely by
  antenna placement. This is the single highest-stakes placement decision in
  the whole enclosure.
- **GPS antenna faces UP when the wrist is raised to read the ring.** The
  reading pose is the pose that has to hold a fix.
- **No ferrous fasteners near the magnetometer** — nylon or plastic screws
  only. Steel hardware becomes a hard-iron offset that rides with the device.
- **Recalibrate the magnetometer after assembly** (`calclear` then `cal`, per
  board). The enclosure changes the magnetic environment the same way the
  18650 did; calibrate in the exact configuration the device will run in.
- **Keep physical access to:** USB-C (reflashing), the BOOT and RESET buttons,
  the LED ring face, and the OLED. The OLED is still the field-debug readout
  even though the product is screenless.
- **Diffusion layer over the LED ring.** Bare WS2812s are harsh point sources;
  the ring needs to read as sectors, not as dots.

## Firmware bring-up notes (hard-won — read before touching any peripheral)

These cost real debugging time during OLED bring-up. Firmware lives in
`Firmware/` (PlatformIO). Thin hardware drivers: `power.cpp` (PMU),
`oled.cpp`/`oled.h` (display), `gps.cpp`/`gps.h` (GNSS), `imu.cpp`/`imu.h`
(QMI8658), `mag.cpp`/`mag.h` (QMC6310N), `radio.cpp`/`radio.h` (SX1262 LoRa).
Ported core logic (hardware-free, mirrors `CoreLogic/`): `compass.cpp`/`.h`,
`navigation.cpp`/`.h`, `ledlogic.cpp`/`.h`, `mesh.cpp`/`.h`, with
`selftest.cpp`/`.h` + `mesh_test.cpp` running the ported logic against Python
golden values / network.py scenarios at boot. `calibration.cpp`/`.h` is the
mag hard-iron calibration (serial-command driven, NVS-persisted).
`roster.cpp`/`.h` tracks last-known position per mesh member (fixed 8 slots)
— the nav target is the most recently heard member.

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

**IMU + magnetometer (QMI8658 / QMC6310N).** Both responding. Drivers are
`imu.cpp`/`imu.h` and `mag.cpp`/`mag.h`, using `lewisxhe/SensorLib`. Rails
(ALDO1/ALDO2) were already correct in `initBoardPower()` — SensorLib's own
T-Beam Supreme examples do the identical disable → 250ms → 3300mV → enable
dance.

- **SensorLib version skew — check before writing code.** The PlatformIO
  registry ships **v0.4.1**, whose API is `SensorQMI8658.hpp` /
  `SensorQMC6310.hpp` with `configAccelerometer(ACC_RANGE_8G, ACC_ODR_1000Hz,
  LPF_MODE_0)`, `getAccelerometer(x,y,z)`, `getDataReady()`. GitHub `master` is
  AHEAD of that and uses different umbrella headers (`ImuDrv.hpp`,
  `MagnetometerDrv.hpp`) — its examples will not compile against the release.
  Read the headers actually installed under `.pio/libdeps/`, not the repo.
- **`getAccelerometer()` returns g, not m/s².** It scales raw counts by
  range/32768, so at rest an axis reads ~1.0. `imuRead()` converts to m/s²
  explicitly. Miss this and the gravity vector is 9.8x too small.
- **Magnetometer must be configured into CONTINUOUS_MEASUREMENT.** It boots in
  suspend and returns plausible-looking stale data otherwise — a failure mode
  that reads as "working" until headings make no sense.
- **Use FS_8G for the mag, not FS_2G.** Measured field on this board is ~299 µT
  total (see below). FS_2G is only 200 µT full scale — the Y axis alone would
  saturate. FS_8G (800 µT) has ample headroom and 16-bit resolution is still
  plenty.

**Interpreting the readings (baseline, board flat on a table):**
- Accel `0.5, 0.9, 10.0` m/s² → magnitude 10.05 vs 9.81 expected (+2.5%, normal
  part tolerance). Gravity almost entirely in Z = board is flat, Z is vertical.
  Residual X/Y implies ~6° apparent tilt: zero-g bias plus a non-level table.
  Tilt compensation derives pitch/roll from this vector, so that bias becomes a
  few degrees of heading error — consider capturing accel offsets during the
  milestone-3 calibration too.
- Mag `169, -246, 9` µT → magnitude **299 µT** against Earth's 25–65 µT. This is
  expected **hard-iron offset** (~250 µT), almost certainly the 18650 holder's
  steel springs and board components: a constant additive vector in the sensor
  frame. It is exactly what the figure-eight calibration removes — rotating
  through all orientations traces a sphere of radius ~50 µT whose *center* is
  the offset; subtract the center to recover the real field.
  Rule out environmental sources first (laptop lid magnets and phone speakers
  are strong enough to swamp this): re-read 30cm away from any laptop/phone. A
  large change means environmental — which moves with you and does NOT
  calibrate out. Unchanged means board hard iron, which is fixed and does.
- **A static reading cannot distinguish "working" from "frozen at plausible
  values."** The real confirmation is motion: spin the board flat and X/Y should
  each swing ~±20 µT around their offsets (the horizontal component is only
  ~20 µT in the continental US — the field points steeply down), while Z barely
  moves. For accel, tip the board onto each edge and watch the ~9.8 migrate
  between axes.

**Sensor axis frames (the tilt-compensation saga — will bite again on the
custom PCB).** The compass math only works if accel and mag report in ONE
shared frame. On this board they don't, out of the box. Root cause, worth
internalizing: **both chips are physically right-handed (all real silicon is),
but the verified Python math embodies a LEFT-handed frame** (X fwd, Y right,
Z up) — so each sensor needs exactly one reflection to match it. On the
T-Beam Supreme (confirmed 2026-08 by 4-pose capture):
- QMC6310N mag: Z inverted → `MAG_FLIP_Z 1` in mag.h.
- QMI8658 accel: X/Y swapped (accel X = board-right, accel Y =
  board-forward) → `IMU_SWAP_XY 1` in imu.h. Applied to gyro too.
- Remaps live at the DRIVER boundary (mag.cpp/imu.cpp), never in compass.cpp.
- **Changing any MAG remap invalidates stored calibration** (offsets are
  captured in the remapped frame) — `calclear` + `cal` after. IMU-side remaps
  don't touch calibration.
Symptom signatures, for diagnosis:
- Frame mismatch is INVISIBLE flat (mz drops out of tilt compensation at
  0/0 pitch/roll; accel X/Y don't matter at zero tilt) and wrecks headings
  under tilt: "right when flat, wild when tilted" = frames, not calibration.
- Discriminator: |M| steady through the tilt = frame problem; |M| swinging =
  calibration problem. (Reflections preserve magnitude; bad offsets don't.)
- Quick vertical-axis check: flat + calibrated, corrected mz must read ≈ −40
  µT here (field points steeply DOWN; down = negative on an up axis). +40 =
  mag Z flipped.
- Full diagnosis: 4-pose capture — flat N, flat E, antenna-end straight down,
  right-edge down — reading the corrected mag + accel serial lines. The two
  GRAVITY poses are the decisive ones (no compass-pointing slop): whichever
  axis catches ±9.8 / the +40 µT down-field names itself.
Verified after the fix: heading holds through tilt to ~2-3°, pitch/roll land
on the correct axes, and two boards point at each other through the full
mesh → compass → ring pipeline.

**Ring orientation:** `RING_MIRRORED 1` (main.cpp) — the pointer swept the
wrong way with a proven-correct compass, i.e. the physical LED index order
runs CCW from the viewing side. `RING_OFFSET_LEDS` rotates LED 0 to 12
o'clock once the ring is mounted; while it dangles on jumpers, hold it with
LED 0 toward the antenna (the current "forward"). Both knobs are
display-layer only (`physIndex()`), per the mapping-stays-out-of-logic rule.

## Ported core logic (C++ on hardware)

These mirror the verified `CoreLogic/` Python. The rule from the architecture
principle holds: **translate exactly, don't "improve" the math** — the Python
was bug-hunted and the ports are checked against it, so any "cleanup" is a
divergence waiting to bite in a field test.

**Compass (`compass.cpp`/`.h`).** Direct port of `tilt_compensation.py`:
`pitchRoll()`, `tiltCompensate()`, `headingFrom()`, plus `compassHeading()`
that samples both sensors and runs the pipeline.
- **`float`, not `double`** — the S3 FPU is single-precision (doubles are
  software-emulated), and float's ~7 digits is four orders of magnitude finer
  than the 0.5° tolerance. This is the *opposite* choice from navigation.cpp
  (see below) and the reason is deliberate: angles are small numbers, coords
  are not.
- The asymmetric `tiltCompensate` form (Xh has three terms, Yh two) is correct
  and load-bearing — verified, do not "balance" it.
- **`compassHeading()` returns the raw accel/mag it sampled** so callers don't
  re-read. `imuRead()` gates on a data-ready flag, so a second immediate call
  comes back empty — reading once and passing the samples out avoids a
  perpetually dead heading. main.cpp relies on this.
- **Hard-iron offsets (`magOffsetX/Y/Z`, extern) are populated at boot** by
  `calBegin()` from NVS — see the calibration block below. Subtracted in the
  sensor frame *before* rotation. On a board with no stored calibration they
  stay zero and heading is badly biased and barely moves (the ~250 µT offset
  dwarfs the ~20 µT horizontal signal) — the OLED flag shows `---` in that
  state.
- Verified: the actual `compass.cpp` compiles on the host against a stub
  `Arduino.h` and passes the same 26-case round-trip as `compass_test.py` at
  err=0.0000. Links the real firmware source, not a copy.
- **Declination:** the mag yields MAGNETIC heading; bearing() is TRUE-north.
  `compassHeading()` adds `MAGNETIC_DECLINATION_DEG` (13.0 = Saratoga,
  hardcoded, location-specific — change it if testing elsewhere) as its final
  step. `c.heading` is true (use against bearing()); `c.headingMag` is
  magnetic (display). OLED shows both as `H<mag>/<true>` — they must differ
  by exactly 13.0. `headingFrom()` itself stays purely magnetic.

**Hard-iron calibration (`calibration.cpp`/`.h`).** DONE on all 3 boards
(2026-08). Min/max per axis over a 30s capture, center = (max+min)/2; soft
iron (ellipsoid distortion) deliberately NOT corrected in v1. Serial commands
into the pio monitor: `cal` starts a capture, `calclear` wipes it.
- **Offsets persist in NVS** (Preferences, namespace "kandi") — they survive
  reboot AND reflash (own flash partition). Loaded at boot by `calBegin()`
  into compass.cpp's `magOffsetX/Y/Z`; compass.cpp itself was not touched.
- **Per-board data, same firmware.** Hard iron differs board to board; each
  board stores its own numbers. A new board (or after `calclear`) needs its
  own 30s capture.
- **The validation signal is |M| on the HDG line** (`HDG 234.5 M 48 CAL`):
  magnitude of the corrected field. Good calibration = ~25–65 µT and steady
  at ANY orientation (measured 46–56 on these boards). Swinging with
  orientation = bad coverage, redo. Near a laptop it legitimately reads
  70–95+ — that's the laptop's field, not a calibration failure; it's why
  the number is on screen. `CAL`/`---` flag = whether stored offsets are
  loaded, i.e. whether heading deserves any trust.
- **Capture technique:** figure-eights are not enough — FLIP the board over
  mid-capture or ±Z never sees both extremes. The OLED shows per-axis spread
  live with `LOW <-` marking the lagging axis; each axis wants ~100 µT
  (2× Earth's field). Keep the board an arm's length from the laptop during
  capture (its lid magnet bakes into the offsets and walks away afterward).
- **Recalibrate after any hardware change near the mag** — and calibrate in
  the configuration the board runs in: an 18650 in the holder is a large
  hard-iron contributor, so battery-in vs battery-out are different
  calibrations.
- Capture reads RAW mag (offsets only apply inside `compassHeading()`), so
  re-running `cal` on a calibrated board can't double-subtract. Capture is
  non-blocking (~0.2ms I2C read per loop pass); it only takes over the OLED.

**Navigation (`navigation.cpp`/`.h`) + LED logic (`ledlogic.cpp`/`.h`).**
Direct ports of `navigation.py` and `LEDLogic.py`.
- **`double`, not float, for navigation** — a lat like `37.2755809` spends all
  ~7 of a float's digits before the part that distinguishes you from a friend
  50m away; float coords quantize position to meters and make short
  distances/bearings garbage. Runs once per position update, not per loop, so
  the software-emulated double cost is irrelevant.
- **Python `%` vs C `fmod` sign trap.** Python's `%` is never negative for a
  positive divisor; C's `fmod`/`fmodf` keep the dividend's sign. `norm360()`
  (nav) and `norm360f()` (led) reproduce the Python so `(x + 360) % 360`
  ports faithfully. Both `bearing()` and `relativeBearing()` route through
  these.
- **The haversine keeps `cos(lat1)*cos(lat1)`**, not the textbook
  `cos(lat1)*cos(lat2)`. That's what the Python has and what the real-world
  tests validated; for group members within a few km the two agree to ~7
  decimals. Commented in the source so nobody "fixes" it into a divergence.
- **`ledForBearing` uses `roundf` (half away from zero); Python `round` is
  half-to-even.** They only disagree on exact multiples of 22.5°, which no
  real reading hits. Documented in-source.
- **Logical→physical LED mapping lives in the display layer (main.cpp
  `renderRing()`), NOT in `ledForBearing`.** Logical sector `s` (8-sector
  model, 0 = top, clockwise) lights physical pair `{2s, 2s+1}` on the 16-LED
  ring. If the ring mounts rotated, fix it with an offset there — never in the
  logic function. Assumes physical LED 0 at 12 o'clock, indices clockwise.

**Boot self-tests (`selftest.cpp`/`.h`).** `runSelfTests()` runs first thing
in `setup()` (pure math, needs only serial) and prints PASS/FAIL per case. The
expected values are **golden values captured from the verified Python**, not
re-derived — so a FAIL means the C++ diverged from `CoreLogic/`, caught on the
bench instead of in the field. Covers distance (same-point ≈ 0, Taipei 101
pair = 2070.5m), cardinal bearings, relative_bearing, and led boundary cases
(44→1, 46→1, 359→0 wrap). Also runnable on the host — see
`scratchpad/compasstest/` for the pattern (links real firmware sources).

**Nav wired to the ring (main.cpp).** The chase demo is gone. Test waypoint is
`TARGET_LAT`/`TARGET_LON` (#defines, currently ~100m N of the Saratoga house,
geocoded + verified 100.1m/360° through the ported haversine). Two ring states:
steady two-pixel pointer when a fix + heading are live (`RING_POINTING`), and a
slow single-pixel breathing pulse at index 0 when not (`RING_SEARCHING`) — so
"searching" and "pointing" are distinguishable at a glance. The nav math runs
on the 1s tick; the ring redraws on a fast ~25ms tick so the pulse is smooth.
A one-tick compass dropout holds the last heading rather than flickering to
searching; GPS validity is not held (gpsStatus() has its own 5s freshness).

**LoRa radio (`radio.cpp`/`.h`, SX1262 via `jgromes/RadioLib` 7.7.1).** Two
boards exchanging position packets — milestone 5. Pins + init sequence verified
against LilyGo's **Factory** example (`utilities.h` pin block +
`Factory.ino` setup), `T_BEAM_S3_SUPREME_SX1262` branch — read the raw source
and mind the per-board `#ifdef`s, same discipline as the GPS rails.

- **`setDio2AsRfSwitch(true)` is mandatory and non-obvious.** On this module
  DIO2 drives the TX/RX antenna switch. Without it every SPI command still
  "succeeds" — the radio configures fine, `begin()` returns OK — but TX
  radiates almost nothing and RX hears almost nothing, because the antenna is
  never connected to the active path. A datasheet-only bring-up misses this;
  it's in LilyGo's `setupRfSwitch()`. Also `setCurrentLimit(140)` (their
  value; a ceiling, not a target).
- **The radio owns the GLOBAL `SPI` object (FSPI, pins 12/13/11)**, started
  with `SPI.begin(SCLK, MISO, MOSI)` like LilyGo does. This is the payoff for
  imu.cpp deliberately using its own `SPIClass(HSPI)` back in milestone 2 —
  the two SPI buses are physically separate and stay that way. Module ctor arg
  order for the SX1262 is `(CS, DIO1, RST, BUSY)`.
- **Sync word left at RadioLib's private default (0x12).** Meshtastic uses
  0x2B, so we're deliberately invisible to Meshtastic nodes even on the shared
  Long Fast preset. Preset is the locked-in one: 915 MHz, SF11, 250 kHz, CR
  4/5, +14 dBm — do not tune.
- **Non-blocking via DIO1 interrupt.** `startTransmit`/`startReceive` return in
  ~1ms; the airtime happens inside the chip. The DIO1 ISR only sets a flag
  (`IRAM_ATTR`, no SPI in an ISR — it would collide with a transfer in flight);
  all real work is in `radioTick()`, called every loop pass like `gpsPump()`.
  So airtime never stalls the loop and the GPS UART keeps draining.
- **`IS_SENDER` is a COMPILE-TIME toggle (radio.h).** One codebase, one line
  changed, flashed to each board. **You must build+flash twice** — the same
  binary on both boards makes two senders (this actually happened: both showed
  `TX` on the OLED header, which is the tell — sender header shows `TX`,
  receiver `RX`). Safe habit: set the toggle, flash immediately, one board at a
  time (avoids guessing USB ports). Sender TX begins seconds after boot →
  **antenna on before flashing a sender build.**
- **Packet format is versioned.** `TestPacket` is 17 bytes packed: 4-byte magic
  `"KND2"`, `uint32` counter, lat/lon as `int32` in 1e-7 degrees (u-blox native
  format — fixed width on air, ~1cm resolution, 8 bytes for the pair vs 16 for
  two doubles), `uint8` fix-valid flag. The magic's version digit is bumped on
  any layout change so a board on stale firmware gets *rejected*, not
  misparsed. No fix yet still transmits (flag false) so the link is testable
  indoors. Receiver computes true GPS distance with the ported `distance()`
  only when both ends hold a fix.
- **Measured airtime is ~200–250ms, NOT the design doc's ~110ms.** The
  SF11/250kHz math on even a tiny packet lands well above 110ms. Doesn't matter
  for the range test (non-blocking), but it affects mesh airtime/relay-delay
  budgeting later — recheck the design doc's number against measured TX-done
  timestamps before porting the mesh timing.
- **`gps.failedChecksum()` is not yet surfaced through `GpsStatus`.** If fixes
  degrade once the radio is active (the RF-interference symptom), that counter
  is the proof — a small additive change to the struct will expose it.

**RSSI/SNR for range testing.** RSSI (dBm) is raw received power, more negative
= weaker. SNR (dB) is signal above the noise floor and is the *cliff*
indicator: LoRa decodes *below* the noise (SF11 works to about **−17.5 dB
SNR**), and links don't fade gracefully — they fall off within a few dB.
Rough guide: RSSI > −90 / SNR > 5 = same room; −90 to −110 / 0 to 5 = healthy
outdoor link; −110 to −125 / 0 to −15 = long range, working as designed; SNR
approaching −17 with CRC errors climbing = the edge, note the distance. The
receiver holds a **max-distance-ever-received** figure on screen (survives link
death — it's the test result) and shows **seconds-since-last-packet** (age),
because everything else freezes on stale values when the link dies; age is what
tells a live link from a frozen display. Serial logs one line per packet
(`grep "LORA: RX"`) with counter, distance, RSSI, SNR for the RSSI-vs-distance
plot; gaps in the counter sequence are packet-loss-vs-distance for free.

**Toolchain:** `pio` isn't on PATH — use `~/.platformio/penv/bin/pio`. Env is
`[env:tbeam-supreme]`, board `esp32-s3-devkitc-1` (generic S3; flashes and
runs fine). Build + flash + watch serial in one shot:
`~/.platformio/penv/bin/pio run -t upload -t monitor` (Ctrl-C quits monitor).

## Deferred (deliberately — not missing)

These are specified in the design doc and consciously NOT built yet. Don't
treat them as gaps or "fix" them unasked; each is scheduled behind something
with more value per hour right now.

- **Group view / focus mode** — multi-member display. A minimal stand-in
  exists (short-press cycles the nav target, long-press shows the roster
  page); the full design-doc interaction is later.
- **Full button gesture map.** Currently three gestures on the BOOT button.
- **BLE bonding.** `group_id` is a compile-time constant; real groups need
  the bonding flow and a shared AES key.
- **SOS.** Reserved in the LED state hierarchy, not implemented.
- **Haptics.** No motor on the dev board — this is blocked on custom hardware,
  and it is the design doc's answer for final approach alongside UWB.
- **Unforced relay rerun.** Redo the 628m relay test with empty
  `MESH_BLOCKED_SENDERS` to show the mesh self-organizing around a genuinely
  dead link rather than a simulated one. See milestone 7 for why this matters
  to how the result is worded.

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

Items 1–7 are phase 2 and are all DONE — kept as a log of what was verified
and how, since the "how" is what a later regression gets checked against.
Phase 3 (8–10) is summarized in Current phase at the top of this file.

1. Toolchain bring-up: flash a blink/hello to one T-Beam via PlatformIO.
   — DONE.
2. Peripheral bring-up, one board: OLED text, GPS NMEA parse to lat/lon,
   IMU + mag raw reads, LED ring lit.
   — DONE: PMU power-up, OLED text, WS2812 ring (chase demo), GPS (lat/lon +
     sats/HDOP), IMU + mag raw reads. See firmware notes above for what the
     baseline sensor values mean.
3. Port compass to C++; add hard-iron calibration routine (figure-eight
   capture) — this is new work, not ported, and needs real mag data.
   — DONE (2026-08): compass ported + `calibration.cpp` (min/max capture,
     NVS-persisted). All 3 boards calibrated; |M| holds 46–56 µT through
     rotation on each. Headings are live and trustworthy away from large
     metal/electronics. See the calibration block in the firmware notes.
4. Port navigation + LED logic (direct translation of the Python).
   — DONE: `navigation.cpp` + `ledlogic.cpp`, boot self-tests against Python
     golden values (`selftest.cpp`), and wired to the physical ring (pointer
     vs searching states) with a test waypoint.
5. LoRa link: two boards exchanging position packets (port mesh.py logic;
   real SX1262 TX/RX replaces transmit() placeholder).
   — DONE, desk-verified on 3 boards (2026-08). `radio.cpp` (non-blocking
     SX1262, versioned wire format) + `mesh.cpp`/`mesh.h` (managed flooding:
     32-slot seen cache w/ 30s expiry, hop limits, SNR-weighted relay queue
     200-2000ms, duplicate-cancels-relay) + `mesh_test.cpp` (network.py
     scenarios as boot self-tests). MESH_BLOCKED_SENDERS in mesh.h simulates
     out-of-range per board — filters on the tx_node byte (the TRANSMITTER),
     not the originator; blocking the originator would kill relayed copies
     too (that bug happened; wire format bumped KNDM->KNDN adding tx_node).
     Verified: line topology 1<->2<->3 on a desk, `S1 h2`/`S3 h2` on the end
     boards' OLEDs = positions crossing the simulated gap via board 2, dedup
     holding R at ~2x T. OLED `h` = hops REMAINING (3 = direct, 2 = one
     relay). Known desk artifact: point-blank SNR clamps all relay delays to
     2000ms, so end-board relays sometimes collide (small R loss); real SNR
     spread or added jitter fixes it.
6. Integration: device_tick on hardware — board A's ring points at board B.
   — CORE DONE (2026-08): boards 2 and 3 point at each other live through
     mesh → roster (`roster.cpp`, last-known position per member, 8 slots) →
     nav → compass → ring. Ring states per design doc in main.cpp:
     NAVIGATING (blink rate = device.py distance buckets), STALE >30s (dim
     breathing at last bearing), LOST >120s (magenta pulse, no bearing),
     ARRIVED <10m (8s full-ring flash → steady, BOOT button cancels, 15m
     re-arm hysteresis), NO_FIX (blue pulse). Verified: pointer tracking,
     tilt hold ~2-3°, stale/lost transitions, both startup pulses, and the
     full ARRIVED lifecycle (walk-in test 2026-08: flash fires at the 10m
     crossing, decays after 8s per spec — it is NOT supposed to stay
     flashing — and re-arms only after a genuine >15m excursion for one
     beacon cycle; standing at ~10m flickers ARRV/NAV on page 5, which is
     GPS noise at the threshold, cosmetic). Tunables if the feel is wrong:
     flash duration (8000ms) and re-arm distance (15m) in main.cpp; don't
     drop re-arm below ~13m or threshold noise re-fires the flash.
   — FIELD-VERIFIED (2026-08): 3-board drive test. Blink-rate-closes and the
     ARRIVED flash both confirmed at range. Multi-member roster page correct
     in the field: pointer to a live member, distances at 264m/628m, states
     transitioning. Also caught a real LOST→reacquire in the serial log
     (member aged to 364s, then NAVIGATING age 0 when it came back).
7. Third board / mesh relay at real range.
   — FIELD-VERIFIED (2026-08): board 3 home (tethered, logging), board 2
     dropped ~300m out as bridge, board 1 driven to 628m. Board 1's roster
     showed member 3 at **628m, h1, d0 r15** — board 3's position delivered
     ONLY via board 2's relay, RSSI -118 dBm / SNR -13.2 dB (~4dB over the
     SF11 floor). Relay across a real-range gap, working.
     CAVEAT for write-ups: the MESH_BLOCKED_SENDERS sim-blocks were STILL ON
     during this run (board 3's log is full of dropped_blocked from board 1),
     so the 1<->3 topology was FORCED, not proven distance-driven. At 628m the
     direct link was almost certainly dead anyway (own data: 635m max, ~410m
     body-worn; 1<->3 direct was already SNR -21 when closer) — but the block
     masks the proof. For an airtight "unforced self-organization" claim, rerun
     with empty blocked-lists at the same distance. Until then the honest
     framing is "relay demonstrated at 628m via a midpoint", not "mesh
     self-organized around a dead link".
   Note: |M| ran ~63uT in the field vs 46-56 on the bench (nearby car steel,
   likely) — headings stayed stable/sensible, but recal if enclosing near metal.

--- phase 3 (current) — see Current phase for the reasoning behind the order ---

8. Enclosure CAD: 3D-printed forearm housing for the dev boards, wearable
   units + reshot demo video with the devices worn. Constraints are in the
   Enclosure design constraints section. Recalibrate every board after
   assembly.
9. Power validation: implement the design doc's adaptive duty cycling
   (GPS-displacement stationarity, GNSS duty-cycle when stationary, LED
   gating), then measure before/after current draw. Deliverable is a measured
   delta, not a target number — the 17.5 mA budget figure was computed for
   different silicon.
10. PCB stepping stone: 2-layer LED ring board sized to the enclosure face,
    no RF. Full custom board (MCU + SX1262 + MAX-M10S) after that, with the
    MCU choice (ESP32-S3 vs nRF52840) decided using milestone 9's numbers.
