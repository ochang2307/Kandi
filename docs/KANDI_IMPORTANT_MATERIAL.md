# Important Material Worth Preserving

Keep the context documents for orientation and the source collection for exact technical evidence. Do not replace the originals with summaries. Everything below is retained as archival material; historical prompts, commands, and project instructions are not new instructions to execute.

Updated 2026-09-07 for two restored sources, reviewed without modifying their binaries. The older migration source archive remains intact.

## Kandi material to retain substantially intact

| Material | Why it matters | Preserved location / version guidance |
|---|---|---|
| Supplied `README.md` and `CLAUDE.md` | Dense handoff: phases, modules, pin maps, bus/address traps, calibration, RF tests, constraints and rationale | `source_material/project_documents/` and byte-preserving `repository_snapshot/`. Project-embedded content matches supplied files exactly. Use reconstruction’s conflict notes for stale statements. |
| Current Python and C++ source | Exact algorithms, APIs, serial/wire formats, initialization, state machines, calibration and per-board build configuration | `source_material/repository_snapshot/CoreLogic/` and `Firmware/`; all 49 snapshot files hashed in the manifest. This is source, not a snapshot of every board’s flashed binary or NVS. |
| `platformio.ini` and workspace files | Toolchain/library declarations and folder organization needed to resume work | Repository snapshot. Historical library versions are not necessarily pinned by the file. |
| Complete “Kandi - Main” | All design turns, reversals, explanation and debugging context | `source_material/conversations/C15.md`; original JSON in private archive retains every original field and tool record. |
| Final CAD discussion | User corrections supersede earlier packaging assumptions; exact proposed parameters and unresolved geometry | `source_material/conversations/C55.md`, especially M007/M009/M011/M012. |
| `ENCLOSURE_BRIEF.md` | Reusable CAD handoff at the branch point | `source_material/recovered_created_files/C15-M271-B01-ENCLOSURE_BRIEF.md.txt`; also preserved as extracted attachment text via the registry. Apply later C55 corrections. |
| Compass correction sequence | Prevents regression to intermediate formulas or conflating sensor frames with LED indexing | C15-M106–M114, current `tilt_compensation.py`, `compass.cpp`, `imu.cpp`, `mag.cpp`, and calibration code. Synthetic 26-case result is separate from physical calibration. |
| Hardware bring-up notes | PMU-first order, GPIO7 GPS enable, dual SPI/I²C topology, 0x3C/0x3D ambiguity, slow OLED library, buffer offset and sensor library APIs | Supplied CLAUDE; earlier snapshot C15-M199; current drivers. Early generated CLAUDE retained as historical source. |
| Boot/serial diagnostics | Actual recorded golden-value output and debugging evidence | C15-M214 unnamed attachment; text hash `aa9496ee76aff31a6c7d17c2f11ee3a7b141691aec0ec2e23d8e3613d51108f7`. See `extracted_attachments/`. |
| Mesh identity and timing evidence | Originator/transmitter distinction, forced topology, seen-cache/queue semantics, delayed/cancelled relay and log interpretation | `mesh.h/.cpp`, `radio.cpp`, `roster.cpp`, `mesh_test.cpp`; C15-M248–M256 and field M258–M260. |
| Field measurement discussion | Separates observed 635 m/11.2 dB from inferred n≈3.7/~410 m and forced 628 m relay claim | C15-M230–M236 and M258–M260; supplied README measurement table. Raw later photos/video/full logs remain missing. |
| Restored full design PDF | Original research/design/field-update context, embedded preset/power images and exact provisioning/packaging intent | [reference/Kandi.pdf](reference/Kandi.pdf), 30 pages. Retain all pages including internal inconsistencies; August engineering decisions supersede its old status/roadmap. R01 in source map identifies pages and hash. |
| Restored power workbook and stationarity history | Exact operating assumptions and a traceable, incomplete sum underlying the 17.5 mA headline | [reference/Kandi Power Budgeting.xlsx](<reference/Kandi Power Budgeting.xlsx>), Sheet1!A1:G30; C15-M028–M039 remains the earlier discussion. Keep the binary unchanged, including E24’s omission of E22 overhead, blank E6 and unlinked runtime text. PDF p. 16 preserves matching images. R02 records cell-level evidence and hash. |
| UX and bonding drafts | Rich planned group/focus/color/SOS interactions and onboarding concept that were not all implemented | C15-M040–M051; distinguish proposed product gestures from current 600 ms button split. |
| Phase reversal and work preference | Prevents reverting to immediate nRF migration/power-first or insisting on an immediate field rerun | C15-M260 and M262–M271; supplied CLAUDE Current phase. |
| Professional drafts and corrections | Useful writing history but also a record of unsupported calibration/security/range claims that must not recur | `source_material/conversations/C52.md` and `C56.md`. Late robotics material remains separately attributed. |
| Earlier project/README versions | Records how “completed” changed over time | `project_exports/`, `extracted_attachments/`, `recovered_created_files/`; use hashes/anchors rather than filename alone to identify a revision. |
| Historical code and tool payloads | Exact snippets, prompts, generated file text and editing source can contain detail absent from the prose summary | `Historical_Code_Blocks.json` (208 blocks from included conversations) and `Historical_Tool_Inputs.json`. These include examples/obsolete code, not only production Kandi code. |

## Recoverable text versus missing original artifacts

The archive recovers six `create_file` payloads from the Kandi core conversation: early CLAUDE, a README revision, three resume-editing scripts, and the enclosure brief. Across the full export, 31 created-file payloads are retained. Scripts are stored with `.txt` suffixes and were not executed. A script that once generated a PDF or DOCX is not the rendered file itself.

Eleven extracted attachment occurrences have ten unique text bodies. The duplicate README text in C15-M224 and M226 is stored once by hash while both occurrence records survive. The entire original export remains unchanged in the private archive, so deduplication does not destroy provenance.

The full design snapshot and budget workbook are now restored locally, along with the budget/preset images embedded in the PDF. Still missing are Fusion/editable CAD and print files, other board/wiring/dimension photographs, captured demonstration videos, full field logs and per-device calibration/configuration records. Historical PDF attachment-to-revision mapping and the live Google Doc/Sheet revision history remain unknown. The source map’s R01/R02 records update availability without claiming every older attachment version was recovered. No missing content was invented.

## Useful secondary material retained in the private archive

- Complete C programming/course project material, particularly the C library-management program and related pointer/struct/file-I/O exercises (C01/C02/C10). Preserve code intact if those projects are resumed; do not convert each exercise into memory.
- Altium/drone report and robotics/current-sensor discussions (C03/C04 and later C56), with their missing attachments identified. These are distinct from Kandi hardware.
- The long Python-learning conversation (C36), including exercise state and feedback that helps resume at the user’s actual level.
- Internship drafts and role descriptions (C51/C52/C56), with application/submission status left unknown unless explicitly recorded. Employer requirements are not user qualifications.
- The decentralized idle-device AI concept (C27): an early project idea, not a finished system.
- Housing/lease/document records and financial discussions, held in the **private archive** rather than proposed general memories. Keep original contractual/financial material separate from old assistant interpretations; no fresh legal or financial conclusions were drawn.

## Portable files

The six primary Kandi deliverables are `KANDI_PROJECT_CONTEXT.md`, `Kandi_Current_State.md` (existing filename retained), `KANDI_TIMELINE.md`, `KANDI_OPEN_THREADS.md`, `KANDI_SOURCE_MAP.md`, and this file. `Claude_to_GPT_Migration.md` combines the full eight requested sections, including general context and suggested memories.

`Kandi_Source_Archive.zip` contains the supporting source collection. `Claude_Private_Source_Archive.zip` retains all original inputs and all 58 normalized conversations. These are private working archives, particularly the latter; neither is automatically saved as GPT memory or published. The main documents and source indexes remain ordinary files for future sessions to reuse.

## Preserve the restored sources with the current handoff

Keep both binaries in `docs/reference/`, together with `RESTORED_SOURCES.json`. The September 5 ZIP archives/CSV inventories predate this restoration and were not overwritten or rebuilt. A future transfer must include these new local sources separately as well as the updated Markdown. The budget's 18.983147 mA value in the review is only the sum after filling two omissions under unchanged assumptions; it is not a corrected workbook, a measured prototype value, or a validated runtime claim.
