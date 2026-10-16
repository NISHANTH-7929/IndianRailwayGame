# Egmore Android review — 2026-10-04

This is a static architectural review, not a finished Egmore route or a WAP-7/LHB gameplay performance acceptance.

## Measured build boundary

Build `20261004-133207`, `08_Builds/S8_EgmoreReview/Android_ASTC/MyProject-arm64.apk`, 280540450 bytes, SHA-256 `eb466af3e962ce5542c68d2b41518e9dcf42144d0b1eb3556f8d487ef9764e92`. Packaging and streamed installation succeeded. The separate archive preserves the earlier Station V6 APK. Only the isolated Egmore review map was added to the cooked map list; the normal station remains the default launch.

Source revision: twelve Egmore modules including ballast, 80123 source triangles; imported LOD totals 79845 / 43913 / 19956. All twelve portable GLBs passed CLI validation and all twelve 1.1.0 canonical packages passed verification. This validates packaging, not visual realism.

## Actual device evidence

`Evidence/Egmore/Device/review-20261004-133855` contains three device screenshots, memory samples, engine log, result and restoration receipts. The engine loaded `/Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study`. All six normal checkpoint file hashes were unchanged during this isolated capture. The saved command line was restored afterwards.

The phone image exposed a blank platform-facing facade, excessive empty ground, weak material response and mismatched camera framing. Visual acceptance remains false. No FPS claim is derived from these static captures.

## Regression and recovery

`Evidence/Egmore/AutomationReview/index.json`: ten automation tests successful, zero failures; one successful test carried an unrelated HTTP timeout warning. This run covers the existing S2–S7 regression suite and station automation.

`Evidence/Station/egmore-review-apk-recovery-matrix.json`: all three real-device cold recovery cases passed identity, logical fingerprint, service-state, paused/unblocked/valid and presentation recreation checks:

| Content | Fingerprint | Tick |
|---|---|---:|
| Legacy fixture | `0efccd7c539ec58a` | 6477 |
| LHB | `ff7e0afb56b48956` | 10236 |
| Station + LHB | `2c55c3509c5a4284` | 516 |

Station actors, LHB actors and expected proxy count also passed. Recovery may advance checkpoint generation while preserving logical state; generation is not used as a gameplay-state equality assertion. These receipts are separate from preserved V6 results.

## Follow-up source revision

V17 adds estimated platform-facing joinery/cornices to the arcade and tower and sets the review camera to 58 degrees without forced aspect bars. The pre-change source, import receipt, map and corridor preview are in `Evidence/Egmore/BeforeRearV17`. These newer changes require their own render/build evidence and must not be attributed to the APK hash above.

Remaining work includes architectural proportions and route layout, multilingual signage, material weathering, surrounding terrain, signals and OHE details, and a reproducible Egmore train workload. No railway simulation algorithms, old content IDs or legacy fixture definitions changed in this art pass.

## V17 device verification

Build `20261004-140638` succeeded and installed: 281671626 bytes, SHA-256 `770bfec68c6471bd34ecaf6b61868b82f7b827184078342ff2d99e7a0ad4bd35`. The preceding APK is retained under `08_Builds/S8_EgmoreReview_20261004_133207`.

`Evidence/Egmore/Device/review-20261004-142240` contains three actual phone screenshots showing rear joinery and corrected full-width framing. Capture and command-line restoration succeeded; all normal checkpoint hashes were unchanged. `Evidence/Station/egmore-rear-v17-recovery-matrix.json` passes all three recovery cases with the same logical fingerprints and service states listed above. Station generation is now 37; tick remains 516. The S7 map hash remains `748ACD747B5E696E23531B7686932A83381A5DCA39B738C9B229B24A6B175D5F`.

Twelve V17 modules pass source/import checks: 109994 source triangles, imported LOD totals 109702 / 60334 / 27420. Twelve GLBs in `EgmorePortable/v3-rear` pass independent validation, and twelve version 1.2.0 packages pass integrity verification. The added rear details increased geometry; these counts are not an Android performance acceptance.

Visual review still finds overly uniform lighting, a pale track bed and insufficient surrounding detail. V18 starts a separately backed-up aggregate material experiment with 1024px cellular stone colour, normal and roughness maps. This experiment is not in the V17 APK or its measurements.

## V18 aggregate and ground candidate

The material experiment keeps all mesh geometry metadata and non-stone material
definitions unchanged (aggregate-v18-invariance.json). EgmoreStone now has
1024px maps; other materials retain their previous resolutions. Twelve portable
1.3.0 packages pass the declared validation policy and integrity verification.
This is not a claim of photorealism or measured mobile performance.

The isolated review map adds a soil yard surface below the ballast and extends
five previously floating mast foundations down to the ground. The platform's
existing foundation supports the remaining mast. The editor-only capture actor
is created after the map save and is not packaged. A trial with continuous
capture disabled changed exposure history and produced a darker screenshot;
that diagnostic was preserved and continuous capture restored in the script.

## V18 measurement helper repair

The first actual-train capture `egmore-v18-apk-wap7-lhb-station` was excluded:
a live telemetry read returned empty JSON after three valid samples. The game
writes telemetry.json directly via SaveStringToFile (RailContinuity.cpp), so a
read can overlap truncation/write. All normal save hashes were unchanged and
the command line was restored after the failure.

profile_lhb.py now retries JSON parse failures at most five times, 200 ms apart,
retaining rejected payloads. It does not reuse stale telemetry or mask ADB
failures/persistent corruption. A focused check verified transient empty/partial
JSON recovery and rejection of persistent invalid JSON. No packaged runtime
code changed. The fresh retry has a separate evidence directory.

## V18 actual device results

Build `20261004-143844` succeeded: 282934662 bytes, SHA-256
`f034d4c7c95cfdf60d828919c6491b3f4300e6972ad83df8edf3b822e28eb387`.
Installed and captured under `Evidence/Egmore/Device/review-20261004-144530`.
Three screenshots show the actual phone view; the last includes an Android
usage-time overlay. All normal save hashes were unchanged, command line restored.
The yard remains an abrupt rectangular study surface, and lighting/materials
still need refinement. Visual acceptance remains false.

The actual WAP-7 + LHB + generic station arrangement A workload completed in
`Evidence/Station/Device/egmore-v18-apk-wap7-lhb-station-retry` on this APK.
It is NOT an Egmore gameplay benchmark. 6000 finalized frames; analysis window
10–170 seconds, 4794 frames, 30 FPS cap, one run:

| Metric | Result |
|---|---:|
| Mean FPS | 29.9634 |
| Mean frame time | 33.3741 ms |
| p95 / p99 frame time | 34.588 / 35.250 ms |
| Mean / p95 GPU time | 21.2047 / 25.698 ms |
| Mean sampled PSS | 602.42 MiB |
| Maximum sampled PSS | 612.95 MiB |
| Reported thermal status | 0 in all 14 samples |

CPU/GPU lanes overlap; do not sum them. PSS samples span the capture rather than
only the frame analysis window. Reported load latency repeats persistent chunk
telemetry and is not 28 independent loading trials. No final performance budget
or repeated-campaign claim. No same-APK baseline comparison was collected.

The current Android AssetReview directory contains ten passed regression groups,
263 total cases: kernel 39, handling 28, operations 39, AI 36, checkpoint 9,
content 7, record 15, world snapshot 19, service 46, LHB 25. Summary receipt is
`android-regression-summary.json`. Normal checkpoint hashes were unchanged.

Initial V18 cold recovery was interrupted: Android exit-info reports REMOVE TASK,
not a recorded native-crash reason for that exit. Preserved under
`Evidence/Egmore/RecoveryV18Failure`; the incomplete recovery attempt is excluded.

Final V18 recovery retry passed for all three saved content modes, including
station/LHB presentation recreation: `Evidence/Station/egmore-yard-v18-retry-recovery-matrix.json`.
Legacy fingerprint `0efccd7c539ec58a`, LHB `ff7e0afb56b48956`, station
`2c55c3509c5a4284` unchanged. Final station save generation 39, tick 516, paused.
No new algorithm or content-identity changes were required. All failed attempts
remain preserved and excluded from success/performance claims.

## V19 validated on 2026-10-05

Build 20261005-084049 succeeded; installed APK is 282957982 bytes, SHA-256
`75d4bb03715a2e25c996ed84234da534bab02baaf702a76e95d2254b9861db44`.
Actual capture: `Evidence/Egmore/Device/review-20261005-084351`. Ground scale,
maintenance walkway/railings and cabinet foundations render on the phone.
All normal checkpoint hashes unchanged during capture; command line restored.
Desktop and phone views were inspected. Mobile lighting remains noticeably
flatter, so visual acceptance remains false. No V19 FPS/GPU benchmark claim.

`Evidence/Egmore/egmore-ground-v19-device-validation/result.json` confirms the
complete device workflow and wake restoration 2 -> 3 -> 2. This handles the
phone reporting AC charging while on wired USB. No lock bypass was used.

`Evidence/Station/egmore-ground-v19-recovery-matrix.json`: all three content
recovery cases pass, including visual recreation. Final station generation 47,
tick 516, paused; all three logical fingerprints and service states preserved.
The final station's Android regression summary confirms 10 groups / 263 cases
passed. S7 map SHA remains unchanged from the recorded repaired baseline.
Previous APKs, failed material API attempts, and the rejected 0.8 fill-light
render are preserved. Final fill is 1.6; all material connections are checked.

## V20 Android build and visual capture — 2026-10-05

Build 20261005-091435 completed with exit 0; APK 289,940,602 bytes,
SHA-256 c5a35bd33d6663d0069bb00e9fdc68ae61579444e73fe817766676e7351eb185.
V19 artifact remains archived. The new APK is 6,982,620 bytes larger.

Actual phone capture: Evidence/Egmore/Device/review-20261005-091651.
091713.png was inspected: new nameboard renders yellow, readable and correctly
oriented; no sign fallback material. The broad scene still has flat mobile light,
limited shadow definition, sparse ground context and estimated architecture.
The exposure/cascade change does not establish parity with the desktop render
and does not satisfy photorealistic acceptance. No new performance claim.
Capture restoration confirms command line restored and all normal checkpoint
hashes unchanged. Cold recovery matrix is recorded separately when complete.

V20 final verification: 263/263 Android cases in ten groups passed. Legacy,
LHB and station cold recovery all passed including presentation recreation.
Station generation 53, tick 516, fingerprint 2c55c3509c5a4284; legacy and LHB
fingerprints unchanged. Evidence: Station/egmore-signage-v20-recovery-matrix.json
and Station/Device/egmore-signage-v20-final-result/android-regression-summary.json.
Phone wake restored 2 -> 3 -> 2. V20 is a validated technical art candidate,
not a visually accepted station or new performance benchmark.

## V21 Android and Automation verification
Build 20261005-092303 succeeded; APK 289944622 bytes, SHA256
c56e049a4a16f8c00d81e74c20c84630eff541f489196ebcf61f869810072565.
Ten Unreal Automation suites passed with zero warnings and zero failures,
including expanded station mast grade/contact/verticality checks.
All 263 Android startup cases passed. Legacy, LHB and station cold recovery
passed, including recreated actors and unchanged logical identities/fingerprints.
Final station generation55, tick516, fingerprint2c55c3509c5a4284.
Egmore phone capture: Evidence/Egmore/Device/review-20261005-092546;
092632.png inspected. Sign remains readable; broader mobile lighting still flat.
Normal saves unchanged during art review; wake2->3->2 restored.

The first V21 WAP7+LHB+generic-station performance run was excluded: app exited
through UGameEngine::HandleExitCommand at about73 seconds, Android EXIT_SELF0.
No performance result is claimed from this incomplete capture. Logs, partial
samples and restoration are retained at Station/Device/ohe-grade-v21-wap7-lhb-station.
Normal checkpoint hashes and wake setting restored. A fresh retry is running.

V21 retry observation: device reported Thermal Status 1 during sampling; a
system notification overlay was visible in screen-80.png. Record these conditions
with this single-run measurement; do not present it as a controlled comparison
against the earlier V18 thermal-status0 run. No notification settings were changed.


V21 retry completed all 6000 frames (4795 analysis frames, 10–170 s window).
FPS 29.9687; frame p95 34.785 ms; GPU mean 21.1144 ms.
PSS mean 621.60 MiB, max 628.72 MiB.
Thermal status ranged 1–2 (mild/moderate). One run, no same-build baseline campaign;
not a sustained budget or Egmore-gameplay claim. Normal checkpoint hashes unchanged,
command line restored, phone wake2->3->2 restored.
Evidence: Station/Device/ohe-grade-v21-wap7-lhb-station-retry.
Combined verification: Station/v21-validation-summary.json. Art remains unaccepted.
