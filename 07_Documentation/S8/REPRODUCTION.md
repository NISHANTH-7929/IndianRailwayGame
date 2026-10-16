# S8 foundation reproduction

Run from D:/Project/IndianRailwayGame using the installed Python, MSVC 2022, UE 5.8
and Android SDK/NDK configured by earlier stages. Do not rerun create_contracts.py or
write_standards.py over future reviewed content: they author initial foundation defaults.

1. `python 07_Documentation/S8/Tools/validate_s8.py --output 07_Documentation/S8/Evidence/validation-results.json`
2. `python 07_Documentation/S8/Tools/test_s8.py`
3. `cmd /c 07_Documentation\S8\Tools\Test-Regression.cmd`
4. Build MyProjectEditor Win64 Development with the existing engine Build.bat; use
   `-Project=D:/Project/IndianRailwayGame/03_UnrealProject/MyProject/MyProject.uproject`.
5. `07_Documentation/S8/Tools/Test-Automation.ps1` (seven suites).
6. `07_Documentation/S8/Tools/Build-S8.ps1` (validation precedes ARM64 ASTC packaging).
7. Connect/authorize the named iQOO. Preserve its existing Saved/S7 checkpoints before
   install. `07_Documentation/S8/Tools/Transfer-S8.ps1` verifies SHA256 and installs -r.
8. `07_Documentation/S8/Tools/Device-S8.ps1 -Action Launch -Label <unique-label>`.
   Use Observe-S8.ps1 for guarded snapshots/touches, and preserve Saved/S8/foundation-runtime.json
   plus the fresh S7 regression reports. Compare paused fingerprints across update/relaunch.
9. With S7 already paused, `07_Documentation/S8/Tools/Profile-S8.ps1 -Label <unique-label>`
   captures a 6000-frame paused-fixture harness smoke, samples PSS/thermal/streaming,
   normalizes the CSV and restores the previous commandline. It sends no driving touches.
   Use the separate representative workload procedure for content-cost comparison.
10. Run Collect-FinalDevice.ps1 to recheck normal launch and restored state after profiling,
    then `python 07_Documentation/S8/Tools/verify_android.py`. Preserve package/permission
    dump, screenshot and runtime reports. Do not remove or rewrite user checkpoints.
11. `python 07_Documentation/S8/Tools/audit_s8.py`, then `write_report.py`, then `audit_s8.py`
    again to include the report in evidence hashes. Android verification is based on actual
    raw reports; never mark its result passed based solely on package build or prior S7 logs.

New content record example (data only):

`python 07_Documentation/S8/Tools/instantiate_content.py Loco.Example.01 locomotive 07_Documentation/S8/Reviews/Loco.Example.01 --second Loco.Example.02`

Final release review uses `validate_s8.py --release`; it must fail on this intentionally
empty foundation. Future review evidence is stored under Reviews/<AssetId>/readiness.json.
Keep failed/incomplete attempts; use distinct device labels instead of overwriting evidence.
