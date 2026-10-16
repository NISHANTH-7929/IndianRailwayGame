# S7 work in progress

S7 is not yet accepted. S6 baseline is preserved under Evidence/Baseline.

Current host evidence: 45 S7 cases and 192 S2–S6 regression cases pass.
Service domain, atomic progression/save integration, authored definition class,
platform presentation, menus, driving controls and Automation wrapper are implemented.
Unreal editor build and all six Automation suites pass. S7 assets were generated. Android APK built successfully (SHA256 B7BCAE98C6F018D03C9650856C1C7713CDF3086121A53FCC4AD8105613783123, 164145118 bytes); transfer/install is running. Device validation is pending.
No S8/S9 work has been performed.

Remaining: compile and repair integration; generate S7 map/data/audio assets;
configure/package Android; test both modes, lifecycle, settlement, performance;
collect real usability observations from three casual players and three enthusiasts;
complete the required report and acceptance evidence. Human usability is not replaceable
by developer or automated testing.


2026-09-28 device progress: initial APK completed all nine objectives, berth error -0.2463 m, full dwell, grade A 93.52. Paused/background/cold recovery fingerprints match. Final UI correction APK SHA256 E83782AB65AD4C9E919452D9B297DB38774A3B143DD93D1CE3AA83260360B6E5 (164142654 bytes) transfer/install session 78164. It includes non-wrapping buttons, opaque menu background, hidden railway debug labels, and clean pause text. Final Automation 6/6 passed. Initial result is pending settlement; preserve it through install for recovery checks. Phone monitor /data/local/tmp/ir-s7-resumed-loop completed; evidence pulled to Device/resumed-loop-final. Phone uses wireless ADB endpoint. No network settings changed.

Latest continuation: gameplay/device work complete; S7 remains unaccepted pending human gates 7 and 33 (34/36). Host 46 S7 +192 earlier cases pass and strict JSON parsing now passes after correcting TestReport::Json escaping in RailKernelTests.cpp. Android pre-fix service-tests.json was malformed due embedded JSON; do not use its failed parser summary (the stale suite count was wrong). A final APK with the writer fix is packaging in exec session 86849. After packaging, install/hash-verify, launch, pull Saved/S7 reports and strictly parse all nine (238 expected), run final Automation, and audit/report hashes.
Phone currently has menu-corrected APK 3D9088F893DF9C9DC4E1791E72D3EF8399F7DC7E9A0551E48DEF479250A43B8B. Run 3 settled through repeated button input exactly once: profile reputation20, awards2, lastSettledRun3; app at Selection. Profiling override was removed. All phone-side monitors stopped/completed.
Successful fresh loop Evidence/Device/delivery-loop/analysis.json: all10 checks true, score98.534885, berth-.195349m,9 objectives,traffic175.833s,zero player lateness. Standard overshoot run2 failed without losing reputation. Pending/paid cold recovery fingerprints match. Profile captured on prior APK9C3D... before menu-only and test-output fixes:12000frames407.75s; active100..320s avg29.965FPS,5frames>50ms,0>100ms; PSS446.47..452.93MiB;thermalstatus2. Capture and summary under Device/delivery-profile/Saved/Profiling/CSV/Profile(20260928_122055).*.
Finalize-S7.py writes 25-section report,42-test mapping,34/36 acceptance (pending real participants). Run it again after final checks, then Audit-S7.py. Finalizer provenance mentions later menu and JSON output changes. Usability question reasked asynchronously; no response received. Do not claim S7 PASS or invent observations. No S8/S9 work.

Latest final verification: APK 78DD07C8540D8C42B713C54173D382497F1044D706E1FCC02BC211786476404E built, remote hash matched, installed successfully. Latest Automation completed 6/6 at 18:11 local. Final launch/report pull not confirmed; old malformed service report remains in final-S7. User closed shade, last foreground was launcher, subsequent ADB inventory empty. Reconnection requested. Replaced invalid Android summary with explicit PENDING status. Next: launch final APK, strictly parse all nine Android reports (238 cases), screenshot selection/profile20, verify override absent; then finalize and audit. Human gates7/33 remain pending.

FINAL TECHNICAL VERIFICATION COMPLETE: reconnected iQOO, cold-launched final installed APK, pulled regenerated reports. All nine strict JSON reports pass 238 cases. Final screenshot reviewed at Device/final-selection/screen.png; telemetry confirms Selection, reputation20, awards2, nextRun4, lastSettledRun3, badge unlocked. Profiling override ABSENT. Host238 and Automation6/6 pass. Only real-player acceptance gates7/33 remain pending, S7 FAIL34/36. No further technical checks outstanding; do not repeat builds or gameplay without a new reason.
