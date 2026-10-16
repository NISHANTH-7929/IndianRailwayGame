# S1 implementation and device evidence

Updated 19 September 2026. **S1 acceptance gate: NOT COMPLETE solely because the current APK's disconnected-network launch is UNVALIDATED.** The user cannot disable the phone's hotspot without disconnecting this Codex session, so connectivity was deliberately left unchanged. The exact latest APK otherwise packages, installs, cold/warm launches, accepts injected Android touch, backgrounds/foregrounds, exits, relaunches, exposes logs/device facts and completes a paced 6,000-frame trace. An earlier APK passed airplane-mode/Wi-Fi-off launch, proving offline feasibility before later pacing/input-only revisions. No S2 implementation is authorized or present.

## A. Initial project state

See [PROJECT_INSPECTION.md](PROJECT_INSPECTION.md), written before modifications. The actual existing Blueprint-only project is `03_UnrealProject/MyProject/MyProject.uproject`, UE 5.8.2 CL56702186. Content was empty, Source absent, and the default engine OpenWorld map and Desktop/Maximum renderer settings were unsuitable for this harness. No second project was created or renamed.

## B. Changes made

Created an ordinary non-partitioned `L_AndroidHarness`: four unlit boxes, three text labels and a PlayerStart. Engine GameModeBase/DefaultPawn and stock virtual joysticks supply movement/look. The fixture is explicitly temporary diagnostic content, not railway state or production input architecture. Labels use an unlit font material and explicit keyword rotations after visual defects were found and fixed.

Selected Android Mobile/Scalable settings and an embedded-data Development ARM64 ASTC APK. Added engine CSV capture on startup and host-side ADB collection/analysis. Installed NDK r27c alongside the old NDK; user installed the missing engine Android component. Added Development-only Back/Escape `quit` debug bindings after observing unhandled Back. That last change has packaged but is **not yet device validated**.

## C. Files created

Project source assets/settings:

- `Config/DefaultDeviceProfiles.ini` under the existing project.
- `Content/IndianRailway/Validation/L_AndroidHarness.umap`.
- `Content/IndianRailway/Validation/Materials/M_S1_Floor.uasset`, `M_S1_Left.uasset`, `M_S1_Right.uasset`, `M_S1_Text.uasset`.

S0/S1 documentation and tools under `07_Documentation/S0_S1`:

- `PROJECT_INSPECTION.md`, `S0_DECISIONS.md`, this report, `EXPERIMENT_LOG.md`, `RUNBOOK.md`.
- `Baseline/`: original descriptor/Config and SHA-256 rollback records.
- `Tools/Install-S1Ndk.ps1`, `create_harness.py`, `fix_harness_text.py`, `Build-S1.ps1`, `Read-S1Device.ps1`, `Test-S1Device.ps1`, `Measure-S1Sustained.ps1`, `analyze_csv.py`.
- `Evidence/`: logs, manifests, hashes, device properties, screenshots, raw CSV and derived JSON. Evidence directories may contain prior device files pulled together; filenames/timestamps and the run references below identify the relevant capture.
- `08_Builds/S1_Android/Android_ASTC/MyProject-arm64.apk` and UAT-generated install/uninstall/symbolization helpers. `08_Builds/S1_ToolDownloads` retains the verified Android command-tools download.

Engine-generated Intermediate, Saved, DerivedDataCache and Binaries outputs are build artifacts, not new gameplay source.

## D. Files modified

Existing project `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Config/DefaultInput.ini`. The `.uproject` descriptor is unchanged. No Source module, runtime plugin or railway system was added. The authoring scripts use PythonScriptPlugin only in an editor commandlet invocation; it is not enabled in the project descriptor or used at runtime.

## E. Unreal settings

Default game/editor map points to the harness; GameModeBase supplies the diagnostic pawn. Mobile forward, Mobile HDR off, Lumen selectors off, ray tracing/proxies off, Substrate off, virtual shadows off, distance fields off, bloom/motion blur/auto exposure off. Mobile content scale factor 1.0 produced 1600×720 render resolution on the 2800×1260 device screen in the captured run. Device profile selected by engine: Android_Adreno7xx_Vulkan.

Stock virtual joysticks are always visible. Legacy PlayerInput/InputComponent are deliberately selected for the engine DefaultPawn. EnhancedInput's default plugin remains inherited and logs a user-settings warning; no production input architecture is implied. `t.MaxFPS=30` alone produced about 60fps; `rhi.SyncInterval=2` then produced an uneven 33/67ms cadence averaging25.50fps. Both observations are retained as failed experiments. The final build starts Android's frame pacer with `r.SetFramePace 30`; its log confirms the request was set to30 and the completed trace measures29.968fps with stable cadence. ConsoleVariables and device-profile duplicate t.MaxFPS produces a harmless priority warning retaining30.

## F. Android configuration

| Item | Value / evidence |
|---|---|
| Package | com.indianrailway.s1harness, display Indian Railway S1 Harness |
| Version | 0.1-S1, versionCode 1; identify revisions by SHA-256/build record |
| Toolchain | SDK platform/compile/target 36, min/NDK API29, NDK 27.2.12479018 r27c, build-tools 36.1.0, Adoptium JDK21 |
| Binary | ARM64 only, Development, debug-signed local APK, embedded data, no separate OBB download |
| Renderer | Vulkan mobile ES3.1, ASTC; GLES fallback and Vulkan SM5 disabled |
| Orientation | Landscape; display-cutout use disabled |
| Storage/logs | App external files directory; private app log path retrievable with adb; public logs off |
| Network services | Android File Server and networking disabled; Google Play/AdMob support disabled; inherited IAP disabled |
| Cook | Only harness map requested, editor content skipped, pak/IoStore/compression enabled |

`Evidence/final-AndroidManifest.xml`, `final-UECommandLine.txt` and `apk-badging.txt` were refreshed from the final paced build. Retained engine permissions: INTERNET, ACCESS_NETWORK_STATE, CHECK_LICENSE, ACCESS_WIFI_STATE, WAKE_LOCK, MODIFY_AUDIO_SETTINGS, VIBRATE and generated signature receiver permission. BILLING is absent after explicit IAP disablement. No camera, microphone, location, contacts or broad storage permission is requested. Network permissions do not prove network dependency; T01 requires an actual offline run. This is not a store/release compliance result.

## G. Build/package commands

Repeat with `Tools/Build-S1.ps1`; exact resolved arguments/environment paths and exit codes are in each timestamped result JSON. It invokes the installed engine's AutomationTool DLL directly through its bundled dotnet runtime. This preserves the quoted startup command that the batch wrapper previously split. BuildCookRun uses Android/ASTC, Development, build/cook/stage/pak/IoStore/package/archive, the one harness map, and `-csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30"`.

Latest build: **20260919-163037, exit0**, log `Evidence/package-20260919-163037.log`, result JSON alongside. BuildCookRun took112.59 seconds; AutomationTool completed successfully. APK size **163,801,150 bytes**; SHA-256 **8004A806F9EA38BD46E340F4AD3EA4021B04B32F4958281E953BCC0851262739** (`Evidence/latest-apk-hash.json`). The same hash appears in `Runs/paced-install/apk-hash.json`.

## H. Device information

User-selected **iQOO Neo 9 Pro**; adb model I2304/manufacturer vivo, serial 10BE1Y2EL40003J, Android16/API36, ARM64, SoC SM8550, Adreno740. OS-visible MemTotal 11,530,872 KiB (~11.0 GiB). `Evidence/Device/` contains the actual property, CPU, memory, SurfaceFlinger, battery and thermal captures. Advertised RAM capacity, final supported-device floor and other phones are UNVERIFIED.

## I. Test results and revision boundaries

All paths below are relative to `Evidence/Runs/`.

| Test | Observation | Status |
|---|---|---|
| First installation | September18 `install/install.txt`: streamed install Success | PASS on earlier APK |
| First cold/offline launch T01 | `offline-first/`: airplane=1, Wi-Fi=0, am start status ok/COLD, rendered map screenshot | PASS on earlier APK |
| Latest installation | `paced-install/install.txt`: Success; hash matches latest build | PASS |
| Latest connected cold launch | `paced-launch/launch.txt`: COLD/status ok/988ms; map initialized; network states airplane0, Wi-Fi0, mobile-data preference1 because phone hotspot had to remain available | PASS launch; offline UNVALIDATED |
| Upright labels / map | `paced-relaunch/screen.png` and earlier `accepted-ready/screen.png` | PASS |
| Basic touch injection | Final APK Android swipes: left420,890→420,660 in1000ms; right2400,890→2500,890 in700ms. `paced-input-move/screen.png` and `paced-input-look/screen.png` show movement/look changes | PASS synthetic Android touch; physical-finger confirmation pending |
| Background/foreground | `paced-lifecycle/`: Home, ~5s interval, WARM187ms; same main PID24633 before/after; map retained; engine lifecycle events recorded | PASS; audio warning recorded |
| Android Back exit | `paced-exit/`: no package PID after Back; launcher resumed | PASS final APK |
| Cold relaunch after exit | `paced-relaunch/launch.txt`: COLD/status ok/878ms; rendered map and logs pulled | PASS final APK |
| Short sustained baseline | `paced-sustained/`: eight PSS/thermal samples through214.9s; completed 6000-frame/202.225s CSV | PASS final APK |
| Log/device retrieval | Engine saved logs, process logcat, CSV, meminfo, thermal/battery and device files successfully pulled | PASS retrieval pipeline |

The initial snapshot command used an incorrect Saved path and failed; discovery found actual files under `/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/Engine/Saved`. Tools now use that observed path. An engine crash-handler helper shares the process name; main application PID is recorded separately in meminfo/logcat. Merely seeing that helper or a CrashReportClient config directory is not evidence of a crash.

## J. Actual measurements

**Final paced APK:** raw `paced-sustained/Saved/Profiling/CSV/Profile(20260919_163419).csv`; derived adjacent `.summary.json`. The completed capture has6000 engine frames/202.225s including startup. Excluding the first10s by cumulative FrameTime leaves5761 frames/192.241s. Nearest-rank percentiles; camera values remained fixed, confirming an idle fixture. This empty diagnostic map does not predict final-game performance.

| Counter | Mean | p50 | p95 | p99 |
|---|---:|---:|---:|---:|
| Frame interval ms | 33.369 | 33.351 | 34.593 | 35.236 |
| Game thread ms | 5.041 | 4.961 | 6.271 | 7.029 |
| Render thread ms | 4.269 | 4.306 | 4.998 | 5.397 |
| RHI thread ms | 3.127 | 3.098 | 3.882 | 4.377 |
| Engine GPUTime ms | 1.597 | 1.507 | 2.897 | 3.506 |

Average engine rate **29.968fps**; post-warmup frame range24.468–40.947ms, zero frames >50ms or >100ms. CPU/GPU lanes overlap and are not additive; render critical-path includes frame-pacer waits. GPUTime is an actual engine counter, not independently calibrated hardware timing. Engine CPU-temperature counter ranged46.3–62.2°C; thermal status was2 for most samples and briefly3. Do not equate vendor sensors with case temperature or infer equilibrium from3.4 minutes.

Final-run PSS samples fell from482,425 to475,485KiB (approximately471.1 to464.3MiB); sampled maximum482,425KiB is not a proven process-lifetime peak. Engine PhysicalUsedMB mean597.25 and p95605.63 are a different counter and must not be substituted for PSS. Thermalservice repeatedly returned a cached37.7°C battery sensor; cached readings may be stale. Long-session throttling, power and battery drain remain UNVALIDATED.

Earlier offline activity-display time was **1100ms** (`offline-first/launch.txt`), with engine LoadMap45.609ms. Final paced APK cold display was988ms; post-exit cold relaunch878ms; lifecycle warm return187ms. These Android activity times are not interactive-ready or total engine/map latency. No exact touch-to-ready measurement is claimed.

No fatal UE crash/ANR was observed in the retrieved test windows. This does not prove crash-free behavior over unobserved periods or after connection loss.

## K. Errors encountered

1. Initial sandbox RunUAT could not write required AppData caches; elevated retry resolved access.
2. Missing engine Android target made Android_ASTC invalid (initial UAT exit25). Initial Turnkey exit0 had skipped useful platform validation, so was not treated as a PASS.
3. NDK r27b did not satisfy exact installed engine requirements.
4. Stock lit text was invisible in the unlit fixture; positional rotation then produced upside-down text.
5. Initial t.MaxFPS setting yielded ~60fps on device instead of30.
6. Nested RunUAT quoting split the intended pacing command; package operation succeeded but overall UAT exited1 with `Failed to find command 30`. That build is a FAIL, not a success.
7. Initial log-pull path was wrong; CSV reader also needed a larger field limit and support for UE's expanded header at the end. Incomplete traces are now rejected.
8. Sustained collector had a PowerShell interpolation parse error (`$index:`); corrected to `${index}:` before reuse. The failed invocation collected no sustained series.
9. Back did not exit the stock harness; the Development-only binding was added and subsequently passed on the final APK.
10. Device became offline. Automatic approval review subsequently could not execute one adb reconnect attempt because its service stream disconnected; this was not an unsafe-action determination.
11. A package-only offline alternative was inspected after reconnection, but this Android build reports `Unknown operation string` for both `android:internet` and `INTERNET`. No app-op or network state was changed; background-data restriction was rejected as inadequate for a foreground cold-launch test.

## L. Fixes applied

Installed correct platform/NDK without creating another project; process-local toolchain settings; authoring material/rotation correction; corrected Saved path and duplicate-process handling; robust complete-CSV parsing; collector syntax fix; Development-only exit binding. Replaced the failed sync-interval experiment with a verified Android frame-pacer command. Build-S1 calls AutomationTool directly so nested quoting is preserved; device log and final trace verify the result.

## M. Remaining problems / warnings

- The current APK's disconnected-network launch is UNVALIDATED because the phone provides the user's Codex hotspot and connectivity could not be interrupted. Earlier APK offline launch passed; later changes are pacing command and Development exit binding, but that does not permit silently transferring the PASS.
- The phone exposes no package-specific INTERNET app-op through `cmd appops`; the available alternatives would either leave foreground networking available or interrupt the user's hotspot.
- Physical-finger confirmation remains pending; Android-injected touchscreen input passes on the exact APK.
- Backgrounding logged `AudioMixerPlatformInterface Timeout [5 Seconds] waiting for h/w`; visual lifecycle recovered. No audio fixture is present, so audio recovery is UNVALIDATED.
- Engine logs include disabled GooglePlay subsystem, EnhancedInput settings mismatch with intentional legacy input, Vulkan surface image-count adjustment, and OEM logcat/SELinux optimization warnings. No observed visual failure was attributed to these; do not call logs warning-free.
- Gradle8.13 deprecation notices are nonfatal at the tested version; no Gradle9 compatibility claim. Host cook low-memory GC warnings do not measure Android memory.
- Engine-default modules/content remain in the prebuilt target/cook. A tiny map does not imply a minimal custom engine binary; no speculative plugin pruning or C++ rebuild was attempted.

## N. S1 acceptance checklist

| Required gate | Current status |
|---|---|
| 1 Existing project packages | PASS, latest UAT exit0 |
| 2 Latest packaged app installs on named phone | PASS |
| 3 Latest app launches reliably | PASS cold, warm and post-exit cold relaunch |
| 4 Latest app launches without network | UNVALIDATED latest; September18 earlier T01 PASS |
| 5 Basic input works | PASS final APK injected Android touchscreen; physical finger pending |
| 6 Background/foreground works | PASS final APK; audio timeout warning retained |
| 7 Logs retrieved | PASS pipeline |
| 8 Device/build information recorded | PASS |
| 9 Baseline trace captured | PASS final paced6000-frame trace plus214.9s sampled run |
| 10 No unresolved S1 toolchain incompatibility | PASS current successful content-only build |

**Overall: NOT COMPLETE because item4 remains UNVALIDATED.** All other required S1 gates pass on the exact latest APK. Do not advance to S2 without completing/explicitly resolving that gate and receiving user approval.

## O. Evidence for each PASS/FAIL

Build: `Evidence/package-20260919-163037-result.json` and `.log`; APK identity `latest-apk-hash.json` and `Runs/paced-install/apk-hash.json`. Original packaging failure: `package-initial.log`; quoting failure `package-20260918-212902-result.json` and `.log`. Toolchain repair: `ndk-install.log` and successful build's Android-valid line.

Device/renderer: `Evidence/Device/device.json`, `meminfo.txt`, `surfaceflinger.txt`; run logs contain engine version, GPU, RHI, resolution and profile. Install/launch/input/lifecycle/back: exact run paths in I. Trace/counter methodology and PSS: paths in J. `Evidence/SourceManifest.json` records current canonical source hashes when generated; `Baseline/SHA256.json` preserves original hashes.

## P. Every UNVALIDATED item

Current APK offline cold launch/repeatability; physical-finger confirmation; thermal equilibrium/throttling/power/battery drain; exact first-interactive-ready latency; audio recovery; process-death/state restoration; supported low-end/other GPU devices; GLES fallback; Shipping/AAB/store compliance; native project C++ compiler; final-game performance/memory and all railway behavior. Several items are explicitly outside S1 rather than hidden S1 successes.

## Q. Recommended next step

When an alternate internet connection is available, keep Codex reachable through it, enable airplane mode/Wi-Fi off on the iQOO, and repeat cold launch on hash8004A806… using RUNBOOK.md. If that cannot be scheduled, the user must explicitly decide whether the earlier offline PASS plus review of the later pacing/input-only delta is acceptable project evidence; it remains UNVALIDATED here. Then explicitly approve S1 and the next scope. Before S2, confirm SI-to-Unreal conversion, actor-independent simulation ownership, coordinate/direction/ID conventions and the intended device-floor experiment. Do not implement track/formation/train/signalling systems during this follow-up.
