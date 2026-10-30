# Driving performance capture

The new CSV instrumentation separates driving from menu, result, background and
explicit-pause frames. `RailS7/DrivingFrame` is sampled before presentation work,
so essential-loading and scenery-build stalls during travel remain included.
`PresentationBlocked` and `PresentationTransition` describe those frames.

CPU timings identify checkpoint saving (including verification and evidence),
telemetry serialization/writing, corridor cell creation/removal, and the complete
continuity update. Scopes overlap; never add them together. `CellsBuilt` records
the number of cell builds in a frame.

Use a Development build with `-csvCaptureFrames=9000 -csvCompression=0` and a
fixed frame cap. For Android, preserve `UECommandLine.txt` before adding flags,
and restore it afterwards. `-S8AssetReview -S8Tambaram -S8Cab -S9LiveJourney`
starts a fresh Assisted diagnostic service in the separate AssetReview save
directory. Back up existing diagnostic saves first. Normal LiveServiceV1 records
must not be replaced. Do not use this fresh-start flag in a normal user journey.

Wait for CSV finalization before pulling the capture from Saved/Profiling/CSV.
Run `python Tools/summarize_driving_frames.py capture.csv --output report.json`
from this folder. The tool refuses unfinished files, absent driving markers,
invalid values and empty driving samples. Existing mixed CSVs cannot be relabeled
as comparable driving runs.

For a before/after comparison, keep device, thermal state, camera, graphics,
frame cap, scenario and sampled route interval the same. Preserve the APK hash,
command line, telemetry, battery/thermal readings and finalized CSV with each
report. A start-of-journey sample includes waiting at the protected throat and
does not qualify the complete corridor. Use long route coverage for release
qualification. Screen recording adds load and should be measured separately.

The instrumentation itself is not a performance improvement. Do not claim
smoother frames until comparable rendered captures support that conclusion.

## October 20 device results

Android10 baseline and Android11 candidate each recorded 5,999 service-driving
frames (6,000 total), fresh Assisted LiveJourney, cab camera, 30 FPS cap.
The candidate limits corridor construction to two cells per view update,
prioritizes essential cells, and reports not-ready until all essential cells
exist. Existing simulation/presentation gating then holds the service safely.

| Metric | Android10 | Android11 |
| --- | ---: | ---: |
| Maximum cells built in one frame | 56 | 2 |
| Maximum cell-building CPU time | 136.816 ms | 12.581 ms |
| Maximum frame time | 467.160 ms | 314.831 ms |
| Mean frame time | 34.201 ms | 36.087 ms |
| p95 frame time | 35.914 ms | 50.053 ms |
| Frames over 100 ms | 3 | 4 |
| Maximum checkpoint CPU time | 79.771 ms | 93.769 ms |

This demonstrates bounded cell construction, not an overall smoothness win.
Baseline thermal status changed from 1 to 3; candidate stayed at 2. These are
short initial-service samples, including the traffic wait; neither qualifies the
whole route. The remaining 315 ms startup frame spends roughly 306 ms in the
continuity update while cell building accounts for only 6 ms. Further timing
breakdown is needed before optimizing that remainder.

Evidence: `Evidence/DrivingPerf-20261020-01` and `-02`, finalized CSVs,
reports and `-02/comparison.json`. Four analyzer tests cover filtering,
unfinished/legacy capture rejection, malformed data and late Unreal columns.

Android11 installed SHA256:
`63eb26e8180832e69b93749fd8f601edb99a0e806118e11ac87b0f818b9dcdfd`.
Desktop Shipping package `08_Builds/S9_Journey-Desktop-20261020-03/Windows`
contains the same change and live launcher; runtime testing remains pending.
The normal phone command line was restored. The completed run's fingerprint,
20 reputation and two awards recovered unchanged, and all 18 older checkpoint
hashes still match.

## Station sampling follow-up (2026-10-20)

Android12 reuses `PreparedRouteSampler` for station precinct selection and all
platform, canopy, furniture and nameboard placement. Previously each placement
walked and validated the route again. Station coordinates and readiness rules
are unchanged. `RailStations/PresentMs`, `RebuildMs` and `Rebuilds` CSV scopes
separate station work from corridor cell construction. The capture analyzer
includes these timings and rebuild counts in hitch rows.

All 77 journey tests passed (`Evidence/StationSampler-20261020-01`), including
dense forward/reverse sampler equivalence, boundaries, terminal dwell and cold
recovery. The four capture-analyzer tests also passed. Checkpoint storage and
its validation remain unchanged.

Android12 installed successfully, SHA256
`29d0115eaccfaa4cc4e2916c3ce31cdf356bb9193a736cb467b12a620311cd29`.
The finalized 6,000-frame capture (5,999 driving frames) is in
`Evidence/DrivingPerf-20261020-03`. Same phone, fresh Assisted review, cab and
30 FPS cap; thermal status was 2 before and after. Compared with Android11:

| Metric | Android11 | Android12 |
| --- | ---: | ---: |
| Mean frame time | 36.087 ms | 33.516 ms |
| p95 frame time | 50.053 ms | 34.423 ms |
| p99 frame time | 51.124 ms | 36.501 ms |
| Maximum frame time | 314.831 ms | 265.797 ms |
| Frames over 100 ms | 4 | 2 |
| Maximum checkpoint scope | 93.769 ms | 82.634 ms |

This is one opening-section capture, not a full-route qualification or isolated
proof of causation. Station rebuild still peaks at 105.414 ms; remaining startup
and checkpoint work needs further improvement. Wireless ADB disconnected after
capture and was reconnected through its discovered endpoint. The complete CSV
was recovered, normal launch flags restored, and cold recovery verified the
unchanged logical fingerprint, complete service state, 20 reputation and two
awards. The result screen was visually checked. Android and Windows editor
compiled; a new Windows Shipping package was not produced in this pass.

## Incremental station construction (Android13)

Station planning now queues mesh paths and transforms without loading every mesh
in the planning frame. Each subsequent update creates at most two components and
adds at most 128 instances per station view. Old components retire two per
update, before construction starts. Nameboards use the same queue. Live
components remain GC-visible; queued work holds only paths and transform values.
The station stays not-ready until all queued work finishes. The existing
essential-presentation gate holds simulation and discards loading deltas.

`RailStations/BuildBatchMs`, `ComponentsBuilt`, `InstancesBuilt` and
`ComponentsRetired` expose the work budget. Limits are per view: overlapping
streamed chunks can contribute to the same CSV frame. Individual synchronous
asset loads are still indivisible, so this is not a guaranteed millisecond cap.
The new `IndianRailway.S9.StationBuildQueue` automation test checks budgets,
readiness, retirement, rebuild counts, disposal and unchanged snapshot bytes.

### Desktop verification, 2026-10-22

The interrupted build was resumed with a fresh output folder. Windows Shipping
package `08_Builds/S9_Journey-Desktop-20261022-01/Windows` built successfully and
includes `Start Live Railway Journey.cmd`. The new station queue automation test
passed (`Evidence/StationQueue-20261022-01/report`); the prior 77 journey tests and
four analyzer tests also passed.

The packaged executable restored a copied diagnostic checkpoint in an isolated
user directory. Its service state and logical fingerprint matched, with 770
station instances across 36 components, ready=true, no pending work and no
presentation block (`Evidence/StationShipping-20261022-01/recovery-check.json`).
Normal player saves were not used for this desktop test.

A separate rendered editor-game capture at 1280x720 and a 30 FPS cap finalized
1,800 frames, including 1,799 driving frames. Station construction peaked at
30.783 ms, planning at 11.401 ms, and checkpoint work at 54.517 ms. Recorded
maximum construction was two components and 128 instances per frame. All 26
blocked presentation frames had zero service-update work. Frame time averaged
33.815 ms, p95 36.198 ms, p99 45.127 ms, with a 373.242 ms startup maximum and two
frames above 100 ms. This is an opening-section editor capture, not a Shipping
FPS qualification, full-route test, or comparison against phone timings.
Evidence: `StationPerf-20261022-01/report.json` and `queue-check.json`.

The screenshot smoke review exposed an existing false-positive: its original
result reported success despite an overload pause visible in later screenshots.
`StationRendered-20261022-02/result.json` must not be treated as a clean gameplay
pass. The editor review now rejects success while a running service is paused
or presentation-blocked. Screenshot capture still reproduces an overload pause;
this work does not claim to eliminate all stalls.
The corrected reviewer was rerun: `StationRendered-20261022-03/result.json`
properly reports failure for the reproduced paused service. All test processes
were closed after collecting evidence.

Mobile was disconnected on resumption. Android13 is packaged, but installation
completion, new phone profiling and phone recovery verification remain pending.

## Coach loading and checkpoint checksum follow-up

Coach construction now loads one of seven independently movable components per
scenery update. The second, pose-only presentation pass cannot load more parts.
Existing chunk readiness still requires all seven components. Checkpoint CRC32
uses a constexpr lookup table with the same polynomial, initial/final XOR and
wire bytes. Semantic restore checks, alternate slots and disk readback are
unchanged. New tests compare CRC against the former bitwise algorithm and the
standard check vector; all 240 host regression cases and four analyzer tests
passed. The updated LHB presentation automation test also passed.

Windows Shipping package `08_Builds/S9_Journey-Desktop-20261022-02/Windows`
built successfully. On 2026-10-23 it restored an older copied checkpoint with
unchanged service state and fingerprint, seven coach components and no block
(`Evidence/StartupShipping-20261023-01/recovery-check.json`).

The matched desktop editor-game opening capture has 1,799 driving frames.
Maximum coach work was one component per frame, with 76.439 ms peak loading.
All 26 blocked frames had zero service-update work. Checkpoint scope peaked at
52.575 ms (previous 54.517 ms). Overall performance did NOT improve in this
single comparison: mean 34.127 ms, p95 36.261 ms, p99 61.583 ms, maximum
423.834 ms and four frames over 100 ms. The largest frame occurs before coach,
station or cell-building scopes, with render-thread startup work; a later
316.631 ms loading frame includes 76.439 ms coach work and other costs.
This does not establish causation or full-route smoothness. Evidence:
`StartupPerf-20261022-02/report.json` and `queue-check.json`.

### Android14 verification, 2026-10-23

Installed `08_Builds/S9_Journey-Android-20261023-14/Android_ASTC/MyProject-arm64.apk`
without clearing app data. SHA256:
`20245552931d7c9fd04bf95c61b523d48a090932f3349818debd7d62692a83f6`.
Backed up normal and review saves before testing. The fresh Assisted cab capture
at 30 FPS finalized 6,000 frames (5,999 driving frames). Thermal status was 0 at
start and 2 after capture; Android12 started and ended at 2, and camera
orientation differed slightly. These are not controlled causal comparisons.

| Android14 metric | Value |
| --- | ---: |
| Mean frame time | 33.475 ms |
| p95 / p99 frame time | 34.244 / 35.806 ms |
| Maximum frame time | 227.290 ms |
| Frames above 100 ms | 2 |
| Maximum station construction batch | 12.989 ms |
| Maximum station Present scope (includes planning) | 34.599 ms |
| Maximum coach loading scope | 58.242 ms |
| Maximum checkpoint scope | 76.738 ms |

The capture verified maximum per-frame work of one coach component, two station
components and 128 station instances. All 64 blocked presentation frames had
zero service-update work. Runtime samples were unpaused, with all seven coach
components and 770 station instances loaded and no pending station groups.
This covers the opening section, not a new full journey qualification.

Restored normal command-line settings and cold-launched the game. Generation
463 recovered with identical service state and fingerprint, two awards and 20
reputation. The completed Tambaram result was visually checked; the phone was
left on that result screen. Evidence: `StartupMobile-20261023-01`, including
`report.json`, `queue-check.json`, `recovery-check.json` and `recovered.png`.

The latest desktop screenshot retest still reproduced an overload pause and
correctly failed (`StartupRendered-20261023-01/result.json`). Startup/rendering
hitches and screenshot capture remain unresolved; no general smoothness claim.
