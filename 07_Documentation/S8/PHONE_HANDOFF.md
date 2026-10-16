# Phone / desktop handoff — 9 October 2026

## Latest: audio sequence3 PASS; phone restored and idle

`oct10-audio-sequence3` completed on installed APK20261010-082112: horn playback,
paused silence, resumed rail/station loops and emergency-brake audio all pass via
observed-layout UI and current engine logs. All eight checks pass; 24 samples.
All original settings/telemetry restored, normal saves unchanged, app stopped and
APK hashes unchanged. This supersedes the foreground blocker below. No running
phone task. Listening quality/full background lifecycle remain unaccepted.

After-brake screenshot reveals HUD inconsistency: cab T000/B100/EMG, but upper
HUD Power35%/Brake0%. Investigate commanded-versus-applied/emergency presentation;
do not infer simulation failure. Fine cab labels remain indistinct. Next interior
implementation remains bounded station night lighting, not exterior expansion.

## Current phone idle: audio sequence needs stable foreground

Supersedes retry-pending details below. Wi-Fi reconnected; controls2 launched but
expired its manual-action deadline, with complete restoration. A bounded sequence
now replaces that manual handoff, guarded by observed layout/focus/state checks.
67 local tool tests pass. Sequence1/2 did not reach running readiness and sent no
touches; sequence2's current log shows Android suspension and world exit at tick0.
Both restored original settings/telemetry, preserved normal saves and APK, and
stopped the app. No active phone runner remains. Next attempt requires phone kept
unlocked and game foreground; do not weaken readiness or claim audio failure.
Horn/pause/resume remain scoped partial evidence from controls1; brake acceptance
and audible-quality review remain open. Latest immutable APK remains unchanged.

## Latest 10 October: compatibility passed; audio interaction retry pending

Current installed candidate remains APK20261010-082112 (hash below).
Clean serial `oct10-combined-legacy-recovery2` passes all three exact paused
recoveries, all preservation/restoration checks, plus 263 fresh startup tests in
ten groups. Egmore completed Standard recovery also passes (generation150).
63 local device-tool tests pass.

`oct10-audio-controls1` has observed horn playback, paused silence and resumed
rail/station loops, but expired waiting for its next action. All settings/saves
restored and app stopped. Late action3 was not executed; brake check is open.
`oct10-audio-controls2` failed read-only preflight before creating evidence or
mutating the phone because Wi-Fi disconnected; may reuse this absent label.
ADB briefly reappeared after user reconnect, then disappeared again.

Unobscured cab capture confirms small panel lettering is still indistinct.
Do not mark visual acceptance complete. See NEXT_INTERIOR_GATE.md for Astra's
bounded runtime night-lighting audit. No outside-polish work is authorized here.

Excluded runs: smoke2 interrupted by owner overlap with legacy-recovery1; both
excluded from completed acceptance, original idle settings subsequently restored
with smoke2/restoration-recovery.json PASS. recovery2 is the clean replacement.

## Current 10 October: combined build installed; verification in progress

This entry supersedes ownership/build status in the historical entries below.
Parent owns ADB and packaging; the other desktop chat is idle after its quota.
APK `S8_EgmoreService_20261010-082112/Android_ASTC/MyProject-arm64.apk`
is installed with SHA256 `8889bf472b52ed3a8bf675d2c7e706e18b92a41dcdc1aa40a2eee171e79bd4cb`.
Install receipt `oct10-cab-audio-install1` preserves normal saves/settings.
Includes cab-label material contrast revision and newer Pixabay audio, with
cab geometry, controls, camera and S7 unchanged; 16 desktop suites pass.

`oct10-cab-audio-smoke1` passes all five audio asset loads and reported rail/station
loop playback, with all restoration/preservation checks passing. Its screenshots
are obscured by an unrelated notification and excluded from visual acceptance.
Fresh capture follows user-confirmed dismissal. No audible-quality claim.

`oct10-combined-egmore-recovery1` PASSES exact paused recovery of completed
Standard run2, generation150/fingerprint9ac1afc6e0f58b73, tick11652, GradeB/86.4194,
unsettled, with profile retained. Scene/signals/locomotives rebuilt; all ten
checkpoint paths and settings preserved. S8 remains incomplete: final visual,
audio interaction/listening, environment and sustained performance gates remain.

## 10 October build takeover

Desktop integration chat hit a usage limit after completing cab-label contrast
revision and 16 passing suites in Evidence/Cab/LabelReadability-20261010.
No editor/build process remained. Parent now temporarily owns packaging and ADB;
do not start concurrent canonical edits/builds. Combined source checks preserve
cab geometry/control/camera/S7 and verify newer audio hashes. New immutable
EgmoreService build is packaging; 20261009-144106 remains installed until a new
exact-hash install receipt passes. Lettering visual acceptance remains open.

## Latest 10 October: Standard resumed itinerary passed; phone idle

`oct10-egmore-standard-itinerary2` PASSES all seven phases with scripted normal UI
inputs: mode1, completed tick11652, dwell300, berth error-1.404824m, zero overspeed
or harsh ticks, GradeB/86.4194, all nine objectives1. Resumed existing checkpoint;
not a fresh uninterrupted journey/human usability test. Reward not settled.
184 ready samples; all normal paths/settings restored, wake0, app stopped. Parent
has no running phone process and awaits the new immutable cab/audio candidate.

## 10 October current ownership

Parent owns ADB exclusively, running `oct10-egmore-standard-itinerary2` against
installed APK20261009-144106. Standard UI controls and paused cold recovery already
pass (`oct09-egmore-standard-controls1`, `oct09-egmore-standard-recovery1`).
Recovery generation113/fingerprint4700e721fe5c3ce0, mode1/tick2725/profile retained.
Full Standard itinerary still under test; first attempt intentionally stopped to
correct an overly conservative test-driver berthing rule, all restoration passed.
No gameplay/source change was needed for that test-driver fix.

`Create reference-based railway props` now owns canonical desktop cab-label
readability improvements and combined new immutable Android packaging, preserving
the newer desktop Pixabay audio work. It must not use ADB; parent does not edit
canonical assets/C++ or run concurrent builds. Latest installed APK has no new
audio integration; do not attribute newer desktop results to it.

## CURRENT: interruption restored; reward persistence verified; phone idle

settlement1 restoration-recovery.json PASSES: normal saves unchanged, original
override/telemetry and wake0 restored, app stopped. Its isolated service had
completed and checkpointed generation97 while ADB was disconnected; original
interrupted run remains excluded as an uninterrupted itinerary.
`oct09-egmore-finish-reward1` recovered that completed checkpoint exactly and
saved its earned reward via one observed-layout UI touch: generation98,
fingerprint dcdde9a4ed3db7b7, one award, reputation10, mastery unlocked.
`oct09-egmore-settled-recovery1` then cold-restored that exact settled state with
all ten checkpoint paths unchanged and no extra reward. All restoration checks
pass, app stopped, wake0. Installed immutable APK remains 20261009-144106.
No phone/build process is running. Historical reconnect warnings below resolved.
Remaining gates include Standard driving, fine cab-label/interior visual work,
night/rain/audio integration, sustained combined Android performance and release
catalog/provenance closure. Full S8 is NOT complete.

## ACTION FIRST ON RECONNECT: restore interrupted settlement test

`oct09-egmore-settlement1` lost Wi-Fi ADB at last observed tick 7947 / Departed.
The Depart touch was accepted and dwell completed, but no settlement touch was
sent. This run is excluded from completion/reward acceptance. Automatic cleanup
could not reach the phone: temporary wake3 and isolated launch override may
remain active. Do not begin another test before restoration.

Run `restore_egmore_interrupted.py --evidence
07_Documentation/S8/Workstreams/StationArt/Device/oct09-egmore-settlement1`
with the verified current endpoint. It retains interrupted evidence, stops the
game, restores original commandline/telemetry/wake0, and checks all eight normal
checkpoint paths without overwriting them. Backups and before.json are retained.
The advertised endpoint 10.35.89.252:41279 actively refused reconnection.
Installed build was verified 20261009-144106 before this test; previous successful
phone recovery results below remain valid. Reward settlement is still open.

## CURRENT verified phone state (supersedes historical disconnect notes)

20261009-144106 installed successfully over Wi-Fi; exact SHA256 below verified.
Before install, the failed USB attempt was checked: previous APK remained and
all normal saves, override and wake matched its stopped backup. Install3 passed
all preservation checks. Egmore isolated cold recovery now PASSES exact state,
generation/fingerprint, paused readiness, scene/signals/both locomotive recreation,
and preservation of all ten normal+isolated checkpoint paths. Three older-service
recoveries also pass on this build; all 263 fresh startup cases pass.
Evidence: `oct09-egmore-resume-install3`, `oct09-egmore-phone-recovery1`,
`oct09-resume-build-legacy-recovery1` under StationArt/Device.
Parent currently owns phone for `oct09-egmore-settlement1` isolated reward test.
Do not run another phone process concurrently. Final S8 acceptance remains open.

## Latest candidate: isolated-resume build ready, phone disconnected

Latest reconnect attempt: USB transfer failed in
`Workstreams/StationArt/Device/oct09-egmore-resume-install2`; phone went offline.
Stopped-save backups exist; no uninstall/data clear or launch/wake write occurred.
Installed APK and preservation are currently UNVERIFIED, not confirmed unchanged.
On reconnect compare preflight with that run's before-stopped.json and original
commandline before retrying installation. See its REVIEW.md. ADB reconnect-offline
did not restore a usable transport; Wireless debugging endpoint requested.

New APK: `D:/Project/IndianRailwayGame/08_Builds/S8_EgmoreService_20261009-144106/Android_ASTC/MyProject-arm64.apk`
SHA256: `92c96bdd077445973a7d05fcfefb5a82444f8924a11a8b52dd2fdba89fd2f5f9`
302123854 bytes, read-only archive; build/cook/package succeeded in 206.07 seconds.
NOT INSTALLED: installer preflight found no phone; no device mutation occurred.
The installed build remains 20261009-121852 below (last verified before disconnect).

New development-only `-S8AssetReviewResume` requires `-S8Egmore -S8AssetReview`.
It restores only isolated checkpoints and remains paused; missing/corrupt saves
fail closed without starting or overwriting checkpoint files. Fourteen desktop
suites and 50 mocked device-tool tests pass. Desktop runtime resume, missing and
corrupt cases pass with all eight normal paths preserved and the complete
original isolated folder restored. Evidence: StationArt/Device/
`oct09-egmore-isolated-resume1` and `oct09-egmore-isolated-runtime2`.

Next phone step after verifying serial 10BE1Y2EL40003J: install this exact APK via
`device_install.py`, then `device_egmore_recovery.py --apk <above> --label <unique>`.
The recovery runner uses the phone's existing isolated last-checkpoint receipt,
requires two paused matching samples, verifies scene/signals/both locomotives,
preserves all ten normal+isolated checkpoint paths, and restores settings.
Do NOT launch a fresh review between capturing that reference and recovery.
Phone cold recovery, settlement and final S8 acceptance remain unverified.

## CURRENT: playable Egmore candidate installed; acceptance incomplete

APK: `D:/Project/IndianRailwayGame/08_Builds/S8_EgmoreService_20261009-121852/Android_ASTC/MyProject-arm64.apk`
SHA256: `5126ed14ce2d04cd9349e923aa1e099ee4cd0efab2ac6513b331199f7fd6be84`
302126190 bytes. Build succeeded. Exact install hash and unchanged normal saves,
commandline and wake verified.90-second Android readiness smoke passes with both
locomotives,39 static modules/2594 placements and2 ID-bound signal housings ready.
Map `/Game/IndianRailway/S8/Egmore/L_RailService_Egmore`; flags
`-S8Egmore -S8Station -S8LHB`. Test isolation adds `-S8AssetReview`.

13 desktop Automation suites pass;43 mocked device safety tests and34 standalone
identity/half-step/queued-control checks pass. Isolated phone itinerary2 now passes
all7 phases with actual Depart touch,10-second dwell and gradeA/98.58. Reward not
settled; full S8 acceptance remains open. All save/settings restoration passes.
Itinerary1 disconnected and is excluded; restoration-recovery.json now passes.
Follow-up: all three legacy/LHB/station cold recoveries pass on this same APK,
with exact fingerprints and 263 fresh startup checks across ten groups passing.
The 60-second isolated moving cab smoke passes with 13 ready samples; main
gauges are readable but fine dark-panel labels remain unaccepted. Both runs
preserved all eight normal save paths, restored settings and stopped the app.
Evidence: `oct09-playable-legacy-recovery1` and `oct09-egmore-cab-smoke1`.
Egmore-specific cold recovery and reward settlement are not yet phone-validated.
Parent chat owns ADB and temporarily owns canonical work
because integration chat/subagent hit usage limits. Do not launch parallel tests.

Evidence: `Workstreams/StationArt/Device/oct09-egmore-playable-install1`,
`oct09-egmore-playable-smoke1`, `oct09-egmore-assisted-itinerary1/2`.
Normal checkpoints are never overwritten. Previous diagnostic build below is
historical; do not reinstall it as the newest candidate. Full S8 remains open.

## READY: immutable Scene02 diagnostic candidate

APK: `D:/Project/IndianRailwayGame/08_Builds/S8_EgmoreScene02_20261008-143136/Android_ASTC/MyProject-arm64.apk`

SHA256: `2b52b9f0b321173106e1dfe39db410cf3f2451b9d5debee62ef7e0da676018b7`

302055762 bytes; build/cook/package exit0; APK set read-only. Desktop verification:
11/11 Automation, fresh reload of40 modules/120 LODs,1,316 unique placements,
six-view runtime tour passed. No local checkpoints were created (none existed).
Evidence: `Evidence/EgmoreScene02/android-handoff.json`, `UnrealAutomationFinal/`,
`runtime-tour-result.json`, `source-runtime-validation.json`, `DESKTOP_REVIEW.md`.

Map: `/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study`

Flags: `-S8EgmoreCameraTour -ExecCmds="r.SetFramePace 30"`.
Tour cycles every12seconds; individual view: `-S8EgmoreView=0..5` without tour.
0 platform,1 waiting/exit sign,2 stair approach,3 footbridge,4 WAP7 cab,5 exterior.
Expected log marker `S8_EGMORE_SCENE02_REVIEW`, identity Station.Egmore.Scene.02,
interior2.1.0,40 modules/1316 placements; telemetry
`Saved/S8/EgmoreScene02/Review/telemetry.json`. staticArtReview=true, gameplayBound=false.

This is ready for **diagnostic phone review**, not visual acceptance. Nameboards
read correctly on both faces; stair camera no longer intersects steps; cab exterior
visibility is corrected. Canopy shadows/fine cab labels remain dark offscreen.
Please distinguish actual mobile rendering from that unresolved desktop issue.
No new service/checkpoint execution, night/weather/audio or full-S8 claim.

Original integration chat has no running phone process. APK/build output above is
frozen for testing; subsequent desktop work will use a different build archive.

---

Historical handoff updates below; the ready candidate above supersedes their
"not ready" status.

## Ownership

Station chat owns ADB, installation, phone capture/recovery/performance testing.
Original integration chat owns canonical assets, Unreal, builds and desktop tests.
No phone commands or device test processes are running from the integration chat.
Do not install the rolling output while a desktop package is being built.

## Immutable verified baseline (do not repeat passed cab tests)

APK: `D:/Project/IndianRailwayGame/08_Builds/S8_CabInstruments_20261007_164056/Android_ASTC/MyProject-arm64.apk`

SHA256: `2b75ffb51eeeaa3ec163ce37b34db3c7852b5b7da1cb9b21cb6a13c02f61d33d`

290633062 bytes. Archived and hash-checked. Includes cab refinement and old V22
station, **not Scene02**. Passed day/night cab, three cold recoveries, 263 Android
cases and one WAP7/LHB station workload (29.965 FPS mean, 34.696ms p95 frame,
20.977ms mean GPU; thermal status 2). Six saves and commandline restored; wake=2.

Evidence: `Evidence/Cab/InstrumentRefinement-20261007/`,
`Evidence/Station/cab-instruments-oct07-wrapper/result.json`,
`Evidence/Station/Device/cab-instruments-oct07-wap7-lhb-station/`.

## Next candidate — not ready for phone

Base Scene02 imported: 35 modules, three LODs, 1,259 placements, 25 materials.
New isolated native presentation actor compiled successfully. Desktop Automation
and assembled train/platform review underway. Additive interior v2.1.0 next.
No Scene02 Android APK published yet. Await an immutable path/hash here.

Preserve original saves, commandline and temporary USB wake setting. Do not mark
full S8 complete from the cab baseline or static art preview alone.

Desktop update: first assembled view exposed mirrored base nameboards; correcting
canonical export UVs and mesh-letter handedness, preserving sealed source. Existing
BeforeLetteringCorrection screenshots are diagnostic, not a passing candidate.
Interior import saved all five modules but editor crashed on commandlet shutdown;
will reload/verify before using them. New six-view runtime tour will use
-S8EgmoreCameraTour or -S8EgmoreView=0..5, exact map to be recorded with APK.
Phone chat's 15/15 manual touches passed on the baseline; see its Device review.

Both base nameboard faces now read correctly in desktop captures. Stair view moved
before the first tread (49m); waiting view at30m faces the exit sign/approach.
Six runtime views: 0 platform, 1 waiting/wayfinding, 2 stair approach, 3 footbridge,
4 WAP7 driver, 5 train exterior. Expected telemetry 1316 placements/40 modules;
Saved/S8/EgmoreScene02/Review/telemetry.json, log S8_EGMORE_SCENE02_REVIEW.
Exact map /Game/IndianRailway/S8/Review/L_EgmoreScene02_Study. Static review only,
no service/checkpoint execution. Final daylight lighting verification underway.

Build 20261008-143136 is packaging into a NEW timestamped archive. Not ready until
its final receipt/hash is published here. All11 final Unreal Automation suites pass;
six-view NullRHI runtime smoke passed, no local checkpoint files created (none
existed in that desktop Saved directory). Visual issue: canopy shading remains dark;
this will be labeled diagnostic, not visually accepted. Cab shell visibility fixed.

Post-archive desktop progress: candidate spatial contract tests pass on all four
station track alignments using the full8-vehicle WAP7/LHB formation;12/12 suites
now pass in UnrealAutomationSpatialContract. This is later source, NOT the frozen
APK. Reserved IR-S8-EGMORE-1 remains rejected until operations/service/recovery are
implemented consistently. No fake legacy-service relabeling. Actual desktop player
capture at Evidence/EgmoreScene02/desktop-player-waiting.png is brighter/cleaner than
offscreen SceneCapture exports; final visual acceptance remains open.

Phone review acknowledged: all6views/restoration passed, visual acceptance false.
Canonical next-source fixes: remove incompatible SkyAtmosphere actor and use the
already-authored mobile sky dome (no warning suppression); allow camera aspect to
match device. Suspected distant thin-structure LOD loss still under diagnosis.
Pure Egmore service configuration boundary now compiles; testing full completion,
cold save/restore, legacy incompatibility and old regressions before engine binding.
No second APK yet. Frozen20261008-143136 bytes remain untouched.

## Desktop candidate in progress — 10 October

Canonical desktop work resumed in integration chat. Cab label contrast comparison
and combined audio validation are in Evidence/Cab/LabelReadability-20261010;
DESKTOP_CAB_AUDIO_20261010.md records scope. No ADB run here. Existing installed
APK20261009-144106 remains the phone baseline until a NEW immutable APK receipt
is published. Audio originals/prepared hashes and all cab geometry/control poses
are unchanged. Current change is two lettering materials, revision
cab-label-contrast-20261010. Small print is not yet declared phone-readable.
