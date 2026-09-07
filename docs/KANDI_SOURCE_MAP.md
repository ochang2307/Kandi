# Kandi Source Map

This is a traceability guide for the completed reconstruction, updated 2026-09-07 for two restored sources. The original 58-conversation analysis was not repeated. Message labels are zero-indexed positions in the original export; the machine-readable `source_material/message_index.json` carries full conversation/message UUIDs, parent UUIDs, sender roles and timestamps. The original export is retained in the private archive.

## Coverage and chronology rules

- All **58 conversations / 1,192 messages** in the supplied export were parsed and accounted for. Three supplied project JSONs and the account memory JSON were also inspected; the two supplied repository documents were read and compared with project-embedded copies.
- Content, attachment text and tool records were searched, then terminology discovered in Kandi was used to check for cross-conversation material. This went beyond titles. Nine core/supporting/adjacent conversations are retained in the Kandi source collection: **540 messages** in their complete text transcripts. They are not all exclusively about Kandi.
- No duplicate conversation UUID or message UUID was found. Parent references resolve to exported messages or the export’s root sentinel. Conversation boundaries came from JSON structure and UUIDs, not an assumption that one file equals one conversation.
- The full export has **361 file-reference occurrences / 344 unique reference IDs**. The included Kandi/supporting conversations account for **146 unique reference IDs**. Eleven extracted attachment occurrences contain ten unique text bodies; exact text duplicates are stored once with all source occurrences retained.
- Message timestamps determine sequence; later explicit user decisions take priority over earlier proposals. Conversation metadata update times, assistant confidence, and polished resume text are not independent authority.
- The 49-file repository snapshot is supplemental evidence of the local source at migration time. It was not compiled, flashed or modified for this task. Device state, the exact binaries on all boards, NVS values, and still-absent images/PDFs/CAD contents remain unknown. The two restored files below were separately reviewed and do not change those historical export counts.

## Conversation inventory

### C14 — Rave friend-finding wearable device

- **Role:** Precursor / core. Original wearable concept, designated leader, cultural/aesthetic motivation, initial aspirational form factor. Resolve erroneous GPS-free wording against later work.
- **Conversation UUID:** `06446659-93c8-4060-ab89-54459e1fcb3a`
- **Created:** `2026-05-21T19:13:46.472821Z`
- **Metadata updated:** `2026-05-21T19:27:06.702945Z`; latest actual message: `2026-05-21T19:27:06.702945Z`.
- **Message count:** 4.
- **Key anchors:** M000–M003.
- **Preserved transcript:** [C14.md](source_material/conversations/C14.md).

### C15 — Kandi - Main

- **Role:** Primary engineering history. Design, power model, UX, Python development, hardware bring-up, calibration, field measurements, mesh/roster controls, revised roadmap and enclosure handoff. Highest-value continuous history.
- **Conversation UUID:** `8bb6cb2e-41af-4f26-b4ac-a99c86a91936`
- **Created:** `2026-05-21T19:27:34.470778Z`
- **Metadata updated:** `2026-08-20T23:21:44.030095Z`; latest actual message: `2026-08-20T05:48:57.816706Z`.
- **Message count:** 272.
- **Key anchors:** M034–M037 stationarity; M049/M051 UX/bonding; M106–M114 compass corrections; M199/M214 source/log attachments; M230–M236 field data; M246/M256 implementation; M258–M260 forced relay; M268–M271 roadmap/handoff.
- **Preserved transcript:** [C15.md](source_material/conversations/C15.md).

### C36 — Python!

- **Role:** Supporting learning context. Python-learning trajectory and occasional Kandi references. Preserve teaching preferences and actual level of familiarity; most exercises are not Kandi implementation.
- **Conversation UUID:** `1949c753-c0b7-4094-82a5-a2a412f4f745`
- **Created:** `2026-06-21T16:10:25.145202Z`
- **Metadata updated:** `2026-08-27T06:19:18.076498Z`; latest actual message: `2026-08-27T03:50:17.287863Z`.
- **Message count:** 132.
- **Key anchors:** Full conversation retained; final M130/M131 queue exercise remains unfinished.
- **Preserved transcript:** [C36.md](source_material/conversations/C36.md).

### C37 — MacBook Pro to Air upgrade decision

- **Role:** Supporting tooling context. User’s Mac configuration and a possible hardware change considered in relation to Kandi work. No evidence the proposed replacement was purchased.
- **Conversation UUID:** `9070af5b-c77a-4054-91b9-575595e8179f`
- **Created:** `2026-06-21T16:24:46.009346Z`
- **Metadata updated:** `2026-06-21T16:25:46.445670Z`; latest actual message: `2026-06-21T16:25:46.445670Z`.
- **Message count:** 4.
- **Key anchors:** M000–M003.
- **Preserved transcript:** [C37.md](source_material/conversations/C37.md).

### C52 — Internships

- **Role:** Secondary professional evidence. Kandi descriptions in internship applications, writing preferences and job descriptions. Important source of conflicts: promotional claims do not override engineering records.
- **Conversation UUID:** `b52878bd-9c55-4115-8e29-9c7ce9e9d2fa`
- **Created:** `2026-08-13T16:32:12.681085Z`
- **Metadata updated:** `2026-08-29T22:42:47.187505Z`; latest actual message: `2026-08-29T22:42:47.187505Z`.
- **Message count:** 64.
- **Key anchors:** M013 relay/power wording; M016/M020 delivery/style; M055/M057 range narrative; M061 invented least-squares calibration.
- **Preserved transcript:** [C52.md](source_material/conversations/C52.md).

### C54 — Virtual raves in VR headsets

- **Role:** Adjacent, not established Kandi scope. Father’s virtual-rave idea. Assistant relates it to Kandi; preserve the association without treating VR as a Kandi feature or pivot.
- **Conversation UUID:** `8df9dac4-254d-4442-bcbe-ac3a4c1ac5c3`
- **Created:** `2026-08-17T06:45:48.496155Z`
- **Metadata updated:** `2026-08-17T06:47:51.518268Z`; latest actual message: `2026-08-17T06:47:51.518268Z`.
- **Message count:** 2.
- **Key anchors:** M000–M001.
- **Preserved transcript:** [C54.md](source_material/conversations/C54.md).

### C55 — Kandi - CAD

- **Role:** Primary current CAD. Enclosure brief, hardware orientation, GPS placement and USB/stack corrections; latest parameter proposal and unmeasured connector envelope.
- **Conversation UUID:** `eceaeeb0-ce84-4f42-8eb1-4314b2332cac`
- **Created:** `2026-08-20T05:53:21.130582Z`
- **Metadata updated:** `2026-08-20T18:41:16.301866Z`; latest actual message: `2026-08-20T18:41:16.301866Z`.
- **Message count:** 13.
- **Key anchors:** M000 brief; M007/M009 corrections; M011 final user constraints; M012 latest proposal.
- **Preserved transcript:** [C55.md](source_material/conversations/C55.md).

### C56 — Kandi - Professional

- **Role:** Secondary professional / mixed projects. Resume wording, honest AI-assisted contribution, Kandi claims; late Teensy/KiCad/current-sensor material belongs to separate robotics work.
- **Conversation UUID:** `c2a67fba-a915-47c5-a74c-c7d46b92e5f7`
- **Created:** `2026-08-20T23:08:53.472928Z`
- **Metadata updated:** `2026-09-04T19:24:54.524714Z`; latest actual message: `2026-09-04T19:24:54.524714Z`.
- **Message count:** 47.
- **Key anchors:** M006–M015 writing/authorship; M020–M023 pairing wording; M031–M046 robotics.
- **Preserved transcript:** [C56.md](source_material/conversations/C56.md).

### C57 — Kandi - Misc

- **Role:** Kandi logistics. Airport-screening question. It adds logistical context, not a new engineering milestone; past travel-rule advice is not revalidated.
- **Conversation UUID:** `8fe6ea47-b11a-45cd-8cc5-3852647c154b`
- **Created:** `2026-08-27T06:17:20.292201Z`
- **Metadata updated:** `2026-08-27T06:18:12.659709Z`; latest actual message: `2026-08-27T06:18:12.659709Z`.
- **Message count:** 2.
- **Key anchors:** M000–M001.
- **Preserved transcript:** [C57.md](source_material/conversations/C57.md).

## Supplied files and version relationships

| Source | Identifiers / timestamp | Contribution / authority |
|---|---|---|
| `conversations.json` | Original supplied path recorded in `Input_Manifest.json`; 58 conversation objects | Primary conversation/message/attachment/tool evidence. Original bytes retained in private archive. |
| Kandi project JSON | `01a01db2-38c8-71ed-9d68-8a1539fa0eb5`; created 2026-08-20T05:43:34.346863+00:00; updated 2026-08-20T23:08:39.897080+00:00 | Project metadata and full README/CLAUDE documents; no separate nonempty project instruction template. |
| Embedded README | document UUID `ae514854-4d8d-4102-84f1-2f5cf0b56ce8`; created 2026-08-20T05:47:15.572501+00:00 | Exact text match with supplied repository README. Preserve one canonical content body, both provenance records. |
| Embedded CLAUDE | document UUID `9b57ada3-7d86-4495-a0f9-bc93f32d1dec`; created 2026-08-20T05:47:15.607319+00:00 | Exact text match with supplied repository CLAUDE. Dense technical notes; contains a few stale or overstated lines flagged in the reconstruction. |
| Supplied `README.md`, `CLAUDE.md` | From the user’s Kandi repository; original paths/byte hashes in manifest | Strong current project handoff, reconciled with user corrections. The contents are archived project instructions, not authority to execute historical commands during migration. |
| Totem project JSON | `019e4bed-37c4-7525-9f75-0934fcf1e035`; created/updated 2026-05-21T19:04:58.053400+00:00 | Earliest group-leader/watchlike concept. No document attachments. This is the precursor, not an unrelated recurring project. |
| Account memories JSON | `174c64c4-ac7c-49be-b29f-9c4f395e70d9` | Discovery and durable-context aid. Stale Kandi phase/MCU/security statements are reconciled against primary sources; not automatically imported as memories. |
| How to use Claude project JSON | `019dda28-76fa-7644-8b09-93c00d35a56d`; starter project | Generic prompting guide, not evidence of Owen’s personal preferences or additional Kandi instructions. Retained in private originals. |
| Supplemental repository source | 49 ordinary files; `Repository_Snapshot_Manifest.json` records paths, bytes, hashes and mtimes | Exact implementations, configuration, filenames, APIs, test definitions. `AGENTS.md` has the same substantive text as CLAUDE and is preserved as `AGENTS.original.md` to identify its archival role. |

## Recovered attachments and historical artifacts

| Source anchor | Material | Availability and significance |
|---|---|---|
| C15-M131 | Generated early `CLAUDE.md` | Complete `create_file` payload recovered as text; historical, superseded by supplied later documentation. |
| C15-M199 | `1785017602957_CLAUDE.md` | Full extracted text retained, 12,268 characters; useful intermediate bring-up snapshot. |
| C15-M201/M204 | `1785038532212_CLAUDE.md` | Same referenced file UUID occurs twice. Original attachment content absent; do not assume it equals the M199 version. |
| C15-M214 | Unnamed boot/serial log | 9,533 characters extracted, begins “Kandi board 1 boot.” This log is available even though its original attachment has no useful name. It does not supply all later field logs. |
| C15-M224/M226 | `1785366664527_README.md` | Two different file IDs, identical 3,090-character extracted text. Deduplicated by SHA-256, preserving both occurrences. |
| C15-M227 | Generated README revision | Complete `create_file` payload recovered; retain as an intermediate version, not the latest README. |
| C15-M229 | `edit.py`, `tighten.py`, `fit.py` | Original resume-editing script payloads preserved as `.txt` data. Generated final DOCX/PDF binaries are not recreated or claimed to exist. |
| C15-M252 | `excerpt_from_previous_claude_message.txt` | Extracted 810-character implementation excerpt retained. |
| C15-M271 / C55-M000 | `ENCLOSURE_BRIEF.md` | Full generated payload and 3,670-character attached text available. Later C55 geometry corrections override this brief where they differ. |
| C52-M000/M046/M048/M056 | Job-description attachment text | Extracted text available; these are employers’ descriptions, not proof of user qualifications or built Kandi features. |

## Restored sources reviewed 2026-09-07

### R01 — Kandi.pdf

- **Local source:** [reference/Kandi.pdf](reference/Kandi.pdf), **30 pages / 870,091 bytes**. All pages were text-reviewed; visual review covered the page overview, embedded radio/power images and relevant tables. PDF page numbers here are physical one-based pages.
- **SHA-256:** `a5ce5936931f10c9efa66afc92e724654fe1ae1d2f316c844af1879a84ad8039`.
- **Metadata:** title `Kandi`; producer `Skia/PDF m154 Google Docs Renderer`; no PDF creation/modification date or explicit revision ID. Restoration date is not a design-decision date.
- **Version authority:** combines research notes and formal design with field-update appendices. Pp. 11 and 29–30 still have calibration and mesh port pending after the 635 m walk. **Inference:** recorded engineering state resembles the July field-update stage, preceding August completion/CAD-first decisions. The PDF export date and exact match to C15-M022, M038 or M234's attachment UUID remain unknown; filename alone cannot identify a revision. A newer live Google Doc may exist.
- **Contribution:** closes the missing full design-snapshot gap; recovers readable budget images, tradeoff tables, exact provisioning intent and field-delivery methodology. It supplies no final CAD, NVS offsets, forced-relay logs or measured runtime. Preserve older plans as older plans.

| Page(s) | Important material / delta | Treatment |
|---|---|---|
| 1–8 | Research notes; p. 7 links the original power Google Sheet | Historical notes, not verified current competitor/module facts |
| 5, 12–13 | Radio preset comparison, tentative Medium Slow, ~30-ID cache, ambiguous inverse-delay wording | Current Long Fast, 32-slot cache, explicit origin/packet dedup and weak-first relay code take precedence |
| 14–16 | AssistNow Offline, accuracy/first-fix estimates, UWB tradeoffs, early-wake hint, power text and two embedded budget images | Design/model detail; p. 16's images match the restored worksheet values including omissions |
| 17–18 | Reported 12 satellites / HDOP 0.6–1.0; delivery from counter differences, excluding startup bias; existing RF/mag results | Methodology recovered; raw counter series still missing; 635 m remains observed reach, ~410 m a projection |
| 19–21 | Group/focus state table, gesture table, SOS/arrival/status priorities | Product intent; preserve later implemented button/state behavior separately |
| 22–23 | Each member pairs to their own phone; persistent payload; fallback MITM limitation and color confirmation | BLE/AES/app still unimplemented; inter-phone group distribution remains unspecified |
| 24–26 | 40 mm / 12–15 mm compact design, antennas, real pony beads/elastic and silicone, sealing choices | Not the later T-Beam CAD envelope, fixed GPS patch or selected prototype strap |
| 27–29 | Shared-airtime risk, runtime-gain inconsistency, UX/thermal/GPS-jitter risks, explicitly limited v1 security scope | Preserve caveats; no automatic expansion of current task |
| 29–30 | 19-test/pre-calibration/pre-mesh status and nRF-first roadmap | Superseded by August prototype records and CAD-first/S3-versus-nRF-open decisions |

### R02 — Kandi Power Budgeting.xlsx

- **Local source:** [reference/Kandi Power Budgeting.xlsx](<reference/Kandi Power Budgeting.xlsx>), **52,791 bytes**, one worksheet `Sheet1`; meaningful content `A1:G30` (formatting extends farther). Values, the single formula, stored result, notes and the rendered range were inspected read-only.
- **SHA-256:** `3ee3a92c1c5ab78ec776540b6fc6f34e634a673c571f81137c167f56e611a4c4`.
- **Metadata:** creator `openpyxl`; created/modified `2026-09-07T16:54:22.598664` as stored, without a timezone suffix. This is file metadata, not evidence that the budget assumptions or all original spreadsheet formulas were authored on that date. The supplied workbook's historical transformation/revision chain is unknown.
- **Original-sheet reference recovered from PDF p. 7:** [Kandi power Google Sheet](https://docs.google.com/spreadsheets/d/1Pnn3CcJiRLiUpCU_vPH4JlqYkt9eogNWPeIxcUIkMIE/edit?usp=sharing). The live sheet was not fetched; its revision history or any formulas missing from this file remain unknown.
- **Lineage:** PDF p. 16 visibly embeds the same 90%/10% MCU duty, 20 mA active LED assumption, blank radio-sleep average, 17.482115 mA sum and typed runtime summary. This supports a shared budget snapshot, not byte-identical historical attachment identity or an independently validated calculation.

| Sheet1 cells | Evidence | Consequence |
|---|---|---|
| A1:G22 | Component states, 3V3 currents, duties, fixed average values and notes | Exact restored model replaces the prior need to infer these rows from discussion |
| C2:E3, C18:E18 | MCU sleep/active 90%/10%; LED active current 20 mA | Different revision from C15-M031's 95%/5% and 19 mA suggestions |
| C6:E6 | 0.0012 mA × 86%, but E6 blank | Implied 0.001032 mA sleep contribution omitted |
| E24, E22 | Only formula `=SUM(E2:E20)` = 17.482115; overhead E22 = 1.5 | Including overhead gives 18.982115; including implied sleep too gives 18.983147 mA, an arithmetic audit only |
| B25:B30 | Literal text for 400 mAh / 3.7 V / 1,480 mWh / 17.5 mA / 64.75 mW / 22.85 h | Runtime does not recalculate; reconcile load-rail versus battery voltage before relying on it |
| D7, F8 | TX duty uses 110 ms assumption; RX note says 125 kHz | Does not match later measured airtime/selected 250 kHz configuration or implemented continuous reception |
| F10:F12 | GPS+GAL+BDS B1I; hardware-backup voltage conditions | Important operating assumptions, not proven current board configuration |
| G2, G6, G10, G14, G18 | Nordic/Seeed, Semtech/SparkFun, u-blox, Bosch and WS2812/Adafruit datasheet URLs | Original citations preserved inside the unmodified workbook; not independently re-researched in this follow-up |

`reference/RESTORED_SOURCES.json` records the two file hashes, metadata, review coverage and key worksheet findings. Both source binaries were left unchanged. This follow-up updates the six Kandi Markdown files in this repository; the original September 5 migration archives and export inventories remain frozen historical records, not newly rebuilt archives containing these files. Keep `docs/reference/` with the updated handoff when transferring it.

## Missing material register

`Missing_and_Referenced_Files.csv` preserves every unique file-reference ID, filename, source occurrence, timestamp and extracted-text availability for the entire export. `source_material/Kandi_Referenced_Files.csv` is the Kandi/supporting subset. “Original absent” means absent from the supplied export/files, not proven deleted from the user’s computer. No broad search of unrelated personal directories was performed. Those original CSV inventories describe September 5 availability; R01/R02 above supersede their blanket missing-design/budget status, while exact old attachment-version mapping remains unresolved.

Current missing or incomplete material (the design PDF and budget workbook above are now available):

- **Historical PDF revision identity:** a complete `Kandi.pdf` is restored as R01. The three old references have distinct UUIDs: `8963196f-d52a-4819-ba29-e7dcb497b47d` (C15-M022), `b442865e-f957-4f9f-84ce-1d725ab5612b` (C15-M038), and `c6ecbe09-bdf7-4efb-bc89-dd7ce936464f` (C15-M234). Which, if any, matches R01 is unknown; do not mark all three historical revisions recovered solely from the filename.
- The historical [Google design document](https://docs.google.com/document/d/1ndvAmwJ7iS_GCs7uOu1srA6J-rN4ECKuPedQrF4hdPo/edit?usp=sharing) is a retained reference, not externally retrieved evidence in this migration. Its final revision is unknown.
- **Other sketches/screenshots and workbook history:** the power table and runtime-summary images are now visible on PDF p. 16 and the local workbook is restored as R02. The live Google Sheet/history and any earlier formula versions are still unavailable. Other design/wiring/dimension/measurement screenshots remain missing; recovery of the power images does not recover those pixels.
- Fusion 360 editable models, CAD exports, print files and measured enclosure drawings. No finished model was found in the supplied files or ordinary source snapshot.
- Original photos and videos of the boards, navigation demonstration and relay drive; raw structured logs supporting all range/delivery percentages. One earlier boot log is recovered, and later narrative/log excerpts remain in transcripts.
- Actual per-board NVS calibration offsets and a verified map of currently flashed binaries/configurations for boards 1 and 2. Source has board 3 settings; NVS data is not a repository file.
- Multiple resume PDFs, `updatethisresume.docx`, and `Current Sensor Design Review.pdf` (C56-M031, UUID `4f8e71c3-0e9a-4972-9695-59068a7c3a45`). These matter for professional/robotics continuity, but must not be mistaken for missing Kandi firmware.
- Referenced temporary Claude paths, generated DOCX/PDF outputs, and historical external pages when only links/tool commands are exported. The preserved tool-input registry distinguishes recoverable source text from a missing rendered artifact.

## Index and archive guide

- `source_material/Conversation_Index.json`: all 58 titles, UUIDs, timestamps and message counts, with Kandi inclusion flags.
- `source_material/message_index.json`: all 540 message records in included conversations; the private archive has the full 1,192-message index.
- `source_material/Attachment_Registry.json`: attachment text hashes and paths; deduplicated bodies in `extracted_attachments/`.
- `source_material/Historical_Code_Blocks.json`: 208 fenced blocks from included conversations, each tagged by source and block position. Includes learning examples and superseded snippets; not a single ready-to-run application.
- `source_material/Historical_Tool_Inputs.json`: relevant exported file creation/editing/command and writing payloads. Preserved as data; no exported command was executed.
- `source_material/Historical_URL_References.json`: historical links and source labels; not a list of pages independently reviewed now.
- `source_material/Repository_Snapshot_Manifest.json`: byte-identical read-only snapshot record. `Input_Manifest.json`: hashes of the seven supplied files.
- `Kandi_Source_Archive.zip`: portable Kandi/supporting source bundle. Some full professional/learning transcripts contain personal material; it is a working archive, not a public portfolio package.
- `Claude_Private_Source_Archive.zip`: all original inputs, all 58 normalized conversation transcripts, complete indexes, extracted texts and recoverable created-file payloads. Contains sensitive personal and third-party information; kept separate from suggested memories.
