# S1 reproduction, remaining tests and rollback

Workspace: `D:\Project\IndianRailwayGame`. Run PowerShell commands from this folder. Unreal/Android tools require access to their normal SDK/AppData caches. Phone serial is10BE1Y2EL40003J; change the script parameter deliberately for another device. Do not replace the named-device result with an emulator result.

## Build

```powershell
& ./07_Documentation/S0_S1/Tools/Build-S1.ps1
```

Require overall exit0 and inspect its timestamped result JSON. A Gradle success inside a failed AutomationTool run is insufficient. Output is `08_Builds/S1_Android/Android_ASTC/MyProject-arm64.apk`. Hash every revision; versionCode remains1 for this local harness. No production signing credentials or store publishing are configured.

The build script selects SDK/NDK in process scope only. If required engine Android components are missing, install the Android platform component for this existing UE5.8 installation. The NDK installer downloads verified official command-tools into the workspace and installs27.2.12479018 alongside existing versions; do not repeatedly reinstall a working toolchain.

## Authoring, only if recreating an absent harness

Do not rerun authoring on an existing map; it deliberately refuses overwrite. Use `UnrealEditor-Cmd.exe` with the existing project, `-EnablePlugins=PythonScriptPlugin -run=pythonscript -script=<absolute script> -unattended -nullrhi -nosound -nop4 -stdout -abslog=<absolute log>`.

Run `create_harness.py`, then `fix_harness_text.py` for its unlit font/explicit rotations. The second script can update the existing fixture; neither creates a new project. Never author assets while a cook is reading them. These are editor-only tools and do not enable runtime Python.

## Finish the latest-device gate

1. Connect/unlock/authorize the iQOO. `adb devices -l` must say `device`, not offline/unauthorized. If phone setup needs changing, tell the user; no GUI automation is required.
2. Install latest hash with a fresh evidence label:

```powershell
& ./07_Documentation/S0_S1/Tools/Test-S1Device.ps1 -Action Install -Label accepted-install
```

3. Ask user to enable airplane mode and disable Wi-Fi; retain USB. Verify actual `airplane=1` and `wifi=0` evidence at launch. Mobile-data preference can remain1 while airplane mode disables cellular; it alone is not active connectivity evidence. Read active connectivity if ambiguous. Do not silently change the user's network preferences.
4. Launch and immediately start the controlled sampler. Leave the foreground app idle, screen unlocked, with no input or lifecycle actions until capture finishes. Record power/battery/thermal conditions; this phone has reported external power as AC.

```powershell
& ./07_Documentation/S0_S1/Tools/Test-S1Device.ps1 -Action Launch -Label accepted-offline
& ./07_Documentation/S0_S1/Tools/Measure-S1Sustained.ps1 -DurationSeconds 210 -Label accepted-sustained
& ./07_Documentation/S0_S1/Tools/Test-S1Device.ps1 -Action Snapshot -Label accepted-sustained-end
```

The APK starts6000-frame capture automatically (roughly200s at30fps). Allow completion; if runtime is slower, collect longer before pulling. Sampling is every~30s, and captures elapsed wall time/process IDs. Check every adb command for failure before treating a file as a measurement. `Pull` retrieves the app's entire Saved folder, including older captures; select the CSV timestamp matching this launch. Do not count stale files as new results.

5. Analyze only a completed CSV, confirmed by `Frames:6000`/capture-ended log and trailing metadata. Use the installed engine Python executable:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe' ./07_Documentation/S0_S1/Tools/analyze_csv.py '<absolute CSV path>' 10
```

It excludes the first10s by cumulative FrameTime and emits nearest-rank percentiles, mean CPU/GPU, counts and observed camera values to adjacent JSON. Check total frame count/duration against the engine log. Zero-only GPU counters are UNVALIDATED. CPU/Game/Render/GPU costs overlap. PSS comes from dumpsys, not the engine PhysicalUsedMB column. Thermalservice cached temperatures are not automatically fresh sensor samples.

6. Ask user to drag left/right sticks physically and confirm movement/look. Also capture before/after evidence if using Android `input touchscreen swipe`; label that synthetic rather than physical. Earlier synthetic native-screen coordinates: left420,890→420,660 in1000ms; right2400,890→2500,890 in700ms. Re-read screenshot dimensions/locations if the display changes.
7. Use `-Action Background`, wait~5s, then `-Action Resume`, `Snapshot`, `Pull` with a new label. Verify same mainPID, map/control response, and background/foreground events. Record audio/device warnings without declaring all lifecycle behavior clean.
8. Use `-Action Back` to test the new Development quit binding. Capture the old process log and current resumed activity/PID. Require actual exit evidence; a command exit0 is not proof the app exited. Then `-Action Launch` with a new label to test relaunch. Repeat cold offline launch at least once on the same hash. If Back remains unhandled, diagnose actual input mapping rather than substituting force-stop and calling it graceful exit.
9. Capture final logcat/engine logs; inspect crash/ANR and warning evidence. Re-audit current APK manifest, hash and canonical source hashes. Update report/experiment statuses only from actual evidence. The latest APK must pass all ten S1 gates before closing S1. Stop before S2 and request the user's explicit stage approval.
10. Tell the user the offline test is finished so they can restore their preferred connectivity. No automatic global network changes are made by these scripts.

## Rollback

Original source was empty Content plus descriptor/Config. `Baseline/SHA256.json` identifies the pre-change files. With Unreal/build processes closed, copy the original `Baseline/Config` files over corresponding project Config files and restore `Baseline/MyProject.uproject` if it changed. The descriptor currently matches its baseline.

Move only the newly added `Content/IndianRailway/Validation` fixture and `Config/DefaultDeviceProfiles.ini` to a separate rollback archive if reverting. Verify exact resolved paths stay inside this project before moving/removing anything. Do not recursively remove all Content, Saved, Intermediate, the SDK or engine. Existing caches were not part of canonical backup. Keep reports/evidence and original baseline for audit. An installed NDK alongside other versions does not require uninstalling for project rollback.

To remove the test APK later, explicitly uninstall only `com.indianrailway.s1harness` with user authorization; uninstall also removes its device-side files, so pull evidence first. No other phone app or data is in scope.
