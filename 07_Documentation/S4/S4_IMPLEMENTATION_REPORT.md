# S4 implementation report

Status: **S4 PASS — 27/27 acceptance gates demonstrated.** Completed 2026-09-24. STOPPED at S4; no S5/S7 implementation.

## 1. S4 objective
Establish protected operations for two trains on the unchanged S2 graph: full-formation occupancy, atomic route requests, conflicting-resource exclusion, point locking, train-bound movement permission and derived signals.

## 2. Starting S3 baseline
Accepted S0–S3 PASS. S2 39 checks and S3 28 checks preserved. Source/config/project baseline and SHA256 manifest are in Baseline and Evidence/baseline-hashes.json. Device is the same iQOO Neo 9 Pro (I2304), Android16/API36, ARM64, Vulkan, ASTC. Hotspot remains enabled; wireless ADB is used.

## 3. Architecture changes
RailOperations.h/.cpp contains pure C++ definitions and OperationsWorld. URailSimulationSubsystem owns its sole mutable runtime instance. Existing S2/S3 modes remain available. RailKernel exposes an externally clocked step and transactional history extension; RailHandling exposes its existing physical step without an additional accumulator. The original clock policy is reused by all modes. Views receive const snapshots and have no resource command authority.

Created RailOperations.h/.cpp, RailOperationsTests.cpp, RailOperationsView.h/.cpp and Content/IndianRailway/S4/L_RailOperations.umap, plus S4 tools/documentation/evidence. Modified RailKernel.h/.cpp, RailHandling.h/.cpp, RailSimulationSubsystem.h/.cpp, RailDrivingWidget.h/.cpp, RailKernelView.cpp, RailAutomationTests.cpp and Config/DefaultEngine.ini/DefaultGame.ini. Exact changed paths and SHA256 values are in Evidence/source-changes.json; raw/source/tool/package hashes are in Evidence/evidence-hashes.json. No Git repository was present, so the preserved baseline comparison is the change record.

## 4. Logical resources
Stable typed BlockId, RouteId, PointId and SignalId follow the S2 Id<Tag> convention. Occupancy sets, reservation owner tokens, route state, point state, movement authority and signal state are separate records. Tokens bind RouteId, TrainId and a monotonically increasing grant generation. Definitions are immutable compiled fixture records; they are not Actor identities or production authoring tools.

## 5. Block model
B1 E1[0,220] is Train A's origin holding area; B2 E1[220,240] protects the split; B3 E2[0,130] is Train B's origin holding area; B4 E2[130,160] protects the lower approach; B5 covers the ordered connected upper edges E3–E6; B6 E7[0,10] protects the common merge; B7 E7[10,220] is Train A's terminal holding area. Definitions partition all graph edges and reject gaps/overlap, invalid extents and disconnected coverage. Boundaries conservatively count contact.

## 6. Point model
P1 has lower T1 and upper T2 positions, protecting B2/B5. P2 has lower T7 and upper T6 positions, protecting B4/B5/B6. Current position, requested position and owner token remain separate. Requests cannot move a point while its protected area is occupied or reserved, or while any route locks it. A denied request may update the displayed requested position, never the physical logical position.

## 7. Route model
R1 belongs to A: E1=220 through E3/E4/E5/E6 to E7=219; reserves B2/B5/B6 and locks P1/P2 upper. R2 belongs to B: E2=130 to E7=5; reserves B4/B6 and locks P2 lower. R3 is a local B holding-area route E2=100→120, used to prove safe nonconflicting coexistence. Train A is 181 m/360000 kg; B 43 m/120000 kg. B uses unique VehicleIds 101/102. No topology extension was made.

## 8. Occupancy model
For each train intersect [head−derived formation length,head] with every directed S2 history occurrence, then with all authored block ranges. Coupler gaps are included conservatively. Reverse occurrences map back to authored edge coordinates. Occupancy is a set of TrainIds, so an invalid overlap is represented and rejected rather than hidden. Every step also checks the swept union of previous and candidate formation intervals, including blocks fully crossed between endpoints.

## 9. Conflict model
B4/B5 have symmetric explicit fouling conflicts; R1/R2 also share B6/P2. Any conflicting reservation or physical occupancy denies a grant. Same-tick requests sort by TrainId then sequence, independently of insertion/render order. Runtime validation recomputes occupancy and verifies complete owner coherence, exclusive holding assignments, point positions and authority bounds.

## 10. Route locking
Evaluate a grant in a candidate state containing train history, all locks, occupancy and grant generation. Validate origin, required resources and point protection; extend committed S2 history along the permitted route; reject an incompatible already-committed branch or omitted shared block. Only then install all block/point tokens and the route owner together. Denial discards the candidate. Safe pre-entry cancellation requires the exact token, stopped train and complete origin footprint.

## 11. Movement authority
A permission record binds the train, owner token and an explicit interval: minimum permitted formation rear and maximum permitted head, plus endpoint TrackPosition. Without a route, a train can move only wholly inside its immutable assigned holding area. This is explicitly labelled holding-area permission, not a granted junction route. A normal restrictive approach submits throttle0/service-brake1 through S3 when distance <= v²/(2×0.55)+2|v|+2 metres. This conservative fixture envelope is provisional. A proposed step outside permission or into conflicting resources is rejected transactionally and produces a diagnostic pause, preserving prior physical position and velocity. It never teleports or assigns speed zero. Missing/corrupt state invalidates permission and makes signals Stop.

## 12. Signal model
S1/S2/S3 are located at their route origins. Minimal Stop/Proceed aspects derive from valid route/authority ownership and whether the train has passed the origin. Once the front passes, Stop can be displayed while locks remain for the rear. No aspect, mesh, view pointer or visual cache is read to authorize motion. This is a protected simulation fixture, not a complete Indian Railways rule implementation.

## 13. Route release
After occupancy update, record entry into protected resources. Release the complete route transaction only once the entire formation has cleared all reserved protected blocks. Points and reservations release together; the train must still have a valid holding-area permission or route corridor. A's rear must pass global station501.327412 m, requiring its head beyond682.327412 m. The small terminal cannot hold both formations: its train-assigned division and B's E7=5 endpoint are explicit fixture restrictions. No generic berth allocator is claimed.

## 14. Fixed-step ordering
One 30 Hz clock, maximum 4 steps per render update, no skipped debt. Each step: validate prior state; process due requests atomically; derive permissions; apply S3 service-brake envelope; advance each S2/S3 train once; check all swept movement against prior occupancy and ownership; recompute complete occupancy; release rear-cleared routes; increment common tick; derive permissions/aspects; verify invariants; commit candidate. Any failed candidate leaves physical state at the prior tick. Suspension excludes elapsed background time, cancels driving input and skips resume delta.

## 15. Debug tooling
The existing instanced train view presents both formations; one ARailOperationsView displays all infrastructure, with orange occupancy, purple reservations, signal IDs/aspects and point positions/owners. No Actor per block/route is created. HUD shows selected TrainId, edge, distance, speed, tick, throttle/brake, route owner, endpoint, blocks, locks, aspects, last request/denial and release event. Buttons select A/B, request A/B routes, attempt P2 movement, cancel pre-entry route and recreate presentation. JSON evidence records the logical state. Actual view recreation compares full logical JSON before/after, old/new Actor IDs, Destroy return and reconstructed presentation digest.

## 16. Tests
39 S4 pure checks PASS, including T01–T28 and extra invariant checks; S2 39 and S3 28 regressions PASS. Pure T25/T26 snapshot-copy checks are explicitly supplemented by required actual Unreal/Android Actor evidence. Unreal Automation runs all three suites. Raw results and exact source hashes are retained. Android startup suites also pass 39/39 S4,39/39 S2 and 28/28 S3 with matching case names. Actual Actor recreation under a granted route passed at tick 184 (Actor IDs 32997→33154, identical full logical JSON and presentation digest). Android touch checks PASS 17/17, including both route states, movement, restrictive stop, rear clearance, point denial, view recreation, pause/resume, background/foreground and Back. Performance and normal-run audit PASS; see sections 18/22.

## 17. Android verification
Package build 20260923-213720 succeeded in 207.89 s. APK 163932558 bytes, SHA256 25558753865EF0EA3AA606564121A5F49CFFF79215324D1B4B8025412A0B0B79; remote hash matched and pm install -r succeeded. Cold launch 2114 ms. Startup suites pass, two-train/signal display inspected, actual granted-state Actor recreation passes. Final controls-complete run PASS 17/17. B stopped at 118.569351 m before its 130 m restriction; A stayed locked at 668.104170 m until the second driver command and rear clearance; A stopped at 700.376230 m in its terminal; B then received R2/P2-lower/Proceed and moved to 154.020172 m below 165 m authority end. Pause froze tick 7557; background froze tick 7662; same main PID 14932 resumed at tick 7752; Back exited. Raw per-stage JSON/screenshots and results.json are in Evidence/Device/controls-complete. Performance and default-launch sustained verification also PASS.

## 18. Performance
Same final APK, profiling enabled only by temporary commandline. Finalized CSV contains 6000frames over 202.562660 s. Analysis uses 5093frames over 169.978529 s in the 10–180 s capture window, excluding startup and the deliberate 190 s export pause. The complete raw CSV is preserved as Evidence/Device/profile-final/Saved/Profiling/CSV/Profile(20260924_072838).csv. Nearest-rank percentiles:

| Measurement | Result |
|---|---:|
| Average frame rate |29.962608 FPS|
| Frame interval mean / p50 / p95 / p99 |33.374932 /33.373 /34.452 /35.080 ms|
| Maximum active frame interval |38.168 ms|
| Frames over 50 /100 ms |0 /0|
| Mean game / render / RHI thread |7.629188 /6.009788 /3.474699 ms|
| Mean / p95 GPU |1.965461 /3.063 ms|
| Profiled PSS, samples 1–8 |468569–475577 KiB|
| Thermal status, samples 1–8 |1 throughout|

Counters are engine timings, not compositor presentation timestamps. CPU lanes overlap and can include waits; they must not be added into a frame total. Sample 0 is excluded from memory/state assessment because it can precede fresh telemetry. Raw engine CPU-temperature counters are also preserved in the summary; no controlled ambient or low-end qualification is claimed. The profiling override was confirmed absent afterward. The final 215 s normal run also PASS: profiling off, no deliberate pause, no fault/overload log matches, all seven post-startup samples valid. Ticks 810→6211 across the sampled interval; PSS 414001–461876 KiB, thermal statuses 0/1. The normal commandline contains no CSV flags. See Evidence/normal-run-summary.json and the full raw normal-final run. These results qualify only the engineering fixture on this iQOO, not final-game capacity.

## 19. Problems encountered
First test trace stopped A at 668.104170 m under conservative braking, before rear clearance; the competing route correctly stayed denied. Original failure is preserved. Android Clang rejected misleading serializer indentation; explicit braces fixed it without lowering warning policy. Early phone runs were interrupted by Quick Settings/auto-clicker foreground changes; these failed/incomplete runs remain preserved. The final guarded run passed after the user prepared the phone. A source-write attempt exceeded Windows command-line length before execution. Host memory limited Unreal build concurrency to one action.

## 20. Fixes
Added a fixed second driver command at tick 4500 to release the brake and apply power, allowing A to clear safely. No rear-clearance invariant was weakened. Used apply_patch for large source edits. Retained compiler warnings-as-errors. Added foreground checks before Android touch injection and once per second during performance measurement. A covered game aborts the test before further touches. Profiling uses a temporary commandline restored in finally, with a deliberate simulation pause before CSV export; the active 10–180 s analysis window excludes that pause.

## 21. Known limitations
Compiled bounded fixture data, not a route editor. Exclusive train-assigned holding areas and conservative whole-route release. Forward authored routes with retained reverse traversal, no loop/repeated-edge route definitions. Provisional service-brake envelope; hard-guard violations intentionally pause the simulation for diagnosis rather than model a physical instantaneous stop. No production save/migration, dispatching rules, national signalling family, AI, timetable, stations, missions, career or final assets. Device/performance qualification remains limited to this fixture.

## 22. S4 acceptance checklist
**27/27 PASS.** Machine-readable audit: Evidence/acceptance.json. Raw/source/config/map/tool/APK SHA256 manifest: Evidence/evidence-hashes.json.

- [x] 1. Actor-independent occupancy — operations-tests.json / S4-T01; source-changes.json and reviewed pure architecture
- [x] 2. Stable logical block/resource IDs — operations-tests.json / S4-T02; source-changes.json and reviewed pure architecture
- [x] 3. Whole-train occupancy — operations-tests.json / S4-T02; source-changes.json and reviewed pure architecture
- [x] 4. Explicit route requests — operations-tests.json / S4-T05; source-changes.json and reviewed pure architecture
- [x] 5. Deterministic conflict detection — operations-tests.json / S4-T06; source-changes.json and reviewed pure architecture
- [x] 6. Atomic route granting — operations-tests.json / S4-T08; source-changes.json and reviewed pure architecture
- [x] 7. Protected point positions — operations-tests.json / S4-T10; source-changes.json and reviewed pure architecture
- [x] 8. Point locking — operations-tests.json / S4-T08; source-changes.json and reviewed pure architecture
- [x] 9. Whole-train route protection — operations-tests.json / S4-T18; source-changes.json and reviewed pure architecture
- [x] 10. Explicit movement authority — operations-tests.json / S4-T12; source-changes.json and reviewed pure architecture
- [x] 11. Authority tied to correct TrainId — operations-tests.json / S4-T12; source-changes.json and reviewed pure architecture
- [x] 12. Authority endpoint enforced — operations-tests.json / S4-T13; source-changes.json and reviewed pure architecture
- [x] 13. Signal state derived from authoritative operations state — operations-tests.json / S4-T14; source-changes.json and reviewed pure architecture
- [x] 14. Signal cannot independently create authority — operations-tests.json / S4-T15; source-changes.json and reviewed pure architecture
- [x] 15. Conflicting movements cannot both be authorized — operations-tests.json / S4-T06; source-changes.json and reviewed pure architecture
- [x] 16. Rear clearance required for release — operations-tests.json / S4-T18; source-changes.json and reviewed pure architecture
- [x] 17. Points cannot release/move while unsafe — operations-tests.json / S4-T11; source-changes.json and reviewed pure architecture
- [x] 18. Stale requests cannot steal resources — operations-tests.json / S4-T17; source-changes.json and reviewed pure architecture
- [x] 19. Missing safety state fails safe — operations-tests.json / S4-T16; source-changes.json and reviewed pure architecture
- [x] 20. Presentation Actors survive destruction/recreation logically — Device/controls-complete/03-view-recreation.json
- [x] 21. S2 regression passes — cross-platform-tests.json / S2 39
- [x] 22. S3 regression passes — cross-platform-tests.json / S3 28
- [x] 23. Unreal Automation passes — UnrealAutomation/index.json
- [x] 24. Android verification passes — Device/controls-complete/results.json; normal-run-summary.json
- [x] 25. Fixed-step/render-rate independence passes — operations-tests.json / S4-T23
- [x] 26. Reproducible evidence exists — evidence-hashes.json; source-changes.json; Tools/README.md; performance-summary.json
- [x] 27. No S5/S7 systems implemented prematurely — Reviewed source-changes.json: S4 operations/tests/presentation only; no S5/S7 implementation

## 23. Exact next dependency: S5
S4 is complete. STOP here. The exact next dependency is S5 AI interaction through the same semantic train command and route-request boundaries. No AI, timetable, stations, missions or career systems have been implemented.







