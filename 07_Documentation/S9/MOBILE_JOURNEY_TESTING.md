# Android live journey qualification

Authorized scope: WAP-7/LHB, representative Egmore-Tambaram live journey,
Assisted and Standard modes. Connected I2304, Android 16, ARM64/Vulkan/ASTC.

## Evidence and preservation

`Evidence/Journey-Mobile-20261018-01/Saved-before` contains all 243 existing
saved files (338671321 bytes). `original-commandline.txt` preserves the old
station-service launch override. Update installation must retain app data;
do not uninstall or clear storage. The new live journey has its own S9 save
directory; there was no S9 directory on this device before this test.

The first Android compile found the service's main header was not first.
The second found misleading indentation in corridor telemetry. Both were
corrected without changing simulation behavior. Failed build logs are retained.

## Required device checks

- Package and install, then verify the running candidate and live-service ID.
- Selection, briefing, both mode choices, formation confirmation, start.
- Touch power/brake/emergency/reset, horn, cab camera and cab controls.
- Sound sliders, muted/resumed playback and saved volume preference.
- Pause, Home/background and return, save/exit, cold restart with exact recovery.
- Protected priority wait, locked crossover and opposing trains.
- Station progression, scenery residency and completed Tambaram stop/dwell.
- One saved reward and identical reward/profile after cold restart.
- Sustained frame timing, memory, temperature and error/crash inspection.
- Compare the preserved older checkpoint hashes after qualification.

Each item requires device evidence. Desktop and host tests do not substitute for
phone interaction or mobile rendering/performance. Startup compilation success
does not establish a playable mobile build. Synthetic supplemental sound and
representative route geometry retain the desktop limitations.

## Current status

Android candidate 03 is cooking after successful compilation. Device tests are
pending installation. No mobile acceptance or performance claim yet.

## Device progress
Candidate03 installed successfully using non-streaming ADB after wireless reconnect
interrupted the streamed attempt. All18 older checkpoint hashes unchanged.
Selection, Assisted/Standard toggle, formation, start, emergency/reset, brake
release, power, camera, HUD, wiper and sound slider inputs exercised on device.
Save/exit and restart preserved tick2991 fingerprint exactly. Home/background
saved tick3978 and returned paused. Later interruption saved tick14711; resumed.
Crossover samples8189/8342/8522 confirm both points locked for player2.
Full journey and terminal recovery still pending. Engine CSV60000-frame capture
is running; it includes menus, background and driving, not an uninterrupted run.
Thermal status reached3 in early run, later2. No smooth-performance claim.
Startup ensure traced to GameFeatureData scan: removing entry broke editor cook.
Candidate05 successfully builds with bIsEditorOnly=True; pending device update.


## Oct 18 continuation: Android 07
Candidate06 failed with an Android Clang compiler crash (null-character diagnostics);
unchanged source rebuilt successfully as candidate07. Includes active-driving
keep-awake and editor-only GameFeatureData scan. Installation pending.
At tick42663 candidate03 paused on fixed-step overload. Background save was
correctly rejected because retained time debt exceeds checkpoint limits. Preserved
all S9 files in Evidence/Journey-Mobile-20261018-01/S9-before-fix, restored via UI
and saved/exited successfully at tick42594, generation166, fingerprint
22b4169c9d5fbdf8. Lost69 ticks (2.3 seconds), not the journey.
Restored plain journey launch arguments (removed long CSV capture override).

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
