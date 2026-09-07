# Kandi Timeline

Dates use message timestamps, not conversation metadata. A design proposal is not marked as implemented merely because its wording sounds definitive.

| UTC timestamp | Status | Development / decision | Sources |
|---|---|---|---|
| 2026-05-21T19:13:47.269848Z | Origin | A watchlike device for finding a designated group leader at a rave; kandi aesthetic and lack of cell service are the motivating constraints. | C14-M000; Totem project created 2026-05-21T19:04:58.053400+00:00 |
| 2026-05-22T07:01:52.846325Z | User correction | Retain the aesthetic/product ambition after an assistant suggests stripping the project down for a résumé. | C15-M003–M005 |
| 2026-05-23T08:10:13.583711Z | Clarification | GNSS determines each wearer’s position; the peer radio replaces the cellular coordinate-sharing path. Initial “GPS-free” wording superseded. | C15-M006–M007 |
| 2026-05-27T01:40:24.115959Z | Design selection | Long Fast / SX1262 / 915 MHz / SF11 / 250 kHz / CR4/5 selected in the radio draft. ~110 ms airtime and ~33% utilization are early estimates, later challenged. | C15-M014–M019 |
| 2026-05-28T00:16:13.504103Z | Design | Managed flooding, hop limit 3, duplicate suppression and SNR-weighted delayed relay; skip learned next-hop routing for v1. | C15-M023–M025 |
| 2026-05-28T00:39:44.776127Z | Design, later partly reopened | MAX-M10S, nRF52840 and deferred UWB direction. nRF52840 is the paper target; it is not the later dev-board MCU and is reopened in August. | C15-M027 |
| 2026-05-29T19:17:09.804239Z | Model | Detailed power spreadsheet assumptions; continuous GNSS for initial simplicity, RX/IMU/LED duty-cycle assumptions, 400 mAh battery model. Budget workbook and matching PDF image restored on 2026-09-07; their later row values differ from this early suggested table, and their total omits overhead (see project context). | C15-M028–M035 |
| 2026-06-01T21:59:34.980947Z | User correction | Dancing in place defeats an IMU-only stationarity trigger; subsequent design uses GPS displacement over time. | C15-M034–M037 |
| 2026-06-20T14:58:42.390969Z | Design refinement | Group/focus UX, reserved battery/charge colors, directional SOS, two-button gestures and short arrival alert consolidated after user choice. | C15-M040–M049 |
| 2026-06-20T15:03:54.013609Z | Provisioning proposal | Companion-app BLE provisioning and direct-device fallback drafted, including group key/roster and A-GNSS. Still not implemented in the final prototype. | C15-M050–M051 |
| 2026-06-21T15:40:04.868018Z | Phase milestone | Architecture document and staged roadmap complete; dev-board functional prototype is next. Original phase 3 calls for miniaturization, later superseded. | C15-M053–M059 |
| 2026-06-21T16:10:25.643537Z | Learning track | Python learning begins alongside hardware-free project logic; C/Java exposure does not mean Python fluency. | C36; C15-M063 |
| 2026-06-25T06:21:08.357333Z | Software correction | Distance/bearing implementation reviewed; early coordinate/longitude error corrected. Saved later code still has a separate nonstandard cosine term. | C15-M064–M067; current navigation sources |
| 2026-06-29T15:28:08.840071Z | Software | LED mapping, managed-flooding simulation and integrated device logic developed in Python before hardware bring-up. | C15-M068–M103 |
| 2026-07-11T08:26:15.510192Z | Validated synthetic milestone | After successive tilt-formula corrections, user reports all 26 synthetic compass cases at displayed 0.000° error. | C15-M106–M114 |
| 2026-07-13T14:57:38.501899Z | Prototype preparation | Hardware-free core stack treated as ready for dev-board integration. Preserve Python as the behavioral reference. | C15-M115–M123 |
| 2026-07-19T04:18:26.642429Z | Hardware identification | Three T-Beam Supreme boards confirmed with QMC6310N; board-specific buses, rails and sensor APIs become central. | C15-M128–M170 |
| 2026-07-25T22:13:52.857437Z | Implementation handoff | A substantial CLAUDE.md snapshot is pasted/exported, preserving peripheral bring-up and porting details. | C15-M199; extracted 1785017602957_CLAUDE.md |
| 2026-07-28T23:41:39.373147Z | Recorded diagnostics | Board boot/serial log text preserved, including navigation golden-value tests and radio/GNSS diagnostics. | C15-M214 unnamed extracted attachment |
| 2026-07-30T05:26:51.761768Z | Field observation | 635 m reached in a residential walk; long-range update gaps reported. This is an observed reach, not a measured absolute limit or festival validation. | C15-M230–M231 |
| 2026-07-30T05:31:45.715207Z | Field observation and model | 11.2 dB held-clear versus chest SNR difference at 200 m; n≈3.7 and ~410 m body-worn projection derived in the reply. | C15-M232–M235 |
| 2026-07-31T01:49:57.239915Z | User reports documentation updates | Owen says the proposed design/README/résumé updates are done; battery arrival delay briefly changes near-term work. | C15-M234–M237 |
| 2026-08-14T01:07:59.944664Z | Calibration work | August hardware work resumes: battery-installed hard-iron calibration and real sensor orientation corrections. | C15-M240–M246 |
| 2026-08-16T04:01:02.502390Z | Implementation correction | Compass frame and LED ring sweep corrected; current ring mirroring is separate from sensor transformations. | C15-M246 and source |
| 2026-08-16T04:18:12.820049Z | Implementation handoff | Origin/relay logging, sticky target selection, 600 ms button split, roster UI and identity checks documented; arrival lifecycle integrated. | C15-M248–M256 |
| 2026-08-18T06:08:46.600027Z | Field relay/demo milestone | Three-board drive test shows one-hop delivery across 628 m endpoint separation, under active endpoint blocklists. | C15-M258–M259 |
| 2026-08-18T06:12:28.828208Z | User priority | Footage is obtained; user does not want an immediate repeat field run. Preserve the forced-relay caveat and move forward. | C15-M260 |
| 2026-08-18T06:33:16.464668Z | Explicit roadmap reversal | CAD first, power second, simple LED PCB before full RF board; immediate nRF52840 migration is no longer settled. | C15-M262–M269; supplied CLAUDE current phase |
| 2026-08-20T05:48:57.816706Z | Recoverable artifact | ENCLOSURE_BRIEF.md created for a dedicated CAD conversation. | C15-M271; C55-M000 attachment |
| 2026-08-20T18:39:47.428587Z | Latest mechanical corrections | GPS stays put; 28 mm already includes current headers/middle USB; hinged whip needs no adapter; left-arm axes and port/ring orientation clarified. | C55-M007–M011 |
| 2026-08-20T18:41:16.301866Z | Open CAD proposal | Tray/faceplate/diffuser parameters proposed. Connector relief, true assembled height, printer and fitting remain unresolved. | C55-M012 |
| 2026-08-27T06:17:21.530593Z | Logistics, no engineering milestone | Airport-screening question about carrying Kandi. Do not infer new technical progress or retain old travel advice as a current rule. | C57-M000–M001 |
| 2026-08-28T18:35:05.050768Z | Conflicting application draft | Assistant introduces least-squares/NumPy/scale-factor calibration, contradicting actual min/max offset firmware. Do not adopt this as a new implementation. | C52-M061 |
| 2026-08-30T18:39:32.308929Z | Writing preference | User requests plain-text résumé work and rejects explicitly labeling the project “vibecoded”; later acknowledges heavy AI assistance when discussing wording. | C56-M006–M015 |
| 2026-09-03T19:20:36.244788Z | Professional narrative caution | Résumé wording includes paired-member terminology despite bonding being unimplemented. Validate every claim against engineering evidence. | C56-M020–M023 |
| 2026-09-04T19:13:14.187607Z | Separate robotics context | Current Sensor Design Review.pdf / Teensy/KiCad control-PCB discussion belongs to robotics, despite being in the Kandi professional chat. | C56-M031–M046 |

## Source restoration, not a new engineering milestone

**2026-09-07:** `reference/Kandi.pdf` (30 pages) and `reference/Kandi Power Budgeting.xlsx` reviewed against the existing reconstruction. The PDF records the July field-update stage with calibration/mesh still pending; later August completions and CAD-first order remain authoritative. The workbook explains the 17.5 mA headline but excludes overhead and radio sleep from its sum. Neither restoration establishes new measured power performance, device work, or a changed roadmap. Historical attachment revision identity remains unknown.
