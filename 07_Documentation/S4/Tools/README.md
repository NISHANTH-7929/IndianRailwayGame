# S4 reproduction tools

Run from D:\Project\IndianRailwayGame with PowerShell. UE5.8.2, MSVC14.44, Android SDK/NDK27.2.12479018 and Java21 are the existing validated toolchain. Use the currently authorized iQOO endpoint; no network settings need changing.

1. `07_Documentation/S4/Tools/Test-Operations.cmd` compiles pure code with C++17 /W4 /WX and emits operations-tests.json, s2-regression.json and s3-regression.json.
2. Compile MyProjectEditor Development using UE Build.bat and the project path. Run UnrealEditor-Cmd with Create-S4Assets.py (PythonScriptPlugin, nullrhi) only if the S4 map is absent. The script never replaces an existing map.
3. `Test-Automation.ps1` runs IndianRailway.S2.Kernel, S3.Handling and S4.Operations.
4. `Build-S4.ps1` builds/cooks/stages/packages Android ARM64/ASTC. Existing config requires Vulkan. It retains the earlier fixture maps. Profiling defaults OFF (`CaptureFrames=0`).
5. `Transfer-S4.ps1` resolves exactly one authorized endpoint matching the iQOO serial, resumes1MiB chunks, verifies whole APK SHA256 and installs with pm install -r. Do not run two transfers simultaneously.
6. `Test-S4Device.ps1 -Action Launch -Label initial-launch`, then Snapshot/Pull as needed. Start/stop affects only com.indianrailway.s1harness.
7. `Test-S4Controls.ps1` injects the bounded route/driver/lifecycle test. Current physical2800×1260 controls: first column x220, spacing393; first row y1068, spacing75. Inspect a screenshot before using on a different display. Save all intermediate states and result JSON. The final Back key intentionally closes the app.
8. `Measure-S4.ps1 -Profile -Label profiled-sustained` temporarily backs up/overrides UECommandLine.txt, captures6000frames, pauses logical simulation at host190s to avoid capture-finalization hitch entering the simulation clock, waits for export and restores the prior commandline in finally. Analyze the active capture window10–180s with `analyze_s4_csv.py capture.csv 10 180`; the complete finalized raw CSV remains intact. This excludes the deliberate end-of-capture pause.
9. `Measure-S4.ps1 -Label normal-sustained` verifies215s with default profiling OFF and no logical pause. Both measurement scripts use a finite external touch trace, not game AI.

Do not disable hotspot, Wi-Fi, mobile data or enable airplane mode. Wireless ADB is supported. If a transfer disconnects, rerun the same hash-qualified transfer; do not assume an incomplete file is installed. Actual Actor recreation evidence includes different Actor IDs and identical before/after full logical JSON, plus reconstructed signal/point presentation.

Reports do not qualify low-end Android or final-game performance. Stop after S4 acceptance; S5 is not implemented.

Manual S4 demonstration: request ROUTE A with A selected, select B and request ROUTE B to observe denial. B can use POWER100 to approach its restrictive limit and will stop under S3 braking. Select A and use POWER50 to traverse. The conservative endpoint envelope stops A around head668m while its rear still protects the junction; the HUD shows BRAKE100 and the route remains owned. After it stops, press BRAKE0 then POWER100 to move forward within its authority and clear the rear. Select B, request ROUTE B, then BRAKE0/POWER25 to proceed. JSON authority restrictions describe the provisional braking/holding permission; the finite fixture is reset using RESET TEST.
