# S5 implementation report

Status: **PASS — 22/22 acceptance gates.** Finalized 25 September 2026. S5 is implemented, documented and verified on the iQOO. No S6/S7 systems were implemented.

## 1. S5 objective

Introduce Player A and autonomous Train B on the same protected railway. AI decides intent; S4 decides permission; S3 integrates handling and S2 advances the complete formation. This is a bounded engineering fixture for the living railway concept, not final locomotive fidelity or production signalling.

## 2. S4 baseline

Accepted S4: 27/27 gates. Regression inventories: S2 39, S3 28, S4 39. The graph, eight-member A (181 m), two-member B (43 m), blocks, routes, points, signals, holding areas and handling parameters are reused. Pre-edit Source/Config copies and hashes are in Evidence/Baseline. Existing stage evidence remains historical.

## 3. Architecture changes

Operations value types moved without behaviour changes to RailOperationsTypes.h. RailAIDriver contains the pure driver. RailOperationsAI is the owner-side observation/dispatch adapter. The optional binding/driver collection and bounded trace reside inside OperationsWorld::State, so AI changes share the existing candidate-step transaction. URailSimulationSubsystem remains the sole mutable Unreal owner. Two presentation Actors represent all train geometry and infrastructure; neither owns authority.

## 4. AI architecture

A driver is bound to one stable TrainId, RouteId and generation. Binding requires a stopped forward train without an active route or pending player route requests. Public player commands/requests cannot override an AI-owned train. Internal driver and S4 protection commands use the same HandlingModel::Submit boundary; route intent uses the same S4 request queue and validation. No AIController, NavMesh, runtime model service or extra physics is introduced.

## 5. AI state machine

Holding -> RequestingRoute -> WaitingForRoute on denial. A matching grant permits RouteGranted -> Driving. Governing Stop yields RestrictiveApproach. The conservative endpoint envelope latches ApproachingAuthorityLimit -> Stopped. A real S4 release yields Completed and safe holding. Missing, stale or contradictory observation latches FailSafe. No autonomous reversal is enabled for these forward-only fixture intents; S3 player reversing remains unchanged.

## 6. Observation model

AIObservation is a value copy of train identity, tick, binding, head station, speed, length, authority, route owner/state, governing signal, last request result, and verified resource ownership. The adapter reads const spatial and operations state, verifies route block/point owners and checks signal relevance against committed path history. The driver has no mutable world, spatial object, Actor, transform or signal reference. Debug display also reads authoritative occupancy and point state directly.

## 7. Command generation

Target speed is 3 m/s. Below target minus 0.2 the driver requests throttle 1 and brake 0; in the target band it coasts; above target plus 0.2 it requests throttle 0 and brake 0.25. Waiting, restrictive and stopped states request throttle 0 and brake 1. S3 applies actuator response, force integration and zero-crossing behaviour. The adapter accepts exactly one throttle and one brake value, both finite and within [0,1], and stamps the existing train command sequence/apply tick. It never writes speed or position.

## 8. Route-request interaction

B requests R2 through S4 and retries every 90 simulation ticks (three simulated seconds). Commands apply at current tick + 1. Because AI executes after due requests are processed, newly emitted route requests explicitly apply at current tick + 2. Request results are authoritative, and denial does not become movement permission. Binding, train, route, observed tick and monotonic decision sequence are checked before dispatch. Public rebinding and queued player intent cannot supersede the AI binding.

## 9. Authority/signal interaction

Before passing the route origin, Proceed and matching valid authority/resources are required. After passing it, the entry signal is behind the train and S4's derived Stop does not revoke the existing corridor. The driver continues only under matching authority and locks. It starts braking when remaining distance is at most v²/(2*0.45) + 2.5*abs(v) + 3 metres, then holds the braking decision. S4's independent braking and swept-authority guards remain unchanged and apply after AI intent. Corrupt infrastructure rejects the entire step and retains prior physical/AI state.

## 10. Player + AI scenario

Reset queues a normal Player A R1 request and binds B to R2. A's request wins through ordinary S4 validation. B is denied and waits at head 100 m. Player A uses touch throttle/brake controls to traverse the upper route. A's first conservative stop can occur before rear clearance; releasing the brake and applying power moves the rear beyond all protected resources. S4 releases R1. A later B retry obtains R2 and B drives without a player driving command. B stops inside its endpoint and retains its still-occupied route. The terminal is too short for B's rear to clear; no fake release or topology extension is added.

## 11. Deterministic design

Movement remains 30 Hz with a four-step maximum and retained-debt fault policy. Decisions run every third logical tick (10 Hz), not per render frame. Ordered maps and monotonic sequences define processing order. AI state rolls back with the world. A bounded 32-decision trace plus a rolling FNV-1a trace fingerprint supports full-sequence comparisons; SHA256 evidence hashes are separate integrity checks. Wall-clock timing metrics are diagnostic only and excluded from authoritative JSON and decisions. Pausing freezes the logical cadence.

## 12. Safety invariants

S4 continues to validate complete formation occupancy, swept movement, corridor bounds, route/block/point owner tokens, point positions and rear-clearance release. AI cannot grant authority, change points directly, clear occupancy or release routes. Missing drivers produce safe control commands. Invalid observations or reverse movement with a forward intent latch braking. Stale decisions reject before queue changes. Current AI state, command queues and railway state commit only after all invariants pass.

## 13. Test inventory/results

Host suite: 36/36 passed on the final scheduling correction; machine-readable results are Evidence/ai-tests.json. S2/S3/S4 full inventories are saved separately as s2-regression.json, s3-regression.json and s4-regression.json. T01-T21 and T23-T27 are mapped to pure tests. Additional checks cover fixed retry tick, uncleared B route retention, logical copies, corrupt observations, player ownership, upper-route AI completion, duplicate binding, pending player intent, rollback and pause. T22 requires actual Unreal Actor destruction/recreation evidence; a pure copy test is not claimed as that result. T22 and T28-T30 now pass against integration-tests.json and the actual Android evidence.

| Verification | Result | Evidence |
|---|---:|---|
| S2 pure regression |39/39|Evidence/s2-regression.json|
| S3 pure regression |28/28|Evidence/s3-regression.json|
| S4 pure regression |39/39|Evidence/s4-regression.json|
| S5 pure suite |36/36|Evidence/ai-tests.json|
| Unreal Automation suites |4/4|Evidence/UnrealAutomation/index.json|
| Android startup inventories |39/28/39/36, all pass|Evidence/cross-platform-tests.json|
| Android interaction/lifecycle checks |17/17|Evidence/Device/ai-controls/results.json|
| Actual Actor recreation / integration mapping |Pass|Evidence/integration-tests.json|
## 14. Unreal Automation results

Final source validated: 4/4 Unreal Automation suites succeeded, zero failed, total test duration 2.9483 seconds. Kernel, Handling, Operations and AI all passed. Evidence: UnrealAutomation/index.json and automation.log. Final incremental editor build succeeded in 19.99 seconds.

## 15. Android verification

Installed the SHA256-verified final APK on iQOO Neo 9 Pro I2304, Android 16/API36, ARM64/Vulkan/ASTC through wireless ADB. Hotspot/network settings were unchanged. Initial cold launch completed in 1063 ms; the successful clean interaction rerun cold launch took 1005 ms. Startup inventories match the host and pass S2 39/39, S3 28/28, S4 39/39 and S5 36/36.

Clean touch/lifecycle scenario: 17/17 checks pass. B remained at 100 m under denial, rejected manual driving commands, and waited while A's front had passed but rear still protected the route. After A moved from 668.104170 m to 700.376230 m and S4 released R1, B obtained R2 and drove autonomously to 152.829249 m, stopping before its 165 m endpoint with its occupied route still retained. Current B position is E2, before the merge; this bounded endpoint does not permit full rear clearance.

Actual train/infrastructure Actor recreation passed before and after movement, including full logical JSON equality. Clean initial train-view IDs 33155 -> 33161. Logical pause froze tick 6906. Backgrounding froze tick 7009, and the same processes 19113/19173 resumed to 7099. Back exited successfully. Screenshots and raw snapshots are under Evidence/Device/ai-controls; integration-tests.json maps T22/T28/T29/T30 to actual evidence. The earlier unexpected pause run is retained and excluded; its cause was not established, and the user confirmed no auto-clicker was active.

Final APK: 08_Builds/S5_Android/Android_ASTC/MyProject-arm64.apk, 163956574 bytes. SHA256: C9E6B2FE1656821F35168FFB2A98FB8DC00345B2AEA3E36B8827E54DEA49AE78. Build ID 20260924-132031, 250.78 seconds, exit 0.
## 16. Performance results

Same final APK; CSV Profile(20260924_184354).csv contains 6000 frames over 204.814607 seconds. The active 10–180 s window contains 5093 frames: 29.9615 average FPS, frame p50 33.360 ms / p95 34.575 ms / p99 35.249 ms / maximum 37.972 ms; zero frames above 50 ms or 100 ms. All active snapshots were valid and fault-free. Host 190.289 s pause is deliberately outside that measurement window and prevents CSV-export work from entering the simulation clock. The external profiling commandline was restored to absent before the normal run.

| Counter | Mean ms per frame | p95 ms | Maximum ms |
|---|---:|---:|---:|
| Frame interval |33.3762|34.5750|37.9720|
| Game thread |8.1001|9.8480|37.7490|
| Render thread |6.1341|7.3860|12.0510|
| RHI thread |3.5884|4.5700|6.9660|
| GPU |2.0894|3.2810|8.2030|
| AI observation/decision/dispatch |0.01590|0.05500|0.22040|
| Simulation Step |0.28166|0.40140|0.74470|

AI runs at 10 Hz; its frame-aggregated timer includes zero on frames without a decision. Simulation Step includes validation, AI, handling and occupancy, but excludes the enclosing candidate-state copy. AI time is included in Step time; CPU/GPU lanes overlap, so these columns must not be added. Timing measurements never affect authoritative decisions.

Profile PSS: 458813–465973 KiB; sampled Android thermal status 0 throughout. These results apply only to this two-train fixture and device/build; larger train counts and final-game capacity are not established. The separate profiling-off 215 s sustained run passes: all seven post-start samples valid, no suspension or fault, ticks 750 to 6151, zero overload/fatal/assertion log matches. Normal-run PSS is 461159–467222 KiB and sampled thermal status is 0. The final sample has A at 700.376230 m and B stopped at 152.829249 m. See normal-run-summary.json and Device/normal-final for raw evidence. Raw CSV, screenshots, PSS and thermal samples are preserved; performance-summary.json validates the capture.
## 17. Problems encountered

S4's short B terminal cannot accommodate complete rear clearance, and its entry signal displays Stop after the train passes the origin. Review also identified that requests emitted after due-request processing need an explicit later apply tick. Available workstation memory limited compilation parallelism.

Wireless debugging disconnected several times; installation and testing resumed after reconnection. The foreground guard stopped an initial touch attempt while the launcher covered the game. A later run entered pause at tick 3303 without a simulation fault. Its cause is unknown; the user confirmed that no auto-clicker was active. That run was retained and excluded from acceptance. The clean rerun passed all 17 interaction/lifecycle checks without recurrence. Raw interrupted-run evidence remains available.
## 18. Fixes

B retains its protected route until S4 can legitimately release it. The observation adapter distinguishes an entry signal ahead from one already passed, while continuously validating authority and resources. Emitted AI route requests apply at tick + 2; controls apply at tick + 1. These contracts were verified in the final host, Unreal and Android tests.

The APK transfer verifies SHA256 before installation. Device tools guard foreground before touches, record each test touch with UTC/coordinates, reject unexpected pauses in active test phases, and preserve interrupted evidence. Profiling overrides are restored before the separate normal run. No simulation safety invariant, fixed-step overload policy or route-release condition was weakened.
## 19. Known limitations

One autonomous B in the player fixture; pure tests also bind A to demonstrate curved traversal and actual rear-clear completion. Only the compiled S4 routes are supported. No automatic reverse/shunting, timetables or dynamic path search. Target speed and braking parameters are provisional fixture values. The bounded debug trace keeps recent decisions plus a full-sequence fingerprint, not an unbounded runtime history. Performance conclusions apply only to this fixture/device/build. The completed 215 s normal run demonstrates bounded fixture stability; it is not a long-duration leak or production-scale capacity test.

## 20. Explicit scope confirmation

Only S5 AI interaction and its tests/evidence are implemented. No S6 streaming/recovery, S7 game loop, stations, missions, career, timetable, passengers, weather, production assets/UI/audio, save/migration, online services or advanced dispatcher systems are added.

Acceptance: **22/22 PASS**. Machine-readable evidence: Evidence/acceptance.json.

- [x] 1. Logical AI train entities
- [x] 2. Existing semantic TrainCommand boundary
- [x] 3. Existing S4 route-request boundary
- [x] 4. AI cannot bypass S4 authority
- [x] 5. AI cannot bypass S3 handling
- [x] 6. No direct authoritative transform mutation
- [x] 7. Movement authority obeyed
- [x] 8. Stop and restrictive conditions handled
- [x] 9. Denied routes handled safely
- [x] 10. Player and AI share the same infrastructure
- [x] 11. Conflicting protected movement prevented
- [x] 12. Whole-train occupancy preserved
- [x] 13. Rear clearance controls release
- [x] 14. AI survives actual presentation Actor destruction/recreation
- [x] 15. Deterministic decisions
- [x] 16. Render-rate independent behaviour
- [x] 17. S2 regression passes
- [x] 18. S3 regression passes
- [x] 19. S4 regression passes
- [x] 20. Unreal Automation passes
- [x] 21. Android integration and measured stable runtime pass
- [x] 22. S5-only scope; no premature S6/S7 systems

Source/config differences are in Evidence/source-changes.json; raw/source/APK SHA256 hashes are in Evidence/evidence-hashes.json. The report hash is stored separately in Evidence/report-hash.json. Interrupted runs remain preserved and are excluded from passing evidence.

## 21. Exact next dependency = S6

All 22 S5 gates have passed. Stop here. The exact next dependency is S6, which requires a separate instruction and is not implemented in this delivery.








