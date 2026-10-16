# S3 experiment log

## S3-E01 — Baseline and pure extension
Inspected the accepted S2 source/report and authoritative project documents before editing. Saved source/config/descriptor baseline and hashes. Added a callback to the existing accumulator; graph, history and placement algorithms remain unchanged. Added the actor-independent handling model with immutable per-vehicle data, a single assigned command source and next-step semantic commands.

Host compilation `/std:c++17 /W4 /WX /O2` passes. S3:28/28; independent S2:39/39. Render30/60/144 trace has zero measured state difference at1200 ticks; tolerance1e-9 applies only to this same-host deterministic fixture. Independent analytic acceleration error2.49e-14; constant braking stopping-distance error0. Physical-step30/60Hz stopping-distance difference0.03005872m, within the unchanged roadmap max(0.5m,1%) criterion. Evidence:host-tests.json,s2-host-tests.json.

## S3-E02 — Unreal touch integration
Added one UMG command adapter, one data asset class, and handling mode inside the existing subsystem. Initial compile rejected a local Slot name hiding UWidget::Slot and a nonexistent UButton::SetIsFocusable API. Renamed CanvasSlot and used default button focus behaviour. Corrected editor compile succeeds13.31s. Initial compiler evidence retained in editor-build-first-failed.log. Device authorized over USB; hotspot unchanged.

## S3-E03 — Widget initialization review and packaging
Both Unreal Automation suites pass (2 succeeded,0 warnings,0 failed),0.20077s test duration. Assets generated successfully. Local engine source inspection showed NativeConstruct executes after RebuildWidget has selected the root, so constructing the tree there would leave the initial Slate content empty. Moved construction to NativeOnInitialized before device installation. Final editor build succeeds62.94s; final Android BuildCookRun succeeds210.21s (package-20260920-170944). Earlier intermediate package201.76s was not installed. Host model/test source unchanged by widget correction. Original S1 five assets remain byte-identical; S2 test source unchanged.

## S3-E04 — Final package installed and Android startup tests
APK163847466 bytes,SHA256 71243974B9DC8ABD52E11DC3259558C1B694272BECED9CBE5139B3D99E1BFF0F; one-MiB transfer completed without interruption, remote hash matched and pm install returned Success. Cold launch1404ms. Both button rows and existing S2 formation/track view render.28/28 handling and39/39 kernel checks PASS on Android. Vulkan/Adreno740 confirmed; installed package has no INTERNET permission; airplane0/mobile data1, hotspot untouched. A floating timer/tap overlay was visible in initial screenshot; asked user to stop/close it before control/performance measurements. No clean performance or lifecycle PASS inferred from startup. Initial evidence:Device/final-launch.

## S3-E05 — Touch/lifecycle and same-APK regression
Owner continued after closing the overlay; screenshot confirms it is gone. All16 touch/control/lifecycle checks PASS: acceleration/coast/brake/stop, unsafe reverser rejection, reverse/reverse-stop, emergency/reset, pause/resume, background freeze, safe control cancellation, post-resume input, Back. Background state remains tick2614; mainPID20303 survives warm190ms resume; resumed tick2704 requests power0/brake1. Test runner returned1 only because successful Back makes pidof return1; fixed the script to return the aggregate assertions explicitly. Recorded results.json contains all16 true values; no APK correction required.

Automatic approval review timed out before smoke commands started; permitted retry succeeded. Same final APK S2:39/39, actor recreation PASS at tick300/head280m/eight vehicles, cold955ms. S1:touch movement visible,cold788ms,warm157ms,samePID6683,Back exittrue. Overrides restored. Evidence:Device/controls, s2-smoke, s1-smoke and s1-smoke-resumed. No app network or hotspot settings changed.

## S3-E06 — Performance capture and profiler-induced overload
Completed6000 frames/201.844045s. Excluded10s leaves5747 frames;29.95549FPS,p50/p95/p99 frame33.370/34.567/35.220ms,max48.440ms,0 steady-state frames>50ms. Mean game7.31268/render5.94507/RHI3.54268/GPU1.98013ms. PSS461341–464883KiB after startup,thermal0. Touch sequence drove forward/reverse across the grade with0 rejected commands.

At capture end, async-load request dumping/CSV finalization caused a hitch and the preserved four-step overload policy paused with debt retained. This occurs after the measured frames and must not be hidden by their healthy percentiles. Retained log and paused final sample. Decision: disable automatic profiling in normal launch, keep Build-S3 -CaptureFrames6000 opt-in, and do NOT weaken/drop overload debt. Archived profiling APK separately under08_Builds/S3_Android_Profiled with profiled-apk.json and native-library hash. Running same driving sequence without profiler while repackaging the normal default; final acceptance pending that evidence.

## S3-E07 — Normal launch package and wireless continuation
Normal launch package20260920-215959 succeeds169.33s,163847422 bytes,SHA256 A44184E24768B707971B1F9A07A66AC8300411D0FB8D68C1D7D0FD67152D20AA. Automatic profiling disabled. Native library hash remains69DC5BAB9596C29C63335A7AFED133FBED7B5D71C28327779454EF0A5BF5765C, identical to the measured profiling variant.
The first unprofiled comparison lost USB after151s and is INCOMPLETE, not a pass. On23September the owner provided a wireless ADB connection. Added identity-filtered endpoint resolution to S3 scripts, requiring exactly one connected endpoint containing the known iQOO serial. Installing the final handoff APK and repeating control/unprofiled checks; hotspot retained.

## S3-E08 — Final handoff controls
Final APK remote SHA matches and installation succeeds over wireless ADB. Removed the exact interrupted-test command-line override, normal cold launch1123ms. All16 repeated control/lifecycle assertions PASS, test runner exit0. MainPID16227 survives warm212ms resume; background tick2675 is frozen, resume tick2765 safely requests power0/brake1. Post-resume touch and Back exit PASS. Evidence:handoff-controls/results.json and handoff-controls-run.log. Final normal-launch215s run is in progress.

## S3-E09 — Final acceptance and stop
Final normal-launch215s driving run PASS over wireless ADB. Fresh log has no CSV flags, overload, fault or rejected commands. Final telemetry tick6364/head499.476597874848m/speed0/power0/brake1;18 accepted touch commands. Final APK startup28/28 handling and39/39 kernel checks PASS. Post-startup PSS455359–468586KiB. Final snapshot shows stopped eight-vehicle formation on committed curved history. Evidence:Device/handoff-sustained, handoff-steady-samples.json, acceptance-summary.json. All21 S3 gates demonstrated. S3 PASS; STOP. S4 requires owner review/approval and has not been implemented.
