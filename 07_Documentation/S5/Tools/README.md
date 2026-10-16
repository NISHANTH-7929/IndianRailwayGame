# S5 reproduction

Run from D:/Project/IndianRailwayGame using PowerShell. The existing UE5.8, VS2022 and Android SDK/NDK environment is required.

1. Run Tools/Test-AI.cmd for the pure 36-case S5 suite and full 39/28/39 regressions.
2. Build MyProjectEditor Win64 Development, then run Tools/Test-Automation.ps1. Four Unreal suites must pass.
3. Run Create-S5Assets.py through UnrealEditor-Cmd with PythonScriptPlugin only if reconstructing the S5 map.
4. Run Tools/Build-S5.ps1; final APK is 08_Builds/S5_Android/Android_ASTC/MyProject-arm64.apk.
5. Resolve exactly one authorized iQOO endpoint with Resolve-S5Device.ps1. Transfer-S5.ps1 supports resumable, hash-verified APK installation.
6. Test-S5Device.ps1 -Action Launch cold launches. Keep the game foreground and stop any external auto-clicker. Never change hotspot/network settings.
7. Inspect a screenshot before choosing touch coordinates. Test-S5Controls.ps1 uses a finite touch script with a foreground guard and preserves snapshots. It drives only A; B is autonomous. Default coordinates are for the validated landscape device layout and must be verified after UI changes.
8. Measure-S5.ps1 -Profile -Label profile-final captures 6000 CSV frames; logical simulation pauses at host190s before export. Analyze only active10-180s. Measure-S5.ps1 -Label normal-final runs215s with default profiling off and no simulation pause.
9. analyze_s5_csv.py reads the finalized CSV and measures standard engine counters plus RailS5 AI/step timings. Summarize-Performance.ps1 joins PSS/thermal/validity samples. Preserve all raw outputs and any failed runs.
10. Finalize cross-platform-tests.json, source-change/hashes, report and Audit-S5.ps1 only after the actual Android evidence passes. Do not mark missing evidence as passed.

Player scenario: reset grants A only through the queued S4 request. Select B to read its denial/state; driving controls for B are rejected. Select A, release brake, apply50% power. A conservatively stops with the rear still protecting the route. Release brake and apply100% power to complete rear clearance. B's next periodic request then grants and it moves/stops autonomously. B retains its uncleared route at the bounded terminal. Reset restarts the fixture.

S5 ends after validation. S6 and S7 are not part of this work.
