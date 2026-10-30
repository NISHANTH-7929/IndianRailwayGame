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

- Visual review loop (renders real frames offscreen from isolated fixture saves):
  `07_Documentation/S9/Tools/Review-Visuals.ps1 -RunId <fresh-label> -Scenes urban,suburban -Cab`
  → PNGs in `07_Documentation/S9/Evidence/<label>/<scene>/`. Build the editor
  target first. A run takes a few minutes per scene.
- Editor-side asset authoring (materials/texture import) runs as a Python
  commandlet: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<abs .py> -unattended -nullrhi`.
  New runtime-loaded content directories must be added to `DirectoriesToAlwaysCook`
  in `Config/DefaultGame.ini`.

Rules: do not overwrite or delete user save checkpoints; use new labels for new
evidence; save-format or content-id changes need a new version, not a silent edit.

## Known realism gaps (as of 2026-10-30, from code review)

Highest-leverage areas for the "very realistic" goal — verify before relying on
this list, and update it as gaps close:

- **Signalling**: four-aspect automatic block signalling is implemented
  (`Simulation/RailAutomaticSignals`, see `07_Documentation/S10/SIGNALLING.md`): lineside
  aspects are *derived* from the operations snapshot (never saved). Still missing: passing an
  automatic signal at danger (GR 9.02), route indicators, calling-on/shunt signals, and the
  surveyed signal plan. The route-level `SignalAspect` (Stop/Proceed) remains the dispatcher's
  internal state used by AI.
- **Brakes**: WAP-7/LHB use the graduated twin-pipe air brake and regenerative braking
  (`AirBrake`, `RegenerativeEffortN` in `RailHandling.h`; `07_Documentation/S10/BRAKES.md`).
  Throttle in Tambaram content ranges −1..1 (negative = BE). Still missing: SA9 independent
  brake (needs a save-schema bump), per-vehicle pipe propagation, wheel slide, coupler forces.
- **Traction**: continuous throttle. The service WAP-7 (`RailTambaramDriving.h`)
  uses 323 kN / 4.5 MW, close to published figures (322.4 kN starting,
  228 kN continuous at 71 km/h, 6120 hp continuous); the generic test fixture in
  `RailHandling.cpp` (150 kN / 900 kW) is a placeholder. The master controller is
  centre-zero TE/BE; TE is continuous (no notch steps).
- **No in-train forces** (coupler slack, buff/draft), curve resistance, wheel
  slip/slide, or weather-dependent adhesion.
- **Cab procedures**: no pantograph raise / VCB close / vigilance (VCD) start-up
  sequence yet (Cab V2 has the VCD button and SA9 handle as static geometry).
- Cab BP and BC needles follow the brake model; MR/FP show a charged supply (no compressor cycling).
- Scenery: corridor buildings/trees are procedural primitives built in
  `RailTambaramCorridorView.cpp` (HISM families). Photoreal results ultimately need
  real modelled assets (Fab/Megascans, CC0 Poly Haven, commissioned WAP-7/LHB).
- Audio is generic, not WAP-7/LHB field recordings.

## Visual baseline (2026-10-30 overhaul)

- Lighting is runtime-spawned in `PrepareTambaramEnvironment` (`RailChunkView.cpp`):
  SkyAtmosphere + atmosphere sun (Chennai ~09:30), real-time captured sky light,
  humid haze fog, unbound post-process grade. No painted sky sphere.
- Inside the cab the sky-light is dimmed (`ApplyCabAmbient`, `RailLocomotiveView.cpp`)
  because mobile shading has no sky occlusion.
- WAP-7 cab is **Cab V2** (`07_Documentation/S10/Tools/build_wap7_cab_v2.py` in Blender →
  `import_wap7_cab_v2.py` in Unreal → `Production/locomotives/WAP7.json`). Its windscreen,
  side window, door and roof line are ray-cast from the exterior model; keep them in sync if
  the exterior changes. Moving-part axes: Blender→Unreal Y mirror means axis (x,y,z) → (-x,y,-z).
  Flat desk text uses `DESK_TEXT` rotation so it reads upright from the seat.
- Driving HUD (`RailDrivingHud.cpp`) is cab-style: A9 brake gate (REL/MIN/FS, BC kg/cm²),
  TE master-controller gate (notches, TE kN), DDU strip at the top, nothing over the centre
  of the windscreen. Gate travel must stay 177 design units (drag model + automation test).
- `Review-Visuals.ps1 -Cab -Hud` captures the cab and the HUD (HUD review shows the driving
  interface over the paused fixture via `-TambaramHudReview`).
- Facades use `M_ChennaiFacade` (S10/ChennaiFacadeV1): biplanar world-aligned
  weathering mask (rain streaks, faded paint, dust), 2 texture fetches.
- Corridor has a third housing row and a continuous far skyline band (houses,
  black rooftop tanks, coconut palms, canopy) so the horizon is never empty.

## Third-party assets

- **Poly Haven (CC0)**: `04_Assets/S10/PolyHaven/` (+ `provenance.json`), fetched by
  `07_Documentation/S10/Tools/fetch_polyhaven_v1.py`, materials authored by
  `import_polyhaven_materials_v1.py` into `/Game/IndianRailway/S10/PolyHavenV1`
  (`M_ChennaiGround`, `M_ChennaiConcrete`, `M_ChennaiAsphalt`; `M_ChennaiFacade` reuses the
  plaster map). 1K maps only, for the Android budget.
- **Fab / Megascans**: downloaded by the developer while signed in to Epic (Claude cannot
  sign in). They land under `Content/Fab/`; check each item's licence on Fab, record it in a
  provenance file next to where it is used, and import at Medium quality for Android.
- Every runtime-loaded content folder needs a `DirectoriesToAlwaysCook` entry.

## Working agreement

- Before a major system: understand → dependencies → propose architecture →
  trade-offs/risks → MVP vs later → then implement incrementally (brief §17).
- Ask the developer when a decision changes product scope, content identity,
  save compatibility, or the realism-vs-accessibility balance.
- Mobile-first: every feature must have an Android performance story.
- Respect IP rules (brief §15): no official IR/IRCTC logos or marks.
