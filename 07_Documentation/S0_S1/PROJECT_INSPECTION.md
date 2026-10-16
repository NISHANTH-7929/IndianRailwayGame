# Initial project inspection — before implementation

Recorded 18 September 2026. Scope: S0/S1 only.

- Actual project: `D:/Project/IndianRailwayGame/03_UnrealProject/MyProject/MyProject.uproject`.
- Engine installation inspected: `C:/Program Files/Epic Games/UE_5.8`; Build.version confirms 5.8.2, changelist 56702186.
- Project directories: Config, Content, DerivedDataCache, Intermediate, Saved. No Source directory; no declared C++ module. No project plugin directory.
- Content contains no files; no project maps or production assets were found. Default map is `/Engine/Maps/Templates/OpenWorld`.
- Config files: DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini, empty DefaultEditor.ini.
- Hardware target: Desktop / Maximum. Renderer configuration enables virtual shadows, distance fields, Lumen selectors, ray tracing and Substrate. Windows RHI DX12 is duplicated. These are unsuitable inherited defaults for a minimal Android baseline.
- Input: Enhanced Input defaults, stock virtual joystick, mouse capture; no project input implementation.
- Explicit plugin: ModelingToolsEditorMode, editor-only. Android File Server settings allow networking; implicit engine-default plugins have not yet been audited by a package manifest.
- Project-specific Android architecture, SDK versions and packaging settings are absent. No previous packaged Android build was found in the project inspection.
- SDK exists at ANDROID_HOME after elevated access. Installed platforms: 33, 34, 35, 36, 36.1. Build tools: 33.0.1, 35.0.0, 36.0.0, 36.1.0. Installed NDK: 27.1.12297006 (r27b).
- Installed engine Android_SDK.json requires minimum/main NDK r27c and recommends 27.2.12479018; it lists platform 36/build-tools 36.0.0. Public Epic 5.8 documentation instead lists recommended SDK 35/build-tools 35.0.1; local engine requirements take precedence for this exact build, subject to packaging verification.
- JAVA_HOME points to Adoptium JDK 21.0.12.8. Android Studio exists. Actual Unreal toolchain acceptance remains UNVERIFIED until Turnkey/build evaluation.
- `adb devices -l` executed successfully outside the sandbox and returned no devices. Phone model, Android version, RAM, CPU, GPU: UNVERIFIED. Physical-device tests: UNVALIDATED.
- Engine Android platform DLLs, Android runtime binaries and Engine/Build/Android were not found at expected paths. Installed-platform configuration refers to missing Engine/Binaries/Android/UnrealGame.target. Platform installation is a likely packaging blocker, to be verified by an actual package attempt.
- vswhere reports Visual Studio 2019 Build Tools only. Engine Windows_SDK.json requires VS2022 17.8+ or VS2026; any C++ editor module requires a compatible compiler installation. No C++ conversion is assumed necessary until the harness strategy is selected.
- No Epic Games Launcher app was returned by the initial computer-use app inventory. Installation discovery continues; absence from that inventory alone is not proof it is uninstalled.

## Preservation

Before project changes, Config and MyProject.uproject were copied to `Baseline/`, with SHA-256 records in `Baseline/SHA256.json`. Content was empty and no source existed. Generated cache/log folders are not copied as canonical project source. Existing caches/content will not be deleted. Engine version and SDK requirements are copied into Evidence.

## Immediate blockers and next checks

Verify platform/toolchain using the engine build tools; resolve NDK mismatch if possible; locate supported engine platform installation method. Prepare a minimal ordinary map and measurement-only harness in this project. Do not mark S1 passed without installation, offline/lifecycle/input and actual trace evidence on a named device. S2 remains prohibited.
