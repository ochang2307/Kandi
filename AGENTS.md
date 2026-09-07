# Kandi — Codex Repository Instructions

## Project

Kandi is a wrist-worn festival wearable that guides group members to each
other without cell service, Wi-Fi, or internet. Devices exchange GPS
positions over a 915 MHz LoRa mesh and indicate direction with an LED ring.

This is both a real engineering/product project and a learning project.
Explain important engineering decisions and reasoning rather than only
producing code.

## Required context

Before substantial Kandi work, inspect the relevant repository state.

Read these documents as needed:

1. `Kandi_Current_State.md`
   - Primary briefing for the CURRENT project state and immediate work.

2. `CLAUDE.md`
   - Detailed hard-won engineering notes from development.
   - Contains firmware bring-up details, hardware behavior, calibration,
     field-test findings, debugging history, and implementation constraints.
   - Despite the filename, this is shared Kandi engineering documentation
     and is useful to Codex.

3. `KANDI_PROJECT_CONTEXT.md`
   - Comprehensive historical/project context reconstructed from the
     previous Claude project.

4. `KANDI_OPEN_THREADS.md`
   - Known unfinished work, open decisions, bugs, and next steps.

5. `KANDI_TIMELINE.md`
   - Use when deciding whether an older idea or decision was superseded.

6. `KANDI_SOURCE_MAP.md`
   - Use when provenance or historical source material matters.

7. `README.md`
   - Repository overview and links to additional design material.

Do not read every historical document for every small task. Load the
documents relevant to the work being performed.

## Source-of-truth / conflict rules

Do not silently reconcile conflicting information.

For implementation reality:
- Current repository code and passing tests are the strongest evidence of
  what is actually implemented.
- Explicitly measured/tested results outrank plans, projections, résumé
  wording, and portfolio descriptions.

For current project direction:
- `Kandi_Current_State.md` and newer explicit project decisions override
  older plans.
- The engineering design document remains authoritative for design decisions
  that have NOT subsequently been explicitly revised.
- Historical context documents explain past decisions but do not
  automatically override current repository state.

When two sources disagree and the newest authoritative answer is unclear,
tell Owen rather than guessing.

## Current phase

Phase 2, the three-device T-Beam Supreme prototype, is complete.

Current Phase 3 order:

1. Enclosure CAD for the existing dev-board hardware.
2. Power validation and measured before/after duty-cycling results.
3. Simple 2-layer LED-ring PCB.
4. Full custom RF board later.

Do NOT prematurely scope-creep the enclosure into the final miniaturized
product form factor.

The ESP32-S3 vs nRF52840 choice for the first miniaturized custom design is
still OPEN and should be informed by measured power results.

## Architecture rules

Preserve the separation between platform-independent core logic and hardware
I/O/drivers.

Do not rewrite or "improve" verified CoreLogic algorithms unless specifically
asked. The Python core logic and corresponding C++ ports were extensively
debugged and tested.

When modifying a ported algorithm, verify behavior against its existing
tests/golden values.

Do not "clean up" unusual formulas merely because a textbook implementation
looks different. Several apparently unusual implementations are deliberate
and documented in `CLAUDE.md`.

## Evidence/status discipline

Always distinguish:

- implemented
- field-verified / measured
- bench-verified
- modeled / projected
- planned
- deferred
- unresolved

Never describe a planned feature as implemented.

Do not use résumé/application/portfolio wording as evidence that something
exists if engineering records or repository state disagree.

Never invent missing measurements, calibration values, files, or test
results.

## Critical hardware safety

NEVER transmit with the LoRa antenna disconnected.

Any firmware capable of transmitting at boot must only be flashed/run with
the antenna attached.

Respect the documented LED power limits and other hardware constraints in
`CLAUDE.md`.

## Development workflow

Prefer incremental changes with a runnable test after each meaningful step.

When something fails:
1. inspect the failure pattern,
2. form a hypothesis,
3. test it,
4. only then change code.

Do not shotgun-edit multiple subsystems simultaneously unless asked.

For core logic/math/protocol changes:
- explain the proposed change and reasoning,
- let Owen participate in the implementation when practical.

For boilerplate, scaffolding, repetitive code, and hardware-driver plumbing:
- implementing directly is fine.

## Testing

Run relevant existing tests after changes.

For `CoreLogic/`, use the existing Python test scripts.

For firmware, inspect the current PlatformIO configuration before building.
The known PlatformIO executable is:

`~/.platformio/penv/bin/pio`

Do not declare work complete if relevant tests/builds fail unless the failure
is clearly explained.

## Documentation

After a material engineering change, check whether these need updating:

- `Kandi_Current_State.md`
- `CLAUDE.md`
- `README.md`
- relevant Kandi context/open-thread documentation

Do not allow the migration documents to become a second stale version of the
project.


## Communication style

Owen is learning through the project. Explain important code and engineering
reasoning.

Keep routine responses and commit messages concise.

Avoid overly polished/template-like prose.

Ask before making a major architectural change or changing a previously
verified core behavior.

### Historical/reference engineering sources

- `docs/reference/Kandi.pdf`
  - Original research and engineering design document created before
    implementation began.
  - Important for original rationale, requirements, architecture, and intended
    product behavior.
  - It is NOT automatically the current state: later explicit decisions,
    implemented repository behavior, and Kandi_Current_State.md override it
    where they conflict.

- `docs/reference/Kandi Power Budgeting.xlsx`
  - Original power-budget model.
  - Treat values as modeled/projected unless later measurements explicitly
    validate them.
  - Do not present these figures as measured ESP32-S3 prototype consumption.