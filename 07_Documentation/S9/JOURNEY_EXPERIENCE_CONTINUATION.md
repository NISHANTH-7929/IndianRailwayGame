# Journey continuation — 17 October 2026

Retain WAP-7/LHB and Egmore–Tambaram. User wants Assisted and manual Standard
selectable, meaningful track switching and opposing traffic. No new stock/corridor,
Android, paid assets or publishing.

Latest compiled/cooked package:
08_Builds/S9_JourneyRealism-Shipping-20261017-02/Windows.
Start Railway Journey.cmd uses normal saves. Current UI test uses isolated
07_Documentation/S9/Evidence/JourneyExperience-Packaged-20261016-01/UserData.

Completed the recovered journey, full 10-second dwell, Grade A 95.5337,
berth error -0.5955 m. Saved reward via UI; cold restart preserved result,
one award, reputation 10, unlock and settled fingerprint aad576dcbfeebd4b at
tick 54882. PID 22560 is paused on recovered result. Do not click Replay unless
intentionally starting a new run. Existing 118 checkpoint files unchanged.

Evidence: Evidence/JourneyCompletion-20261017-01/journey-summary-reviewed.json
and three result screenshots. Telemetry snapshots are in the isolated journey
evidence directory. Earlier frame overloads at ticks 35852 and 38590 remain
preserved. This is recovered completion, not an uninterrupted pass.

Implemented and cooked: station-aware HUD/ambience, clear mode briefing,
position-based traffic/junction notices, varied scenery density/palette/spacing,
eleven derivative cab/glass/coach materials, closer exterior camera, distant
scenery prefetch budget and cleared stale recovery notice. Journey tests 62/62
passed; Editor/Shipping builds passed. Content IR-S8-MSTBM-6,
service IR-S8-MSTBM-service-7, save ServiceV6 unchanged.

Remaining gameplay work: player route stays straight; only one crossing express
and one adjacent opposing train exist. HUD cues do not fix that gap. Develop a
versioned service with player turnout movements and additional protected encounters;
preserve old saves and test route locks, conflicts and whole-formation clearance.
Both driving modes should share safe dispatch rules. See GAMEPLAY_GAPS.md.

Still open: recurring scenery-cell hitches, full-route performance acceptance,
authentic WAP-7/LHB recordings, coach/interior detail and route-art fidelity.
PresentMon captures are documented in JOURNEY_REALISM_REPORT.md; different
routes/window sizes prevent a matched causal improvement claim.


## Active live-service implementation — 17 October

User authorized implementation. New service IR-S9-MSTBM-LIVE-1 retains old
content and adds protected player crossover plus three moving opposing trains
(five trains total). Service/save identity S9-MS-TBM-LIVE-01,
IR-S9-MSTBM-live-service-1, Saved/S9/EgmoreTambaram/LiveServiceV1.
Use -S8Tambaram -S8Cab -S9LiveJourney. Both modes remain selectable.

Host-06: 70/70 PASS (three moving encounters, locked crossover, mid-crossover
cold recovery, manual controls, legacy identity separation, full terminal dwell).
Regression-02: 238/238 PASS. All118 baseline checkpoints unchanged.
Nearest-opposing audio source implemented after Host-06 (presentation-only).

Shipping03 completed successfully; both launchers and package hash manifest
are present. Packaged UI verified mode briefings, protected traffic wait,
both crossover edges under both point locks, and exact cold recovery at
12843 / fingerprint96752b731205d63f / generation50. No sampled faults.
Evidence: LiveJourney-Packaged-20261017-01/verification.json and screenshots.
Game PID40836 remains paused in the isolated test profile after restart.
Previous sampler ended with old process15232. Use a fresh sampler label.
Full new packaged terminal journey and visual observation of all three passes
remain pending; host full journey passed. Previous original-service terminal
completion and reward verification do not qualify this new packaged service.

Host04/05 retained encounter failure: first opposing train reached its holding
area before player; changed its start segment40 to60. Host06 verifies all three
moving passes at roughly9.8m/s. LiveJourney-Host-20261017-06 records final simulation.
See LIVE_JOURNEY_IMPLEMENTATION.md. Broader performance/visual/audio realism
remains limited; do not claim verified authentic sound or smooth whole-route play.

## Journey finish pass - active 17 October

User requested all next steps. Live Shipping03 resumed from tick12843 to25080,
near Guindy. First opposing pass screenshot saved; second passing HUD observed.
No sampled faults before deliberate pause/save/exit. Pre-upgrade fingerprint
7ce63bd66257242c (tick25080). Profile remains LiveJourney-Packaged-20261017-01/UserData.
Game is closed for packaging. Resumed 83-second frame capture: p99 7.3515ms,
max72.9494ms, zero >100ms; no whole-route performance claim.

Implemented source: bounded shared foliage/ground MID palette (18 per corridor
actor), varied rooftop tanks/panel banks, opposing audio source identity reset.
Imported18 finish derivatives in JourneyFinishV2 (cab roughness/glass and coach
underframe/paint finish separation). Imported five JourneyAudioV3 synthesized
clips, longer16s loops and reduced dominant hum, boundary continuity verified.
No authentic WAP-7 recording claim. Original S8 assets and118 checkpoints retained.
Both runtime cook roots added. AudioBank test follows new asset paths.

First material import failed on Noise Position pin; fixed using first input
per installed MaterialEditingLibrary.cpp; retry passed and receipt records18.
Build JourneyFinish-Shipping-20261017-01 stalled across interruption for hours;
only owned UBT19740 terminated, evidence retained. Fresh build02 currently runs
via hidden PowerShell7956. Need finish package; run focused audio/cab/LHB tests
and acoustic capture; add launchers; resume live test profile in new build,
verify exact fingerprint, then remaining journey/third encounter, dwell, reward,
cold restart. Capture fresh frame times. Write final report and package manifest.

Build02 succeeded exit0, six focused Unreal tests passed (AudioBank,
AudioDynamics, AudioLifecycle, PassingAudio, CabStabilization, LHBPresentation).
Both launchers added, package hash manifest written. Game PID21092 in new
JourneyFinish-Shipping-20261017-02 restored tick25080 fingerprint7ce63bd66257242c
exactly (generation94), then resumed. Native window2556684. Sampler6308 writes
finish-observations.jsonl in LiveJourney-Packaged-20261017-01. PresentMon43584
sessionRailJourneyFinish writes finish-presentmon.csv (1200sec bound).
Remaining third opposing pass around tick32520; terminal around52138. Need
native dwell/reward/cold restart and finish evidence. Keep real-time game running;
no build/editor resource competition during drive. New build cab visual inspected.

## 18 October continuation
Desktop journey completed, reward saved once, and exact settled recovery verified.
See JOURNEY_FINISH_REPORT.md for the final evidence and frame-stall limitation.
User now explicitly authorized Android implementation and all required testing.
Android candidate Journey-Android-20261018-03 built successfully after two
compiler portability fixes. Phone I2304 Android16 connected over wireless ADB.
243 saved files backed up; all18 checkpoint hashes verified. Streamed install
failed during transport reconnect; non-streaming install retry is in progress.
Mobile evidence Journey-Mobile-20261018-01; no mobile pass claimed yet.
Fresh host journey checks pass70/70. See MOBILE_JOURNEY_TESTING.md for coverage.


## Completed mobile journey — Android07
Installed Android07 in place, retaining all18 older checkpoint files (final hash
comparison passed). Recovered tick42594 with exact fingerprint22b4169c9d5fbdf8.
Completed at tick52138: GradeA95.4723, berth error-0.603698m, dwell300ticks/10s,
overspeed0, player lateness0, traffic wait4767ticks/158.9s. Saved +10 reputation
and mastery unlock; force-stop/cold restart retained the exact settled fingerprint
and profile with awards=1. No duplicate reward.
Active-driving KEEP_SCREEN_ON observed on Android; released at Result. Final
approach completed without another overload pause. Startup log has no prior
GameFeatureData ensure. Plain journey command line restored; long CSV override off.
Across513 sampled observations:0 logical faults,2 presentation-blocked samples,
3 crossover samples with both points locked. These are sampled observations,
not a continuous smoothness certificate.
Evidence: Evidence/Journey-Mobile-20261018-01/journey-summary.json, terminal.json,
settled.json, settled-reloaded.json, preservation-final.json.
Remaining qualification: uninterrupted frame-time/performance pass; visual capture
of actual opposing train passes; acoustic fidelity and physical touch ergonomics.
Earlier thermal status3 and frame overload remain recorded. Current scene is a
prototype, not a surveyed railway replica; generated sounds are not authenticated
WAP7 field recordings. Full Standard-mode manual journey is not yet certified.

## Small recovery UX fix (source only)
Frame-overload pauses now hide Save and exit and guard direct exit actions. Lifecycle saves retain the checkpoint with a clear restore-required status instead of attempting an invalid save. Restore remains the recovery path; no time debt is discarded. Targeted whitespace check passed with CRLF handling. Not compiled, packaged, or installed in this limited pass; Android07 remains installed. This fixes recovery messaging, not the underlying frame hitch.


## Recovery UX compilation verified
Windows Editor Development build succeeded (58.36 seconds); Android Development native build succeeded (49.14 seconds). Evidence: Evidence/RecoveryUX-Compile-20261018-01. This supersedes the uncompiled status above. Packaging, installation and interactive verification remain pending; the phone still runs Android07. No new gameplay/performance claims.


## Oct 19: cached scenery route sampling
PreparedRouteSampler validates a snapshot once per cell instead of repeating graph connection checks for every placement. Movement/authority code unchanged. All72 journey tests pass including dense forward/reverse and boundary equivalence. CPU benchmark medians: 35074.700ms baseline vs 140.381ms cached; not rendered performance evidence. Android08 packaged successfully; device installation underway, prior completed journey backed up to Journey-Mobile-20261019-01/S9-before. Standard full journey and visible-pass qualification remain pending.

## Oct19 device qualification continuation
Android08 installed in place (SHA256812f795dd61155e7e1ff451dfaf66067ef6d7c640ae453aefa72a75b1d33adc4). Prior completed result fingerprint matched; all18 older checkpoints remain unchanged.
Standard-mode manual ADB inputs exercised power, coasting, braking, background pause and exact cold recovery at tick2008. Crossover edges90000/90001 captured from cab/exterior and video; four telemetry samples show both points owned by player2/route2, maximum sampled22.423km/h.
An opposing WAP7 visibly passed the stationary player: opposing-standard08.mp4, extracted frame opposing-video54.png. One encounter verified, not allthree.
The Standard attempt advanced during an interrupted assistant turn at50% throttle and oversped. It is NOT a clean full-journey pass. Cut power, braked to standstill and saved/exited. Final tick25828, head7568.703m, overspeed ticks4139, save=world-exit verified.
Windows Shipping package Journey-Desktop-20261019-01 succeeded and includes Start Live Railway Journey.cmd; new Windows runtime validation pending. Existing realism material/audio work is retained; no new asset-authenticity claim in this pass.
Evidence: Evidence/Journey-Mobile-20261019-01/continuation-summary.json. No rendered FPS improvement claim from the CPU sampling microbenchmark.


## Oct19 Android09 Standard terminal and recovery

Android09 installed in place, SHA256 697663e48fe36dfd5f4388a8811bcba175964877620c7fa5842abf7d72470f63. Standard overspeed/restriction, terminal braking and berth/dwell advice included. All 77 journey tests pass. Desktop-20261019-02 Shipping built with live launcher; desktop runtime qualification remains pending.

Run 2 continued with injected UI inputs in Standard mode. Wireless timeout interrupted the first continuation; reconnection confirmed a background-verified pause at tick 40219/head 13602.473m. The resumed test completed at tick 63384 with berth error -1.8375m (2m tolerance), dwell 300 ticks/10s, Grade B 76.2187. Prior 4139 overspeed ticks remained unchanged. This is functional Standard-mode coverage, not clean human manual-driving qualification.

Reward saved through result UI: reputation 20, awards 2, last settled run 2. Force-stop/cold launch recovered generation 449 with identical settled logical fingerprint and no duplicate reward. All 18 earlier checkpoint hashes unchanged. No save data or simulation clock edited. Phone is at completed result, no train moving.

Evidence: Evidence/Standard-Continuation-20261019-01/continuation-summary.json; arrival09-resumed/terminal.json; standard-settled.json; standard-reloaded-confirmed.json. 486 resumed observations, zero blocked/fault samples. These observations are not rendered performance measurements. Remaining: matched frame-time comparison, remaining visible opposing encounters, desktop02 runtime qualification, scenery/audio realism.


## Oct20 measured scenery-loading fix

Android11 installed; Desktop-20261020-03 Shipping built with live launcher. Added CSV driving/loading markers and CPU scopes; analyzer has four passing tests. Finalized Android10/11 captures show maximum cell-build burst 56 to 2 and cell-building CPU maximum 136.816ms to 12.581ms. All essential cells must exist before readiness; simulation remains paused while loading.

Overall smoothness is NOT yet improved: mean frame 34.201ms to 36.087ms, p95 35.914ms to 50.053ms, >100ms frames 3 to 4, maximum frame 467.160ms to 314.831ms. Baseline thermal status 1->3 versus candidate 2->2; short initial-service sample including traffic wait. See DRIVING_PERFORMANCE.md and Evidence/DrivingPerf-20261020-02/comparison.json.

Normal launch flags restored. Completed Standard result fingerprint unchanged, reputation20/awards2; all18 older checkpoint hashes unchanged. Next: isolate remaining startup continuity cost, reduce checkpoint stalls safely, longer mobile and desktop frame-time qualification, remaining traffic visual captures, art/audio realism.
