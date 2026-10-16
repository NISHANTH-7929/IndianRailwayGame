# S6 work in progress

S6 is not accepted. No S7 systems have been added.

Read S2/S3/S4/S5 implementation reports and inspected current source/config before editing. Pre-edit Source, Config and .uproject copies and SHA256 hashes are in Evidence/Baseline and Evidence/baseline-hashes.json.

Current host evidence: S2 39, S3 28, S4 39, S5 36 pass. S6 content-policy 7, record-envelope 15, world-snapshot 19 and transaction-store 7 pass (48 foundation tests; these do not substitute for named S6-T01–T39 integration evidence).

Implemented so far:
- Pure content activation policy and explicit manifest.
- Immutable little-endian version/content/generation/length/CRC32 record; bounded reader; semantic newest-valid selection; conflicting generation rejection.
- Complete fixture runtime codec, including queued controls, AI internals, path history, formations and operations. Graph/definition identities bound to exact compiled content v1. Derived poses/occupancy/authority/signals rebuilt and validated.
- Separate recovery pause preserving queued commands and actuators.
- Alternating-slot store with readback validation and previous-valid preservation.
- Unreal streamed chunk Actor, lifecycle/save/restore integration and debug controls.
- Asset creation script (not run yet) and Unreal Automation integration.

Structural change: four pure classes grant friendship only to WorldSnapshotCodec. Public mutable state APIs are not added to those classes. Existing S2-S5 simulation algorithms are unchanged.

Build in progress. Initial Unreal compile found the engine check macro conflicts with a local test lambda named check. Renamed the S6 helper to recordCheck; rebuild still required. A previous auto-review usage-limit failure prevented one build invocation; authorized retry succeeded in launching the compiler.

Pending: finish source review and extra negative restore tests; generate actual .umap assets; build/automation; Android package/install; actual streaming/lifecycle/corruption/offline-recovery controls; repeated residency/memory/performance audit; full 39-case mapping, 30 gates and 21-section implementation report with hashes.

Device wireless ADB endpoint was connected on the latest check (transport 4). Do not disable hotspot or use airplane mode. APK must retain removal of INTERNET permission. Do not use computer-use tools for these tasks.

2026-09-27: Editor build and Unreal Automation 5/5 pass (3.296s). Two .umap chunks and persistent map authored. Android package succeeds, SHA256 67A8E5E7E221439BB6ECC9DDEB23C327CE4D3631F9E901C88BF8D9D70FC2D5B0, 164034078 bytes. Installed over USB with matching SHA and Success. Initial device controls prove real level count 2→0→2, unchanged complete fingerprint, retired actors/meshes released, moving and braking restore, offline permission absence, and Back. Two first-run assertion failures (AI retry phase, save generation advancing after successful fallback) corrected; full controls rerun is active. Sustained/profile runs remain pending.


## 2026-09-27 final APK continuation
- Final APK build 20260927-093401, SHA256 BF5173EBD75E270C42BFDF6FF043D30548F385D33BB4A67A55B815E43C319619, installed and remote hash verified.
- Final controls: 21/21 PASS (`Evidence/Device/controls-final/results.json`). Final compatibility: 10/10 PASS; original slots restored (`compatibility-final/results.json`).
- Inspected boundary-stop screenshot: all eight player bodies, both locked points and two waiting AI bodies visible.
- Host 50 S6 + 142 regression cases and Unreal Automation 5/5 remain final-source results.
- Final profile attempts disconnected at ~137 s and ~9 s. Preserved as `profile-final-disconnected` and `profile-final-disconnected-retry`; neither counts as final acceptance.
- Latest ADB state: USB 10BE1Y2EL40003J unauthorized, transport 9; no mDNS endpoint. User asked to authorize/stabilize cable or provide wireless debugging endpoint. Hotspot untouched.
- IMPORTANT: second profile override cleanup remains unverified because ADB disconnected in finally. Before next test, remove ONLY `/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/UECommandLine.txt` (it was absent before the test), record cleanup, then rerun fresh `profile-final` (245 s), analyze latest CSV 10–170 s, and run `sustained-final` (450 s).
- Added `Tools/finalize_s6_evidence.py` with prerequisite assertions and exact 39/30 requirement extraction. Run only after completed final profile/audit; expects copied `Evidence/profile-final-summary.json`. Added `REPRODUCTION.md`. Python syntax checked.
- Finish report sections 15/20, final source/hash manifests and acceptance mapping after actual final passes. S6 remains IN PROGRESS. Do not start S7.

### USB-independent measurement harness prepared
Third host-driven final profile disconnected after sample 69.47 s (`profile-final-disconnected-third`). Latest ADB inventory is empty. User asked to reconnect/authorize once to start a bounded on-device runner.
Next action: `Tools/Start-S6LocalMeasurement.ps1` once authorized. This restores the known profiling override, syntax-checks and starts `measure_s6_device.sh` with nohup. It runs guarded 245 s profile followed by 450 s sustained audit and writes `/data/local/tmp/ir-s6-final-measurement/{status.txt,FAILED,COMPLETE,runner.log}` and phase directories. It does not need USB during measurement. No instance has been started yet.
After COMPLETE, pull phase directories under Evidence/Device, run `Tools/collect_s6_device_measurement.py Evidence/Device`, analyze the new profile CSV, run the residency audit, inspect screenshots. Then run finalization scripts. Do not restart a runner if its directory/process already exists; inspect first.
The profiling override cleanup after the third disconnect remains pending and is handled at the start of this new runner. Prior first/second interrupted cleanup is verified in their commandline-restore.json files.

### On-device run STARTED at 15:44:08 IST, 2026-09-27
`Start-S6LocalMeasurement.ps1` completed successfully after user reconnected; shell syntax validation passed; nohup PID 26453. It cleared the interrupted profiling override before starting. USB disconnected again immediately afterward, but nohup allows measurement to continue independently. Do NOT launch another runner or force-stop the app. Expected total duration ~12 minutes including launches/copies. When ADB reconnects, inspect `/data/local/tmp/ir-s6-final-measurement/status.txt`, `runner.log`, `FAILED` and `COMPLETE`. Pull evidence only after phase completion. No final performance/residency verdict yet. Source-change-manifest hashes refreshed to final packaged source.

## 2026-09-27 evening continuation
Final profile completed: 245 seconds, 123 samples, 16 events, 6,000 frames. 10–170 s window: 29.9672 FPS, p95 34.555 ms, p99 35.260 ms, max 37.898 ms, 0 frames >50/100 ms. Profile override absence restored. Screenshot reviewed; complete player formation visible after demand-driven Chunk A unload.
First on-phone sustained run stopped correctly when launcher became foreground at 278 s; preserved as `Device/sustained-incomplete-foreground`. Started a fresh 450 s audit using `Start-S6LocalMeasurement.ps1 -SustainedOnly` around 20:28 IST. Remote root `/data/local/tmp/ir-s6-residency-final`; inspect status/FAILED/COMPLETE before collection. Last observed 150/450 s, no FAILED. Do not restart or force-stop while running.
Offline checkpoint inspector now built and successfully decoded moving and later checkpoints into `Evidence/moving-checkpoint-decoded.json` / `profile-checkpoint-decoded.json`; source/APK unchanged. Report and reproduction updated. Finalization scripts remain pending completed sustained result.


## S6 completed — 2026-09-27
S6 PASS. Acceptance: 30/30. Final controls 21/21, compatibility 10/10, host and packaged 192 cases each, Unreal Automation 5/5. Final frame capture and 450-second residency audit completed; all interrupted profiling overrides restored. See S6_IMPLEMENTATION_REPORT.md and Evidence/acceptance.json. Stop here. Exact next dependency: S7 — Vertical slice: complete player loop.
