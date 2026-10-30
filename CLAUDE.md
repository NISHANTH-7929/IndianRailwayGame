# CLAUDE.md — Indian Railway Game

Guidance for Claude (and other AI assistants) working in this repository.

## North star: realism

The #1 product goal is **realism** — the "Monoposto of Indian train sims". The
market already has many casual Indian train games; this project only wins by
feeling like a real Indian Railways loco-pilot experience. When choosing between
"easier/flashier" and "more authentic", prefer authentic, and say so when a
shortcut would cost realism.

Realism means *consistent, believable behaviour of the whole railway*, not just
graphics (see `02_TechnicalArchitecture/PROJECT_MASTER_BRIEF.md` §5, §10, §19):
train physics, brakes, cab controls, signalling rules, AI traffic, stations,
sound and environment must all agree with each other.

Rules that follow from this:
- Base numbers on real data (loco specs, IR General & Subsidiary Rules, signal
  aspects, speed restrictions, brake behaviour). Cite the source in code comments
  or docs. If a value is a guess, label it *provisional* — the codebase already
  does this ("Provisional engineering data", "not measured") — keep that honesty.
- Never fake realism in presentation: no decorative trains, invented signal
  aspects or HUD cues that contradict the simulation. Presentation reads from the
  simulation; it never drives it.
- Never relax a test threshold to make something pass. Failed evidence is kept
  with an explanation (project convention).

## Project facts

- Engine: Unreal Engine **5.8** (C++ module `MyProject`). Target platform:
  **Android** first; desktop (Win64) builds currently used for review.
  Android is *not* currently qualified for the S9 content.
- Solo developer + AI assistance. Human developer makes final decisions.
- Current content scope: **WAP-7 locomotive + 7 LHB coaches**, representative
  **Chennai Egmore → Tambaram** suburban corridor (≈ train 12631 service).
  WAG-9 / BLC freight assets exist but are deferred.
- Two driving modes: **Standard** (manual power/brake) and **Assisted**.
- Milestones are named S0…S9; S9 is current. Latest state lives in
  `07_Documentation/S9/CURRENT_CONTINUATION.md`, `S9_KNOWN_LIMITATIONS.md`,
  `GAMEPLAY_GAPS.md` — read these before starting new work.

## Repository layout

| Path | Contents |
|---|---|
| `01_GameDesign/` | Game vision |
| `02_TechnicalArchitecture/` | Master brief, architecture blueprint, roadmap |
| `03_UnrealProject/MyProject/` | The Unreal project (`MyProject.uproject`, `Source/`, `Config/`, `Content/IndianRailway/S2..S9`) |
| `04_Assets/`, `05_Audio/`, `assets/` | Source art/audio + Python/Blender build scripts and provenance |
| `06_Routes/` | Route references (Egmore–Tambaram OSM etc.) |
| `07_Documentation/S*/` | Per-milestone reports, decisions, `Tools/` (build/test/capture scripts). `Evidence/` and `Baseline/` are git-ignored |
| `08_Builds/` | Packaged builds (git-ignored) |

Binary assets go through **Git LFS**.

## Code architecture (`Source/MyProject/`)

Two layers — keep them separate:

1. **`Simulation/` — pure C++17, no Unreal includes** (namespace `Rail`).
   Deterministic, fixed-step (1/30 s), testable outside the editor.
   - `RailKernel` — track graph (nodes/edges with analytic geometry: yaw,
     curvature, grade), path history, formation placement, fixed clock.
   - `RailHandling` — longitudinal train dynamics: traction (force/power/adhesion
     limited), rolling + aero resistance, grade, service/emergency brake with
     first-order apply/release lag. `HandlingDefinition` holds per-vehicle params.
   - `RailOperations*` — signals, routes, authorities, dispatch, point locking.
   - `RailAIDriver` — AI train driving to authority/signals.
   - `RailService` / `RailTambaram*` / `RailScenarioContent` — the service
     (timetable, stations, dwell, scoring) and route content.
   - `RailWorldSnapshot`, `RailCheckpointStore`, `RailSnapshotRecord` — save/load
     with content + schema identity (saves are versioned; no silent migration).
   - `*Tests.cpp` — host tests (`RunKernelTests`, `RunHandlingTests`, …).
2. **Unreal layer** (`Rail*View`, `Rail*Widget`, `RailDrivingHud`,
   `RailSimulationSubsystem`, `RailServiceAudio`, …) — presentation, input,
   audio, UMG. Reads simulation state; submits commands. Unreal automation
   tests live in `Rail*Tests.cpp` at this level (`Automation RunTests IndianRailway`).

Code style: compact, dense C++ (one-line structs, short names, `Result` return
values instead of exceptions). Match the surrounding style when editing.

## Building and testing

Scripts live under `07_Documentation/S8/Tools/` and `07_Documentation/S9/Tools/`
(several contain paths specific to the original workstation `D:/Project/...`).

- Host simulation regression: `07_Documentation/S9/Tools/Test-Regression.cmd`,
  `Test-Journey.cmd`, `Test-Dispatch.cmd`.
- Editor build: engine `Build.bat MyProjectEditor Win64 Development -Project=<abs path to MyProject.uproject>`.
- Unreal automation: `07_Documentation/S8/Tools/Test-Automation.ps1`.
- Desktop package: `07_Documentation/S9/Tools/Package-Desktop.ps1`;
  Android: `Package-AndroidJourney.ps1`.
- See `07_Documentation/S8/REPRODUCTION.md` for the full pipeline.

Rules: do not overwrite or delete user save checkpoints; use new labels for new
evidence; save-format or content-id changes need a new version, not a silent edit.

## Known realism gaps (as of 2026-10-30, from code review)

Highest-leverage areas for the "very realistic" goal — verify before relying on
this list, and update it as gaps close:

- **Signalling**: `SignalAspect` is only `Stop`/`Proceed`. Indian Railways uses
  multi-aspect colour-light signalling (Red / Yellow / Double Yellow / Green),
  route indicators, calling-on, permissive/automatic signals, distant/home/
  starter roles, and the absolute/automatic block systems.
- **Brakes**: modelled as one lumped first-order lag. Missing: brake-pipe
  pressure propagation along the rake, separate automatic (train) and
  independent (loco) brakes, graduated release, and WAP-7 regenerative braking.
- **Traction**: continuous throttle; WAP-7 has a notch/tractive-effort controller
  and published TE/speed curves (start TE ~322 kN, 6120 hp) — current fixture
  values (150 kN / 900 kW) are placeholders.
- **No in-train forces** (coupler slack, buff/draft), curve resistance, wheel
  slip/slide, or weather-dependent adhesion.
- **Cab procedures**: no pantograph raise / VCB close / reverser / vigilance
  (VCD) / speedometer-recorder style start-up sequence yet.
- Scenery is sparse/repetitive; audio is generic, not WAP-7/LHB field recordings.

## Working agreement

- Before a major system: understand → dependencies → propose architecture →
  trade-offs/risks → MVP vs later → then implement incrementally (brief §17).
- Ask the developer when a decision changes product scope, content identity,
  save compatibility, or the realism-vs-accessibility balance.
- Mobile-first: every feature must have an Android performance story.
- Respect IP rules (brief §15): no official IR/IRCTC logos or marks.
