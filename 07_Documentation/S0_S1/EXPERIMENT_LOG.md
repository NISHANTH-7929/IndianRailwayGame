# S0/S1 experiment log

S2 follow-up,20 September2026: S1 asset hashes and launch/touch/background/resume/Back smoke passed on the final S2 APK. Detailed S2 experiments and final evidence are in ../S2/EXPERIMENT_LOG.md and ../S2/S2_IMPLEMENTATION_REPORT.md. Historical S1 measurements below remain unchanged.

Status updated19 September2026. Latest APK/build identity and gate are in S1_IMPLEMENTATION_REPORT.md. A PASS is bounded to the revision and test actually observed.

## S1-E01 — Exact Android toolchain

- Question: can this installed engine support Android packaging?
- Setup: existing UE5.8.2 Blueprint project, NDKr27b, missing engine Android component.
- Change: user installed engine Android platform; install script added NDKr27c, process-local selection in build script.
- Test: Turnkey and real BuildCookRun.
- Measurement: initial UAT exit25 invalid Android_ASTC; later Android VALID r27c and overall exit0.
- Result/status: **PASS current S1 content-only toolchain**, initial attempt FAIL. No C++ game-module validation implied.
- Evidence: Evidence/turnkey-initial.log, turnkey-elevated.log, package-initial.log, ndk-install.log, package-20260919-163037.log/result JSON.
- Decision: retain installed engine/project, use SDK36/build-tools36.1.0/NDK27.2.12479018/JDK21.
- Consequence: no second project, global NDK replacement or unnecessary C++ conversion.

## S1-E02 — Minimal diagnostic fixture

- Question: can an ordinary tiny map show input/render behavior?
- Setup: empty original Content; editor Python commandlet only.
- Change: four boxes, three labels, PlayerStart; unlit materials, GameModeBase/DefaultPawn; mobile settings.
- Test: commandlet save/cook, desktop smoke, then Android screenshots and joystick swipes.
- Measurement: eight authored actors; successful save/package; initial text invisible/upside-down, corrected Android screenshot upright; move/look visible.
- Result/status: **PASS fixture/input injection on pacing revision**. Development Back exit initially FAIL; packaged fix UNVALIDATED on device.
- Evidence: Evidence/harness-map.json, create-harness.log, fix-harness-text.log, fix-harness-orientation.log, Runs/paced-input-move, paced-input-look, paced-exit and paced-relaunch.
- Decision: keep stock diagnostic pawn; use named rotation arguments and unlit font material. Back/Escape debug quit is S1-only.
- Consequence: no railway gameplay state, runtime Python, World Partition or production input framework.

## S1-E03 — Reproducible Android package

- Question: can the existing project produce an installable embedded-data APK?
- Setup: repaired E01 environment and E02 map.
- Change: ARM64, ASTC, Vulkan ES3.1, Development, API29/36; billing disabled, maps-only cook; startup CSV.
- Test: multiple BuildCookRun attempts and aapt manifest audit.
- Measurement: latest build20260919-163037 exit0,112.59s BuildCookRun,163801150-byte APK; SHA8004A806F9EA38BD46E340F4AD3EA4021B04B32F4958281E953BCC0851262739.
- Result/status: **PASS package**. Intermediate20260918-212902 overall FAIL due nested command quoting, despite inner Gradle success.
- Evidence: timestamped package logs/results, apk-badging.txt, final-AndroidManifest.xml, latest-apk-hash.json.
- Decision: invoke AutomationTool DLL through bundled dotnet so `r.SetFramePace 30` remains one quoted staged argument; keep actual result-code records.
- Consequence: repeatable content-only build path and verified packaged frame-pacer startup; installation remains separate evidence for each revision.

## S1-E04 / T01 — Install, cold/warm and offline launch

- Question: does the harness boot without network on the named phone?
- Setup: iQOO Neo9Pro/I2304, Android16, authorized USB; September18 user-enabled airplane and Wi-Fi off.
- Change: installed embedded-data APK via adb; force-stop then am start-W.
- Test: initial cold offline launch; latest install, connected cold launch, warm lifecycle, Back exit and post-exit cold relaunch.
- Measurement: initial install Success, airplane1/Wi-Fi0, COLD1100ms, LoadMap45.609ms. Latest hash installs; connected COLD988ms, WARM187ms and post-exit COLD878ms.
- Result/status: **PASS earlier T01**; **latest connected cold/warm/relaunch PASS; latest offline launch UNVALIDATED** because the phone hotspot is the user's only Codex connectivity.
- Evidence: Runs/install, offline-first, paced-install, paced-launch, paced-lifecycle, paced-exit and paced-relaunch.
- Decision: do not interrupt the user's hotspot or transfer the earlier offline PASS to the latest hash. Package-specific INTERNET app-op checks returned unknown operation and changed nothing; retest with alternate internet access.
- Consequence: overall S1 remains open even though earlier offline feasibility is demonstrated.

## S1-E05 — Touch, lifecycle and exit

- Question: do input, Home/resume and exit/relaunch work?
- Setup: exact latest hash, landscape stock virtual joysticks, mainPID24633 before exit.
- Change: synthetic Android touchscreen swipes; Home for~5s; activity launch; Android Back.
- Test: compare screenshots/process IDs and engine lifecycle log.
- Measurement: final screenshots show movement/yaw; same PID returned with WARM187ms; lifecycle events recorded; Back removed package PID and exposed launcher; relaunch COLD878ms. Audio background timeout warning observed.
- Result/status: **PASS injected input, visual lifecycle, exit and relaunch on exact latest APK; physical-finger confirmation UNVALIDATED**.
- Evidence: Runs/paced-input-move, paced-input-look, paced-lifecycle, paced-exit and paced-relaunch; commands described in reportI.
- Decision: retain Development-only Android_Back/Escape quit binding for S1; production menu/exit behavior remains future scope.
- Consequence: no production menu architecture; audio recovery remains separate/unvalidated.

## S1-E06 — Measurement and short sustained run

- Question: can actual frames/CPU/GPU/memory/thermal/log evidence be captured?
- Setup: engine startup CSV, adb meminfo/thermal/battery, app files/logcat retrieval.
- Change: corrected Saved path; CSV parser handles expanded trailing header, large startup event and rejects incomplete files; fixed sampler; replaced failed t.MaxFPS/sync-interval experiments with verified `r.SetFramePace 30`.
- Test: complete final6000-frame idle capture plus eight PSS/thermal samples through214.9s.
- Measurement: total202.225s; post10s5761frames/192.241s;29.968fps; frame p50/p95/p99 33.351/34.593/35.236ms; mean game5.041/render4.269/GPU1.597ms; zero frames>50ms; sampled PSS482425→475485KiB; thermal status mostly2, briefly3.
- Result/status: **PASS capture/retrieval and30fps pacing baseline on latest APK**. The earlier60fps and25.50fps attempts remain recorded FAIL experiments.
- Evidence: Runs/paced-sustained/Saved/Profiling/CSV/Profile(20260919_163419).csv and `.summary.json`; sample memory/thermal/battery files; paced-launch/lifecycle logs.
- Decision: use explicit platform frame pacer, measured values and raw evidence; exclude warmup and reject unfinished CSV.
- Consequence: measurement pipeline is ready, but a tiny-map baseline cannot validate final performance, low-end support or long-session thermals.
