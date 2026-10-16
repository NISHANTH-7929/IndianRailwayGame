# S3 implementation report

Status: S3 PASS — all21 acceptance gates demonstrated. Final verification23September2026. STOP before S4.

## 1. S3 objective
Prove that a player can drive a complete formation through semantic commands, deterministic longitudinal handling and the accepted S2 path/placement kernel. This is an engineering fixture, not a finished train simulator.

## 2. Starting S2 baseline
Accepted S2:39/39 kernel tests, Unreal Automation and Android verification. UE5.8.2; iQOO Neo9Pro/API36/ARM64/Vulkan/ASTC. Existing S2 seven-edge junction/curve/grade fixture and eight-vehicle formation retained. Original source/config/descriptor in Baseline; initial hashes in Evidence/baseline-hashes.json. S2 report and historical evidence remain intact.

## 3. Architecture changes
URailSimulationSubsystem remains the only mutable world owner. It owns the existing Rail::Simulation and the new pure Rail::HandlingModel. Simulation::UpdateSteps exposes the same S2 accumulator to a handling step; the original Update delegates to it and preserves S2 commanded-speed behaviour. No graph, retained-history or placement replacement.

Flow: touch adapter → validated command queue → handling fixed step → signed displacement → S2 AdvanceDistance → authoritative formation snapshot → read-only interpolated view. S2 map retains its original demonstration; S3 map activates handling. Visual actors and UMG own no physical state.

## 4. New C++ types/files
New Simulation/RailHandling.h/.cpp: HandlingDefinition, TrainCommand, ControlState, LongitudinalState, HandlingModel. New RailHandlingTests.cpp contains28 pure checks. New RailHandlingDefinition.h wraps immutable records in editor data assets. New RailDrivingWidget.h/.cpp provides touch buttons and read-only telemetry.

Modified RailKernel.h/.cpp only at the fixed-step integration boundary; RailSimulationSubsystem.h/.cpp for ownership/input/lifecycle/evidence; RailKernelView.cpp for speed/title labels; RailAutomationTests.cpp for S3 wrapper; MyProject.Build.cs for UMG/Slate dependencies. DefaultEngine.ini and DefaultGame.ini select/cook S3 while retaining S1/S2 maps. Tools and evidence are under this S3 directory. S3 assets: L_TrainHandling, DA_PoweredHandling, DA_CoachHandling.

## 5. Command model
Commands contain train ID, assigned source ID, monotonically increasing sequence, application tick, kind and value. Throttle/brake use[0,1], reverser uses{-1,0,+1}, emergency uses{0,1}. Reject nonfinite/out-of-range values, unknown kind, wrong train/source, stale sequence/tick, paused state, more than256 queued commands or tick horizon beyond18000. Ordered future ticks support deterministic tests; player commands target current tick+1. No command writes position or speed directly.

Reverser changes require |speed|≤0.02m/s, zero requested throttle and applied throttle≤0.001. Neutral cuts traction at any speed. Emergency latches; reset requires stopped/zero throttle. A same-tick emergency application takes precedence over reset. No AI is implemented; future drivers can use this same pure boundary. Rejected commands change audit fields only.

## 6. Train state
Signed speed/acceleration, requested throttle/brake/reverser/emergency, applied actuator levels, signed traction/brake/resistance/grade/net forces, derived complete-formation mass, held flag and command audit. The eight-vehicle formation is360000kg and181m long; powered vehicle80000kg, seven coaches40000kg each,1m gaps. The existing S2 commandedSpeedMps snapshot field is populated with derived physical velocity in S3; it is not a player speed command. Copying the full pure spatial/model state supports an in-memory checkpoint test; no disk save system is added.

## 7. Traction model
All values below are provisional engineering fixture parameters, not measured Indian locomotive performance. Powered member:150000N force cap,900000W power cap,16m/s cutoff, adhesive mass fraction1, adhesion coefficient0.25. Coaches have zero traction/power. Available force is the minimum of force cap, power/max(|v|,0.1), and μ·adhesiveMass·g; a linear taper reaches zero at cutoff, beginning at min(cutoff/2,power/force). Multiply by applied throttle and reverser sign. Formation force sums powered members. Throttle slews at1/0.5 per second. Any commanded/applied brake, emergency or neutral cuts actual traction immediately. This is not wheel-slip or motor simulation.

## 8. Resistance model
Rolling force0.015N/kg; aero coefficient25N/(m/s)² for powered member and15 per coach. Rolling/aero oppose signed speed. Gravity uses g=9.80665m/s² and sums −mass·g·tangent.z at each member centre sampled from committed S2 history. Mixed flat/grade formations therefore get mass-weighted signed grade. At rest, brake holding then rolling resistance cancel available drive up to their capacities; insufficient holding allows rollback.

## 9. Braking model
Service capacity0.8N/kg, emergency1.2N/kg; each member limited by μ·mass·g. Applied brake ramps toward target:1.5s full service application,2s release,0.5s emergency application. The train-level response uses the slowest member parameter, with no per-coach physics. Moving brake force opposes velocity. Static braking holds against grade up to available force. No brake pipe propagation, thermal fade, coupler or suspension simulation.

## 10. Fixed-step integration
30Hz logical update; maximum4 steps per render update; active-time overload explicitly pauses and retains debt. Integrate constant force within a step: v1=v0+a·dt; ds=v0·dt+½a·dt². On a zero crossing, split at stop time and evaluate static holding/rollback for the remainder, avoiding brake-induced oscillation. Force readback is time-weighted over this split. Apply signed ds through S2 AdvanceDistance; commit handling state only after successful traversal. Unsupported fixture extent produces the existing explicit diagnostic fault, not movement authority or automatic braking.

Same1200-tick/40s command trace at render30/60/144Hz uses tolerance1e-9 for maximum station/speed difference; observed zero on host. This is a deterministic fixture check, not cross-platform bitwise or real-world accuracy. Preserve S2≤1cm arc bookkeeping and≤5cm bogie-centre requirements. Separate physical30/60Hz stopping convergence retains roadmap tolerance max(0.5m,1% reference distance).

## 11. Input/control design
Two rows of seven buttons: power0/25/50/100%, brake0/50/100%; reverse/neutral/forward, emergency/reset emergency, pause/resume, reset test. Button clicks enqueue commands; no held-touch state to stick after cancellation. Pause/focus/background cancels queued input and requests zero power/full service brake at the next active step. Background elapsed time and first resume delta are excluded; normal active-time overload remains enforced. Reset test explicitly restores this finite engineering scenario. HUD shows train/tick/station, signed speed/acceleration, requested/applied controls, forces, formation, stop/pause/fault state and rejection reason. Existing S2 track/body/bogie view remains.

## 12. Tests
Host tools: Tools/Test-Handling.cmd and Test-Kernel.cmd. Unreal suites: IndianRailway.S3.Handling and IndianRailway.S2.Kernel. Required T01–T17 cover stationarity, acceleration, coast, braking strength, stop, total mass, signed/mixed grade, forward/reverse committed history, multi-edge placement, render independence, lifecycle, invalid source/value and safe direction changes, full S2 regression. Additional checks cover analytic motion/stopping, response lag, emergency precedence, grade holding, adhesion, checkpoint continuation, step convergence, invalid definitions and overload.

## 13. Test results
Host S3:28/28 PASS; separate S2:39/39 PASS. Analytic acceleration maximum error2.4869e-14; analytic stop error0m; mixed-grade force error0N; render-rate maximum difference0; physical-step stopping difference0.0300587207m (tolerance0.5m). Raw JSON retains the S2 TestReport schema: errorMetres/toleranceMetres are generic error slots for some S3 checks; details identify Newton and combined position/speed checks. Unreal Automation:2/2 suites PASS,0 failed,0 warnings (67 pure assertions across wrappers), test duration0.20077s. Evidence/UnrealAutomation/index.json. Android startup also passes28/28 handling and39/39 S2 checks; Evidence/Device/final-launch/Saved/S3.

## 14. Android results

Final handoff package package-20260920-215959: BuildCookRun169.33s,163847422 bytes,SHA256 A44184E24768B707971B1F9A07A66AC8300411D0FB8D68C1D7D0FD67152D20AA. Installed23September over the owner-connected wireless ADB endpoint; remote hash matched and pm install returned Success. Normal cold launch1123ms with no automatic CSV capture. Interrupted-test override removed after matching its exact contents. Native library SHA256 69DC5BAB9596C29C63335A7AFED133FBED7B5D71C28327779454EF0A5BF5765C is identical to the measured profiling variant. Final control recheck PASS:16/16 assertions,runner exit0. Acceleration+5.71050m/s,reverse−3.71658m/s,both stop tests pass. MainPID16227 survives warm212ms resume; background tick2675 and station remain fixed, then resume tick2765 applies power0/brake1. Post-resume touch and Back exit pass. Final uninterrupted-play recheck PASS:215s scheduled driving,final tick6364,head499.476597874848m,speed0,power0,brake1,0 rejected commands, no overload/fault in the fresh log. Normal launch command line contains no CSV flags. Final APK startup again passes28/28 handling and39/39 S2 tests. Evidence:final-apk.json,handoff-install.log,handoff-launch,final-native-hash.json.
Profiling variant package-20260920-170944:BuildCookRun210.21s,163847466 bytes,SHA256 71243974B9DC8ABD52E11DC3259558C1B694272BECED9CBE5139B3D99E1BFF0F. Resumable1MiB transfer verified the same hash on-device and pm install returned Success. Cold launch1404ms. S3 map, eight vehicles, body/bogie meshes and both button rows visible. Device log confirms Adreno740 Vulkan; manifest API36/version0.3-S3 and no INTERNET permission. Airplane0/mobile data1; hotspot unchanged.28/28 handling and39/39 S2 tests pass on ARM64. The owner removed the floating tap overlay before interaction tests (pre-controls/screen.png). All16 control/lifecycle checks PASS in Device/controls/results.json. Touch acceleration reaches+5.76248m/s; coasting drops6.05272 to5.94077m/s; half brake reduces speed, full brake stops at head327.85583m; reverse reaches−3.99400m/s and reverse braking stops. Emergency/clear, paused tick/station equality, safe reverser rejection and post-resume input pass. Background tick2614/head are unchanged between samples six seconds apart; after resume tick2704 requests power0/brake1, same mainPID20303, warm190ms. Back removes app processes.

Profiling-variant S2 map smoke (same native library as final handoff): cold955ms,39/39 checks, real view recreation PASS at tick300/head280.00000000000142m/eight vehicles; curved multi-edge formation screenshot verified. S1 smoke: cold788ms,warm157ms,same mainPID6683,touch movement visible and Back exit PASS. Temporary map overrides restored; S3 performance run launches normally. The6000-frame profiling capture completed; its export subsequently triggered the intentional overload pause. See sections15–18. The initial unprofiled run lost USB after151s and is incomplete. The final wireless repeat completed successfully; the earlier incomplete run is retained as failure evidence.

## 15. Performance results
Profiling variant:6000 frames over201.844045s. Excluding10s startup leaves5747 frames over191.851292s,mean29.95549FPS. Frame mean33.38286ms,p50/p95/p99=33.370/34.567/35.220ms,max48.440ms;0 measured steady-state frames>50ms or>100ms. Mean game7.31268ms,render5.94507ms,RHI3.54268ms,GPU1.98013ms. Post-startup PSS461341–464883KiB across seven samples; thermal status0. Forward/reverse/grade driving and braking occurred during capture; sample telemetry has0 rejected commands. Evidence:final-performance.json,final-memory-thermal.json,Device/final-sustained plus full finalized CSV.

The measured capture excludes its subsequent export hitch. At capture completion the log reports async-load request dumping and CSV finalization, immediately followed by the existing fixed-step overload pause. This is retained failure evidence, not counted as a clean uninterrupted run. Normal builds now omit automatic profiling; the final unprofiled215s driving run passes beyond the capture-ending point to tick6364 with zero speed and no fault. Profiling remains opt-in. Final unprofiled post-startup PSS455359–468586KiB (seven samples); see handoff-steady-samples.json.

Compared with S2:mean game6.227→7.313ms,render5.799→5.945ms,GPU1.808→1.980ms; pacing remains approximately30FPS. Thermal status differed(S2=1,S3=0), and the camera/HUD/train motion workload is a tiny fixture. CPU lane times can overlap/include waits; these are engine intervals, not compositor timing or final-game capacity.

## 16. Problems encountered
Initial editor build: local Slot hid UWidget::Slot; UButton has no SetIsFocusable API. Source review found NativeConstruct too late for initial widget-tree construction. An external tap overlay was present initially. Automatic permission review timed out once before optional S1/S2 smoke commands started; one permitted retry succeeded. CSV profiling finalization generated an active-time hitch and correctly paused the simulation with time debt retained. The control script inherited pidof exit1 after a successful Back exit despite all16 checks passing. Evidence retained in editor-build-first-failed.log, initial screenshots, controls-run.log and controls/results.json.

## 17. Fixes
Renamed CanvasSlot, retained default focus handling, and built the widget tree in NativeOnInitialized. Rebuilt before installation. Owner closed the external overlay; clean screenshot confirmed. Review timeout resolved on retry. Control script now explicitly returns0 when every assertion passes instead of leaking the expected pidof exit1. This script-only exit-status correction does not change the installed APK or recorded assertions. Automatic profiling was removed from the normal package command line; it remains an explicit Build-S3 -CaptureFrames option. The four-step overload safeguard was not relaxed. Corrected editor/Android/host builds succeed.

## 18. Known limitations
Engineering parameters, a single formation, finite S2 graph, simple force/actuator approximation and explicit diagnostic reset at extent failure. No validated real locomotive performance, adhesion/weather model, air pipe, distributed brakes, couplers or suspension. No operations protection: the player must brake within the fixture. CSV export can trip the explicit overload pause; use a separate profiling run and RESET TEST afterward. Telemetry JSON paused means the user pause flag; subsystem fault pause is also shown in the HUD and log. No production HUD/cab/assets, save system or cross-platform bitwise guarantee. No signalling, occupancy, authority, points/interlocking, AI, stations, missions, career, World Partition or online functionality.

## 19. S3 acceptance checklist
| Gate | Result | Evidence |
|---|---|---|
| 1. Actor-independent handling state | PASS | Pure RailHandling types; subsystem owns model |
| 2. Semantic train commands | PASS | TrainCommand/Submit; next-step test |
| 3. Player input maps to commands | PASS | RailDrivingWidget;16 Android assertions |
| 4. Traction changes acceleration | PASS | T02;Android+5.76248m/s |
| 5. Coasting/resistance | PASS | T03;Android6.05272→5.94077m/s |
| 6. Braking changes speed | PASS | T04/T05;half/full touch brakes |
| 7. Train stops | PASS | T06;Androidzero speed |
| 8. Forward/reverse correct | PASS | T09/T10/T16;Android−3.994m/s/reverse stop |
| 9. Total formation mass | PASS | T07;360000kg snapshot |
| 10. Signed grade | PASS | T08 rollback/mass-weighted seam;device grade force |
| 11. S2 history integration | PASS | AdvanceDistance boundary;T09/T10 |
| 12. Multi-vehicle placement | PASS | T11≤5cm;device multi-edge screenshot |
| 13. Junction commitment | PASS | T12;preserved S2 branch test |
| 14. Fixed-step deterministic | PASS | T13/overload/analytic tests |
| 15. Render independence | PASS | Same1200 ticks at30/60/144Hz;zero measured delta |
| 16. Android touch | PASS | Device/handoff-controls/results.json:16/16 |
| 17. Android lifecycle | PASS | Frozen background tick/station,samePID,resume input,Back |
| 18. S2 regression39/39 | PASS | Host,Automation,Android startup and S2 map smoke |
| 19. Unreal Automation | PASS |2 suites,0 failures,0 warnings |
| 20. No premature S4 systems | PASS | Source/asset review;scope unchanged |
| 21. Documentation/reproducible evidence | PASS | This20-section report,experiment log,Tools/README,raw device/tests/profiling evidence and hashes |

## 20. Exact next dependency: S4
After owner review and approval only: occupancy, protected infrastructure, route requests, point locking, movement authority and signal presentation. S3 supplies the authoritative longitudinal state, shared commands and committed S2 placement needed by S4. STOP at S3; no S4 systems have been implemented.







