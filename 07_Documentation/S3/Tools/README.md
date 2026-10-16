# Reproduce S3

Run from D:\Project\IndianRailwayGame. Requires the installed UE5.8.2 toolchain, SDK/NDK, and authorized iQOO device10BE1Y2EL40003J over USB or wireless ADB (exactly one connected endpoint). Keep the hotspot enabled. Close external tap/timer overlays; no computer-use automation is required.

1. Run `cmd /c 07_Documentation\S3\Tools\Test-Handling.cmd` and `Test-Kernel.cmd`. Results go to S3/Evidence; S2 historical evidence is not overwritten.
2. Build MyProjectEditor Win64 Development using the engine Build.bat and absolute uproject path. Run `Test-Automation.ps1` for both Unreal suites.
3. If reconstructing the assets, run the engine PythonScriptPlugin commandlet with `Create-S3Assets.py`, using `/Engine/Maps/Entry -run=pythonscript -nullrhi -unattended`. It writes only the S3 assets.
4. Run `Build-S3.ps1`, then `Transfer-S3.ps1`. Transfer resumes by SHA-qualified remote length in1MiB pieces, verifies remote SHA256 and installs with `pm install -r`.
5. Run `Test-S3Device.ps1 -Action Launch -Label final-launch`, then Snapshot/Pull. Inspect the screenshot and Saved/S3 tests before touch testing.
6. Run `Test-S3Controls.ps1`. Defaults are measured coordinates for the known2800×1260 landscape display: first button centre x220, column spacing393, rows y1153/1228. Override parameters for a different display; do not guess coordinates. It drives only the visible buttons, records19 telemetry checkpoints and16 assertions, then exits with Back. Successful Back intentionally gives adb pidof exit1; the script returns the assertion result explicitly.
7. Run `Smoke-S2.ps1` and `Smoke-S1.ps1` if checking the preserved maps. Each restores/removes only its temporary UECommandLine override in a finally block.
8. Run `Measure-S3.ps1`. It cold-launches S3, sends a finite scheduled touch sequence, records eight memory/thermal/screens/telemetry snapshots, and pulls Saved after215s. This host test is not an in-game AI driver. It must run without simultaneous phone interaction or other test scripts. The normal APK no longer starts profiling automatically. For a CSV capture, build with `Build-S3.ps1 -CaptureFrames 6000`, or supply the same CSV options in a temporary UECommandLine file as used in the archived profiling package. `Test-S3Unprofiled.ps1` explicitly excludes those flags for an uninterrupted-play comparison and restores its override afterward.
9. For a normal-play check on the handoff APK, run `Measure-S3.ps1 -Label handoff-sustained`; it records215s of driving without profiling. Require continued ticks beyond6000, a stopped final train and no overload in the fresh log. Then select the CSV created by that final launch under Device/final-sustained/Saved/Profiling/CSV. Run the existing `07_Documentation/S0_S1/Tools/analyze_csv.py <csv> 10`. It requires the finalized footer and excludes10s warmup. Retain the complete CSV, summary, commands, logs and APK hash.

Runtime Saved is `/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/MyProject/Saved`. The older Engine/Saved directory is not the current native project output.

Profiler limitation: automatic CSV finalization caused a hitch after the measured6000 frames and correctly triggered the preserved four-step overload pause. Do not mistake its pre-finalization frame statistics for an uninterrupted export. Default profiling is off in the handoff APK; RESET TEST recovers a profiling-session diagnostic pause. Do not disable the simulation's overload safeguard to hide this tool-induced hitch.

Drive manually: release brake, select power. To coast select POWER0. To stop select POWER0 and BRAKE100%. Wait for zero speed before changing reverser, then release brake and select power. Emergency latches; stop and select POWER0 before RESET E-BRAKE. Pausing/backgrounding cancels power and requests full service brake at the next active step. RESET TEST restores the finite fixture after an extent fault. No authority or automatic end protection exists in S3.

