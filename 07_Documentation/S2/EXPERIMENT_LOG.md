# S2 experiments

Final gate,20 September2026: **S2 PASS** on APK D11DF26857979E4CAA18EEC7B2E42CFAEAC13797B2F147CF4D05DFE5EC76333B. Earlier failures below are historical; corrected results are E07/E08.

## S2-E01 — Pure coordinate/history kernel
Question: can a181m eight-vehicle formation follow explicit branch history forward and backward independently of Actors and render FPS?
Fixture:7 nodes/7 edges/14 transitions; straight, four radius40m arcs, .02dz/ds grade. Change branch choice after head enters upper route. Reverse along recorded occurrences.
Result:39/39 standalone tests PASS. Exact numerical errors: Evidence/host-tests.json. Arc tolerance .01m and bogie tolerance .05m unchanged. Invalid topology/vehicle dimensions and forbidden reverse traversal reject. Fixed30Hz matches30/60/144 render schedules, including100s tick-scripted reversal. Compiler /W4 /WX.
Decision: proceed to Unreal/device integration; pure results do not prove rendering or Android.

## S2-E02 — Native toolchain
Initial:content-only S1; VS2019 insufficient for UE5.8. Installed signed Microsoft VS2022 BuildTools compiler19.44.35229. First editor build failed missing NetFxSDK. SDK installer1602/5007 failures traced to required elevation. Elevated installation succeeded and registry confirms4.8SDK. Editor compile retry in progress.
Evidence:editor-build.log; installer logs under Windows TEMP. Do not claim package success until completed.

## S2-E03 — Unreal and Android integration
Editor build exposed a collision between the standalone helper name `check` and Unreal's macro. Renamed helper and corrected float literal warnings. Editor build PASS. Asset creation PASS. Unreal Automation:1 suite,39 kernel assertions,0 failures/0 warnings. Android Clang compilation and BuildCookRun PASS,269.37s,163794442-byte APK. Both fixture maps cooked. Evidence:editor-build.log, asset-creation.log, UnrealAutomation/index.json, package-20260919-193646.log.

## S2-E04 — Device transfer and offline strategy
Phone authorized before transfer; streaming installation caused transport to become offline, including after user reconnection. Non-streamed install could not begin on offline transport. Reconnect command did not restore a usable connection. Installation/runtime checks UNVALIDATED; requested another cable/USB port and unlocked phone. Evidence:Device/first/commands.jsonl, install.txt, install-retry.txt. No hotspot/network settings changed.
UPL removes app INTERNET permission to allow a future network-isolated launch while retaining the workstation hotspot. Intermediate manifest confirms removal; installed manifest and launch still unvalidated. No device performance or visual success inferred from build success.

## S2-E05 — Resumable transfer and on-device proofs
One-MiB chunks completed the initial APK transfer; local/device SHA256 matched C4D7717DCCB288C424A4C36BACBB0825B95D2F2C8C7E3455C1440BFDFFE4ED60. Package install Success, cold launch1103ms. All39 kernel tests PASS on Android. Actual view recreation at tick300 preserved head280.00000000000142m and8 vehicles. Installed package permission dump contains no INTERNET; network-isolated app launched with phone mobile data1 and airplane0. Hotspot untouched. Evidence:chunked-install.log, Device/verified/S2, launch.txt, package.txt.

## S2-E06 — Visual/lifecycle failures and corrections
Initial screenshot showed track/text but black vehicle meshes. Device log identified missing InstancedStaticMeshes usage on the borrowed S1 materials. Created dedicated S2 copies with usage enabled and changed the view references; original S1 asset hashes unchanged. Background/foreground logs show suspended time caused a fixed-step overload pause. Added lifecycle callbacks that exclude background elapsed time while preserving active-time overload checks. Initial failure evidence retained in Device/verified/Logs and screen.png.
Corrected package20260920-081350 builds successfully in230.07s; APK163801846 bytes. Corrected transfer paused after45088768 bytes because device disappeared. Hash-qualified remote filenames prevent resuming against a different APK. Rebuilt device visual/lifecycle checks, sustained timing, and S1 device smoke remain pending.

## S2-E07 — Corrected final device acceptance
Resumed transfer through USB interruption and host ADB timeout; final SHA256 matched and install Success. Final build renders bodies/bogies and follows the upper path forwards/backwards. All39 Android tests PASS. Real view recreation PASS at tick300. Background/resume keeps PID28534, warm157ms, continuing tick1149 without overload. INTERNET absent from installed manifest; cold launch1068ms with airplane0/mobile data1. Evidence:final-install.log, Device/final-launch, lifecycle-before/after.

## S2-E08 — Sustained performance and S1 regression
Final6000-frame capture complete201.818132s; excluded10s leaves5748 frames. Mean29.964fps, frame p50/p95/p99=33.372/34.720/35.459ms,0 frames>50ms. Mean game6.227/render5.799/RHI3.563/GPU1.808ms. Post-startup PSS487510–493421KiB, thermal1. No overload or missing material-usage warning in final log. Added fixture/debug work costs more than the S1 baseline but retains30fps pacing; differing thermal conditions and this tiny fixture preclude production-performance claims.
S1 original5 asset hashes unchanged. Same final APK: S1 cold807ms, injected touch response, same PID8894 after warm155ms resume, Back removes processes. Temporary command-line override removed; S2 restored (cold693ms) and screenshot verified. Evidence:final-performance.json, final-memory-thermal.json, Device/final-sustained, Device/s1-smoke*, Device/final-restored. Decision: S2 acceptance PASS; STOP before S3.
