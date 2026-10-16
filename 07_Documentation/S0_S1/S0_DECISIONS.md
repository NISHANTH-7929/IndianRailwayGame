# S0 decisions and experiment register

Status: updated 19 September 2026 with actual package/device observations. See PROJECT_INSPECTION.md for the unchanged pre-modification record, and S1_IMPLEMENTATION_REPORT.md for the final gate. No railway implementation is in scope.

## Locked for this stage

- Keep the existing MyProject name and project; no second project or broad restructuring.
- Implement only the Android measurement harness. No track, train, signal, AI, station, progression or simulation manager.
- Initial target: Development diagnostic package, ARM64, landscape, paced 30 fps baseline; actual performance UNVERIFIED.
- Prefer mobile forward with conventional rendering, no Lumen/ray tracing/virtual shadows; exact RHI/device compatibility remains experimental.
- Use only a tiny ordinary non-World-Partition map; installed core harness has no online requirement.
- Preserve project settings with hashes before modifications. Record every modified file and build attempt.
- Future simulation convention remains SI units with explicit Unreal centimeter conversion; actor-independent railway state. Neither is implemented during S1.
- Measurement statuses distinguish observed PASS/FAIL from UNVALIDATED. No baseline number predicts final-game performance.

## Verified environment

Engine: UE 5.8.2 CL56702186. At inspection: Desktop/Maximum, empty Content, no Source/module, implicit packaging defaults, missing Android engine components and NDK r27b below the engine minimum. Explicit project plugin remains editor ModelingToolsEditorMode; the descriptor is unchanged. User installed engine Android support. NDK 27.2.12479018 (r27c) was installed alongside the prior version. Real BuildCookRun then validated Android and packaged successfully using SDK platform 36, build-tools 36.1.0 and JDK 21.0.12.8. Build-S1.ps1 selects NDK paths only for its process. VS2019 BuildTools exists; S1 uses the engine's prebuilt UnrealGame target, not a newly compiled game C++ module.

First test device: user-selected iQOO Neo 9 Pro. Actual adb model I2304, manufacturer vivo, Android 16/API36, ARM64, SoC SM8550, GPU Adreno 740 (SurfaceFlinger). OS MemTotal 11,530,872 KiB (~11.0 GiB); advertised physical RAM capacity is not inferred. Serial 10BE1Y2EL40003J identifies this test unit. Evidence: Device/device.json, meminfo.txt, cpuinfo.txt and surfaceflinger.txt. Initial adb absence/offline was resolved by connecting and authorizing the phone. The September 18 first installation launched offline with airplane=1 and Wi-Fi=0. Final revised-build tests are separately recorded; earlier evidence does not silently validate a later APK.

Current S1 rendering/package decisions: Mobile/Scalable, mobile forward Vulkan ES3.1, ASTC, ARM64 only, min API29/target API36, landscape, Development APK with data inside, no Android File Server or required online service. Lumen, ray tracing, Substrate, virtual shadows, distance fields, bloom and motion blur are disabled. The map uses four unlit boxes, three labels and one PlayerStart; the engine DefaultPawn and virtual joysticks provide diagnostic movement only. No World Partition or railway authority state exists.

30fps is the initial diagnostic target, not an approved production guarantee. Actual experiments found t.MaxFPS alone near60fps and a sync-interval attempt averaging25.50fps with repeated67ms intervals. Both failed. The final package uses Android's explicit `r.SetFramePace 30`; its phone log confirms set-as30 and the completed idle trace measures29.968fps, p9534.593ms and no frames above50ms after warmup. This3.4-minute tiny-map result does not establish thermal equilibrium, low-end compatibility or final-game performance.

## Experiments

| ID | Question | Prerequisite / initial state |
|---|---|---|
| S1-E01 | Does exact installed engine/toolchain support Android packaging? | Run Turnkey and real BuildCookRun; missing platform/NDK suspected |
| S1-E02 | Can a minimal ordinary map and engineering controls be authored/validated? | Same project; no production content |
| S1-E03 | Can the project cook/package a Development ARM64 harness? | Correct platform binaries/toolchain and E02 |
| S1-E04 / T01 | Can named phone install, cold/warm launch and run offline? | Actual device and packaged APK required |
| S1-E05 | Do touch, exit/relaunch and background/foreground work? | E04 plus device evidence |
| S1-E06 | Can logs, device facts and sustained baseline timings/memory be captured? | E04; GPU timing only if actually available |

## Hypotheses / unknowns

Final supported device floor/reference matrix, other GPU families, long-session thermal behavior, production memory budget and representative-scene30fps remain UNVERIFIED. The exact current APK's offline launch is also UNVALIDATED because this phone supplies the user's Codex hotspot; an earlier revision passed airplane1/Wi-Fi0. This single high-end phone cannot define a low-end support floor. Public docs and local SDK metadata differ; preserve both observations and use actual build validation. No S1 toolchain mismatch remains. Native C++ compilation for a future stage is UNVALIDATED and requires checking the then-required compiler rather than assuming the content-only result covers it.

Permissions were inspected from the packaged manifest. Engine defaults retain INTERNET, ACCESS_NETWORK_STATE, CHECK_LICENSE, ACCESS_WIFI_STATE, WAKE_LOCK, MODIFY_AUDIO_SETTINGS and VIBRATE, plus the generated receiver signature permission. No camera, microphone, contacts, location or broad storage permission is requested. Inherited billing was explicitly disabled. Retained network permission does not establish a network requirement; T01 tests operation with connectivity absent. Release permission minimization/store compliance is a later review, not an S1 PASS.
