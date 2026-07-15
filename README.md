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
- ✅ **Phase 2 software — complete, pre-hardware.** The full navigation and
  networking stack is implemented and unit-tested in platform-independent
  Python, validated in simulation before dev boards arrive.
- ⏳ **Phase 2 hardware — next.** Porting to LilyGo T-Beam dev boards for a
  two-device field demo.

## What's implemented

| Module | What it does | Verified by |
|---|---|---|
| `navigation.py` | Haversine distance + initial bearing between GPS coordinates | Tested against real-world coordinates and cardinal-direction cases |
| `compass.py` | **Tilt-compensated compass**: recovers true device heading from raw accelerometer + magnetometer at any wrist orientation | Synthetic round-trip tests — generates fake sensor readings for known orientations, algorithm recovers heading to <0.5° at all pitch/roll/heading combinations |
| `LEDLogic.py` | Maps target bearing + device heading → correct LED on the 8-LED ring | Boundary, rounding, and wraparound cases |
| `mesh.py` | Per-device mesh logic: packet structure, duplicate suppression, hop-limited relay decisions, group separation | All four receive-path outcomes |
| `network.py` | Multi-device flooding simulation with configurable topology | Relay across non-adjacent nodes, loop termination via dedup, hop-limit expiry |
| `device.py` | Integration loop: member positions in → full LED display state out | End-to-end pipeline with heading changes |

The design principle: all core logic is hardware-independent. On real
hardware, only the sensor/radio I/O layer changes — the algorithms above
run unmodified.

## Roadmap

Dev-board prototype (2-device "find your friend" demo) → miniaturized
integrated unit → wrist form factor → scaled field testing at progressively
larger events. See the design doc for the full plan and open risks.