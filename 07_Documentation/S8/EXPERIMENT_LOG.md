# S8 experiment record

- 2026-09-29: read S2-S7 reports and inspected Source/Content/Config/stage tools.
  Preserved baseline hashes before edits. S7 human gates 7 and 33 remain pending.
- Initial authoring script failed syntax parsing (unclosed audio profile expression).
  Corrected before generation; no partial data generation from that failed run.
- Python lacked optional jsonschema. Implemented a limited, fail-closed standard-library
  evaluator for the exact checked-in schema keywords; no package installation required.
- Sandbox launch of engine build did not complete. Reviewed external tool/cache access
  allowed the actual editor build, which succeeded. Android device inventory was empty.
- Host foundation tests 23/23 and prior pure regressions 238/238 passed. Unreal Automation
  passed seven suites with zero test failures/warnings. Android packaging succeeded.
- 2026-09-30 continuation: iQOO I2304/API36 available over wireless ADB. Pre-install Saved
  data copied successfully (91 files). The S8 copy of the inherited pull helper then
  raised a null-PID error because the app was stopped. Installation had not run. Fixed
  only the S8 helper and separately transferred/verified/installed the foundation APK.
- First launch passed packaged S8 checks and restored S7 fingerprint 0c71336bb96fdebf,
  tick 5385, reputation 20 and two awards. Screenshot shows the preserved pause menu
  and an external floating overlay. No overlay-based or human usability claim is made.
- APK permission inspection confirms no INTERNET permission; no phone network setting
  changed. Profile smoke uses the paused service to test the harness without changing
  the saved logical state. Its observations cannot establish active simulation cost.
- A direct aapt redirection invocation failed before capture; a successful invocation
  subsequently produced Evidence/apk-permissions.txt. Failed invocation is not evidence
  of APK permissions.

Final outcomes are generated from raw data in android-verification.json and test-mapping.json.
No prior-stage gameplay issue was silently repaired. No production content was acquired.

Final device outcome: 6000-frame paused capture completed; 15 state/memory/thermal samples
retained the same logical fingerprint. Temporary commandline was restored to absent.
Normal cold launch reran 238 S2-S7 cases and S8 packaged checks successfully. Final paused
state/progression matches pre-install. All 22 Android evidence checks passed. Simulation
and service cost counters are unavailable for this paused capture, not treated as zero.
