# S9 dispatch diagnostics and four-train logical stress — 15 October 2026

Status: bounded desktop logical hardening, not a new shipped scenario or full MVP qualification.

## Implementation

`Source/MyProject/Simulation/RailDispatchDiagnostics.h` adds read-only `InspectDispatch(definitions, snapshot)` and `DispatchWatchdog::Observe(world)`. Denied route requests identify blocking trains and required blocks, fouling blocks, points and point protection. Cycle detection follows the dependency graph for up to 64 waiting trains. A cycle is a warning, not proof that a moving train cannot clear. A prolonged hold means 900 simulation ticks without at least 0.5 metres of movement by the waiter or its blockers, or a changed dependency signature. Pause does not age it. Backward simulation clocks and explicit Reset clear history. No diagnostic releases a resource, bypasses a command, changes an authoritative field or enters checkpoint serialization. Automatic deadlock resolution and general starvation guarantees are not implemented.

The main integration owns HUD/subsystem wiring and Unreal evidence. `RailOperations.h` adds only the test function friend hook. Existing content `IR-S8-MSTBM-6`, service `IR-S8-MSTBM-service-7`, three-train playable roster and ServiceV6 records remain unchanged by this work.

## Stress envelope and denominator

`RailDispatchTests.cpp` constructs four real formations on one 3,000 metre authoritative graph edge: player T1 and AI T2/T3/T4. It initializes shared stock, handling, occupancy, route reservations and signal derivation; it then advances only through normal owner updates and command/request queues. Origins and destinations are assigned holding blocks with explicit reserved transit. Overlapping route resources cause actual following holds; the leading AI clears first and later AI resume as complete formations clear reserved track. This is a conservative serial following/interlocking fixture, not a rendered junction, real timetable, freight roster or new route.

Declared denominator: six deterministic parameter combinations. Seeds 0–5 select departure delays of 1–6 seconds and alternate player target pacing at 3.0/2.5 m/s. Each includes two pauses, an applied player emergency with unaffected AI emergency state, denied/retried route requests, and three AI reaching their destination holding areas. Each run is bounded at 2,000 simulation seconds. No speed multiplier or injected traffic event is used. Actual denial observations are counted from lastRequest/blocker state; counts are observations of persistent events, not counts of unique dispatch denials.

Result: **6/6** cases expose real blocking interactions and all three AI eventually stop with route release after whole-formation clearance. Terminal AI head positions are approximately 2049.887, 2249.828 and 2449.811–2449.911 metres. Player remains in progress at cutoff (1478.667–1769.548 metres); player service completion is not claimed. Existing invariants run each fixed step. This 33-minute logical stress envelope does **not** establish the proposed 90% product coverage for 5–10-minute missions, full eight-mission coverage, rendered four-train performance, save/recovery of this synthetic roster or mobile capacity.

## Evidence

- `Evidence/Dispatch-20261015-01/compile.log`: retained failed first link; shared TestReport implementation was missing from the runner's compile inputs. Runner corrected to compile RailKernelTests.cpp.
- `Evidence/Dispatch-20261015-02/results.json`: retained failed completion deadline, with safety/pause/emergency checks passing. Initial 1,066-second window was insufficient for about four kilometres of serial AI movement at 3 m/s plus acceleration/clearance. Corrected fixture duration to 2,000 seconds; no handling or safety threshold changed.
- `Evidence/Dispatch-20261015-03/results.json`: 21/21 PASS after duration correction.
- `Evidence/Dispatch-20261015-04/results.json`: **23/23 PASS**, adding moving-blocker watchdog reset, deterministic continuation from identical logical state and checking emergency isolation after actual update processing. Includes real denial identity, mutation-free inspection, legitimate wait versus deliberately synthetic dependency-cycle differentiation, prolonged hold without release, pause clock, and recovery-clock reset.
- Subsequent source-only rename `check` → `RecordCheck` avoids Unreal's `check` macro. Main owns integrated editor build/automation evidence for that final spelling.

Reproduce from project root (requires existing local VS2022 Build Tools):

```powershell
cmd /c 07_Documentation\S9\Tools\Test-Dispatch.cmd D:\Project\IndianRailwayGame\07_Documentation\S9\Evidence\Dispatch-NEW-UNIQUE
```

The runner rejects an existing evidence directory and does not overwrite S8 evidence. Compiler uses pure C++17 `/O2`; no Unreal session, Android build, normal save write or external service is used by this runner.

## Remaining boundaries

The standalone cycle input is deliberately diagnostic-only and is never advanced as a legal railway world or counted as an actual traffic interaction. Prolonged wait and cycle warnings require permitted dispatcher/user action; they are not automatic recovery. Snapshot serialization/cold recovery of the four-train fixture is NOT RUN and not supported by the unchanged playable content ID. New production rosters require a separately versioned data-driven configuration and UI/audio/presentation/save integration. Existing hardcoded opposing driver identity/speed profile remains inventoried rather than silently changing compatible content.
