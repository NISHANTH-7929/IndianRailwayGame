# S2 implementation report

**S2 STATUS: PASS — engineering fixture acceptance, 20 September 2026.**

Final APK: `08_Builds/S2_Android/Android_ASTC/MyProject-arm64.apk`, 163801846 bytes.
SHA256: `D11DF26857979E4CAA18EEC7B2E42CFAEAC13797B2F147CF4D05DFE5EC76333B`.
Installed application: Indian Railway S2 Kernel, version0.2-S2, package `com.indianrailway.s1harness`.
S3 has not been implemented and requires explicit owner approval.

## 1. Objective

Prove actor-independent railway coordinates, explicit graph traversal, retained branch history and multi-vehicle placement. Movement is a commanded geometric test input, not train physics.

## 2. Initial project state

The owner explicitly accepted S1 in the S2 request. The project was UE5.8.2 content-only, with an accepted Android harness and no Source module. Original descriptor/config/S1 assets are retained under Baseline, with baseline-hashes.json. Historical S1 offline limitations were not rewritten. The phone supplies the workstation hotspot, so its network settings remained enabled throughout S2.

## 3. Architecture decisions

Pure C++17 kernel with no Unreal headers or Actor/UObject references. URailSimulationSubsystem is the single mutable world owner. ARailKernelView consumes copied snapshots and owns instanced primitive meshes, not train state. No spline projection, Actor-transform authority, Level Blueprint, or Actor per logical edge/vehicle.

Thirty-Hz fixed steps with at most4 steps per update. Active-time overload explicitly pauses and retains debt rather than skipping simulation. Background/foreground callbacks exclude time when the application is suspended. Test speed changes occur on fixed ticks. Render interpolation resamples the retained path between logical stations.

## 4. C++ types created

Typed NodeId, EdgeId, TransitionId, DefinitionId, VehicleId and TrainId; Vec3, Geometry, TrackPosition, DirectedEdge, Graph, Occurrence/PathHistory, VehicleDefinition/Instance, Formation/Properties, VehiclePose, Snapshot, Simulation, Fixture and TestReport.

Unreal classes: URailVehicleDefinition for authored immutable records, URailSimulationSubsystem for ownership, ARailKernelView for presentation. One runtime module, game/editor targets, and one Unreal Automation suite wrapping the pure tests.

## 5. Blueprint assets created

None required. The view and camera are C++; editor-only Python creates the small assets. Python is not a runtime dependency.

## 6. Data assets and fixtures

Content/IndianRailway/S2 contains DA_Powered, DA_Coach, L_RailKernel, M_S2_Body and M_S2_Bogie. The map is an ordinary non-partitioned level. Vehicle assets are copied into validated pure records at initialization. The graph is a small compiled analytic fixture. Dedicated unlit materials support InstancedStaticMeshes; original S1 materials were preserved.

## 7. Files created

Within 03_UnrealProject/MyProject/Source:

- MyProject.Target.cs and MyProjectEditor.Target.cs.
- MyProject/MyProject.Build.cs, MyProject.cpp and S2Offline_UPL.xml.
- MyProject/Simulation/RailKernel.h, RailKernel.cpp and RailKernelTests.cpp.
- MyProject/RailVehicleDefinition.h.
- MyProject/RailSimulationSubsystem.h/.cpp and RailKernelView.h/.cpp.
- MyProject/RailAutomationTests.cpp.

Five S2 content assets listed above. Documentation, baseline and evidence are under 07_Documentation/S2. Tools include KernelMain.cpp, Test-Kernel.cmd, Create-S2Assets.py, Fix-S2Materials.py, Build-S2.ps1, Transfer-S2.ps1, Test-S2Device.ps1, Measure-S2.ps1, Smoke-S1.ps1 and S1-smoke-commandline.txt. Exact final source hashes are in Evidence/final-source-hashes.json.

## 8. Files modified

MyProject.uproject adds the native runtime module. DefaultEngine.ini selects the S2 map and S2 application/version labels. DefaultGame.ini cooks both maps and S2 definition/material assets. S0_S1/Tools/Build-S1.ps1 now explicitly launches the S1 map even though the project default is S2, and retains the closing frame-pacer quote with a trailing flag.

The five original S1 content files are byte-identical to baseline: Evidence/s1-asset-regression.json.

## 9. Track graph design

Seven nodes, seven bidirectional edges, fourteen directed transitions.

| Edge | Geometry | Length |
|---|---|---|
| E1 | Straight approach, (-240,0,0) to origin |240m|
| E2 | Straight lower branch to merge (160,0,0) |160m|
| E3–E6 | Four radius40m quarter circles through (40,40), (80,80), (120,40), (160,0) |20*pi m each|
| E7 | Shared exit, dz/ds=.02 |220m|

T1/T2 explicitly select lower/upper at the split. Reverse approach to the merge selects T107/T106. E2 and E6 are two legal approaches to E7. Other transitions are unique. Validation covers stable/unique IDs, finite positions, positive lengths, endpoints, directed permissions/connections, duplicate transitions, analytic lengths and positional seams. Grade joins have C0 positional continuity; vertical easements are absent.

## 10. Coordinate/unit conventions

Canonical position is edge ID + metres from the authored edge start + direction +/-1. Path station is distinct and increases along recorded edge occurrences. Negative commanded speed moves backwards on the same path; it does not reorder members or reverse authored vehicle orientation. Snapshot leadingEnd identifies the movement-leading end, including reversed direction, while head remains the permanent member0 geometric anchor.

Geometry integrates heading change per true3D arc metre and grade dz/ds analytically. Mass is kg, time seconds, speed m/s. Calculations use doubles. The view explicitly converts metres to Unreal centimetres at100cm/m.

## 11. Train formation design

Powered definition:20m,80000kg, bogies at+/-7m. Coach:22m,40000kg, bogies at+/-8m. Gap1m.

Short formation:2 vehicles,43m,120000kg. Long formation:8 vehicles,181m,360000kg. Each contains one powered member. Count, mass, length and powered capability are derived from ordered members. Reversed variants flip authored local orientation while retaining member IDs/order. Invalid/missing definitions, duplicate instance IDs and invalid dimensions are rejected.

## 12. Path-history implementation

Ordered occurrences store directed edge, begin/end station and exact preceding TransitionId. Extension and distance advance are transactional. A later junction choice cannot rewrite committed occurrences; every trailing reference uses the same tape. Reverse movement retraces that tape and checks legal inverse traversal. Prepending supports initial formations spanning previous edges.

Ambiguous choices and end-of-track reject without moving state. History retains up to4096 occurrences and fails explicitly at capacity. Pruning and save/restore are outside S2.

## 13. Placement implementation

Member centres come from ordered lengths/gaps. Front/rear body references and bogie offsets sample the same recorded path. Body forward follows the bogie chord; body origin interpolates between bogies, including asymmetric offsets. Visual length comes from the definition. No nearest-track projection.

The rigid body chord need not coincide with the curved centreline. Reference points are geometric fixtures, not suspension, wheel-contact or coupler physics.

## 14. Debug tooling

Overview camera; edge IDs/directions, junction markers and retained-path lines; instanced bodies/bogies and reference segments; stable vehicle IDs and canonical positions. Screen text reports train/tick, head, direction, commanded speed, formation totals, branch choice, history and errors.

The test changes future choice to lower after committing upper. A fixed-tick100-second forward/reverse cycle demonstrates retracing. At tick300 the real view Actor is destroyed/recreated; runtime JSON verifies unchanged tick/head/member count. Screenshots verify recreated meshes. Source separation prevents the view from accessing mutable simulation state.

## 15. Tests executed

- Host:39/39 pure checks PASS with MSVC /W4 /WX /O2.
- Unreal Automation: IndianRailway.S2.Kernel PASS,0 failures/0 warnings.
- Android startup on final APK: same39/39 checks PASS.
- Actual Actor destruction/recreation: PASS at tick300, head280.00000000000142m,8 vehicles.
- Corrected Android meshes, multi-edge curved placement and reversal: PASS by screenshots and kernel evidence.
- Background/resume: PASS, same process28534, continued movement, warm resume157ms; no overload on resume.
- S1 smoke on exact final APK: PASS — cold launch807ms, injected touch response, same-process8894 background/resume155ms, Back removes app processes.
- Final6000-frame capture: complete,201.818132 seconds; no active-play overload.

Coverage includes T02 analytic straight/curve/grade and14 directed seams forward/reverse; T03 eight vehicles spanning at least3 edges, changed selector after entry, committed rear path and reversal; T04 analytic curve/bogie/body and translated coordinates. Additional checks cover alternate branch, reverse-directed start, prepended history, ordering/orientation, invalid data, atomic end-of-track, ambiguity, forbidden reverse traversal, overload, disposable snapshots and30/60/144Hz render schedules including tick-scripted reversal.

T03 point-lock/authority behavior belongs to S4 and is not claimed.

## 16. Numerical results

Maximum errors, metres:

| Check | Host | Android | Tolerance |
|---|---:|---:|---:|
| Straight and grade analytic samples |0|0|.01|
| Radius40m quarter-circle,1001 samples |1.2809491335957507e-14|same|.01|
|14 directed positional seams |7.105427357601002e-15|same|.01|
|400m forward/back bookkeeping |0|0|.01|
| Coach bogies on curve |1.7763568394002505e-15|3.552713678800501e-15|.05|
| Bogie with10,000km x/y translation |8.786409648948585e-10|same|.05|
|20s at30/60/144 render FPS |6.59383658785373e-12|same|.01|
|100s tick-scripted forward/reverse |5.684341886080802e-14|same|.01|

Expected curve point is (40*sin(s/40),40*(1-cos(s/40)),0. Coach head40m gives centre29m and bogies37m/21m; body origin is their midpoint. Grade reference is (160+s*sqrt(.9996),0,.02*s). Tolerances were not loosened. These are synthetic fixture results, not surveyed railway or extreme-coordinate rendered precision claims.

## 17. Android verification and performance

iQOO Neo9Pro, vivo I2304, Android16/API36, arm64/Vulkan ASTC Development. Final package built successfully in230.07s. Resumable1MiB transfer verified exact SHA256 before installation; pm install returned Success.

**App-level offline launch PASS:** S2Offline_UPL removes INTERNET permission. Installed final package dump confirms absence; app cold-launched with airplane0 and mobile data1. This proves this app can launch without network access while preserving the phone hotspot. A handset airplane-mode test was intentionally not performed.

Final capture:6000 frames/201.818s. Excluding first10s leaves5748 frames/191.829s. Engine frame intervals, nearest-rank percentiles:

| Metric | S1 accepted baseline | S2 final fixture |
|---|---:|---:|
|Average FPS|29.968|29.964|
|Frame p50/p95/p99 ms|33.351 /34.593 /35.236|33.372 /34.720 /35.459|
|Mean game-thread ms|5.041|6.227|
|Mean render-thread ms|4.269|5.799|
|Mean RHI-thread ms|3.127|3.563|
|Mean GPU ms|1.597|1.808|
|Post-warmup frames over50ms|0|0|

The added fixture/debug view increases measured CPU/render work while retaining30fps pacing. CPU lanes overlap; do not sum them. These are separate runs with different thermal conditions, not a controlled causal benchmark or final-game performance.

S2 post-startup sampled PSS487510–493421KiB; initial loading sample307717KiB is excluded from the steady comparison. Thermal status1 throughout sampling. S1 had475485–482425KiB PSS and mostly thermal2. No long-session leak, support-floor or production performance claim.

The same APK contains and successfully runs the preserved S1 map. Temporary map override was removed; default S2 view was restored and visually verified.

## 18. Problems encountered

Unsupported VS2019-only initial toolchain; missing NetFxSDK; installer elevation requirement; Unreal check-macro collision; missing instanced-material usage; background interval counted as active simulation time; repeated USB disconnections and host ADB loopback timeouts; initially assumed wrong Saved directory. One editor command was deferred when automatic approval review hit a usage limit; it ran normally after access returned.

Initial visual/lifecycle failure evidence is retained under Device/verified; it is not represented as passing.

## 19. Fixes applied

Installed MSVC19.44.35229, Windows SDK22621 and .NET Framework4.8 SDK. Corrected explicit ID default construction and graph_ member reference; renamed helper recordCheck; added inverse-direction checks and fixed-tick demo commands. Created separate instanced unlit materials; excluded suspended time through lifecycle callbacks without changing active-time overload policy.

Used hash-qualified resumable transfers, recovered host ADB, verified final APK hash, and corrected retrieval to UnrealGame/MyProject/MyProject/Saved. Final visual/lifecycle/kernel/performance and S1 tests passed after these fixes.

## 20. Known limitations

Small compiled analytic topology, bounded unpruned history, C0 grade seams, one fixture train and simple overview. Debug labels can overlap at this scale. Rigid geometric placement has no coupler/suspension/contact dynamics. Active hitches exceeding the four-step budget deliberately pause; relaunch resets the fixture.

No traction, braking, signalling, occupancy, authority, point locking, AI, timetable, gameplay, production UI/assets, World Partition or online services. No airplanemode test, physical-finger usability study, low-end-device qualification, long-session thermal/leak proof, or final-game performance claim. The Internet-free manifest is intentional for this offline harness.

## 21. S2 acceptance checklist

| Gate | Result | Evidence |
|---|---|---|
|1 Actor-independent logical state|PASS|Pure kernel and read-only view boundary|
|2 Edge + distance + direction|PASS|TrackPosition and directed-position tests|
|3 Deterministic traversal|PASS|Host/Android suite and fixed-step tests|
|4 Explicit junction transitions|PASS|Graph validation and branch tests|
|5 Retained history|PASS|T03 and reverse/prepend tests|
|6 Multi-vehicle formations|PASS|2/8-member derived-property tests|
|7 Every vehicle follows committed path|PASS|T03 selector-change test and screenshots|
|8 Forward/reverse work|PASS|T02, fixed-tick reversal, Android samples|
|9 Curves/grades work|PASS|Analytic tests and device view|
|10 Long formation spans edges|PASS|T03 at least3 occupied edges|
|11 Invalid graph/definitions rejected|PASS|Negative tests|
|12 Prototype tolerances pass|PASS|Numerical table and JSON|
|13 Real view removable/recreatable|PASS|Runtime view-recreation.json and rendered replacement|
|14 S1 remains functional|PASS|Asset hashes and final-APK smoke|
|15 Tests reproducible|PASS|Checked-in runners, fixtures, logs and hashes|
|16 No premature excluded systems|PASS|Source scope review|

## 22. Evidence and reproduction

Key evidence paths, relative to this directory:

- Evidence/host-tests.json and host-compile.log.
- Evidence/UnrealAutomation/index.json; editor-build.log and material-fix.log.
- Evidence/package-20260920-081350.log and corresponding result JSON.
- Evidence/final-apk-hash.json, final-source-hashes.json and final-install.log.
- Evidence/Device/final-launch/package.txt and screen.png.
- Evidence/Device/lifecycle-before and lifecycle-after.
- Evidence/Device/final-sustained/Saved/S2/kernel-tests.json and view-recreation.json.
- Evidence/Device/final-sustained/Saved/Profiling/CSV/Profile(20260920_082320).csv.
- Evidence/final-performance.json, final-memory-thermal.json and final-sustained-sampler.log.
- Evidence/Device/s1-smoke*, s1-smoke-run.log, s1-asset-regression.json.
- Evidence/Device/final-restored/screen.png and launch.txt.
- EXPERIMENT_LOG.md records failures, corrections and bounded results.

Reproduce on the configured workstation: run Tools/Test-Kernel.cmd; run Unreal Automation IndianRailway.S2.Kernel; package with Tools/Build-S2.ps1; install using Tools/Transfer-S2.ps1; use Test-S2Device.ps1 for launch/snapshot/pull, Measure-S2.ps1 for the210-second sampler, and Smoke-S1.ps1 for the reversible S1 map override. Build paths/SDK/compiler versions are explicit in the scripts. The device runs the pure suite automatically on S2 world startup. Regenerate assets with Create-S2Assets.py and Fix-S2Materials.py through the editor Python commandlet when needed.

## 23. Exact next dependency: S3

S2 is complete at its engineering gate. Stop here. With explicit owner approval, S3 can add shared semantic train commands, player input and traction/braking/handling on this kernel. S3 implementation has not begun.
