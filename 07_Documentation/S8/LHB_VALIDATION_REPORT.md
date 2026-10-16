# LHB integration validation — 3 October 2026

Status: LHB configuration integration and Android validation completed. Installed
build is left on the recovered LHB result screen. This report distinguishes
configuration validation from final art acceptance. Other S8 content remains deferred.

## Delivered boundary

`IR-S8-LHB-3A-1` uses stable definition 3001/version 1 for the AC 3-tier coach:
24 m over coupling planes, 23.54 m body, bogie centres at +/-7.45 m, 2.56 m
bogie wheelbase. Additional formation gap is zero because the definition already
includes coupler projection. The original 22 m fixture and its identity remain.

The existing WAP-7 asset uses definition 3000/version 1 with its measured
21.33043212890625 m envelope and +/-5.85 m authored bogie pivots. This is an
asset measurement, not certification of prototype WAP-7 dimensions. The short
formation is 45.33043212890625 m and the seven-coach formation is
189.33043212890625 m. The short formation fits the existing 95–145 m platform
at head marker 141 m. See LHB_ARCHITECTURE.md for the complete change record.

Operations initialization, service validation/start, checkpoint save/restore and
snapshot validation/rebuild consume the same versioned configuration. Old
service/save directories remain separate. The LHB map removes historical static
review exhibits from a copy; the original S7 map was not changed.

## Validation evidence

- Host regression: 263 cases passed (238 existing cases and 25 LHB cases).
- Final Unreal Automation: 9 suites passed, zero warnings/failures/not-run,
  `Evidence/LHB/UnrealAutomation-FinalOct03/index.json`.
- Android embedded tests: all ten groups passed, including the full passenger
  service, curved traffic footprint, legacy recovery and mixed-slot no-write
  checks. Files: `Device/compatible-restored-result/Saved/*tests.json`.
- Actor/presentation recreation: Unreal LHB presentation test validates seven
  component types, eight coaches, destroy/recreate cycles and unchanged logical
  snapshot bytes. Android screenshots show the corrected instanced materials.
- Cold recovery: `compatible-restored-result` restored generation 34 paused;
  saved and restored fingerprint both `8890142ee5d6ce9a`.
- Original fixture recovery on the final APK: `legacy-final-result` restored
  `IR-S6-fixture-1`, generation 164 and fingerprint `0efccd7c539ec58a`, matching
  the earlier preserved recovery. Reputation 20, two awards and unlock state
  remain intact; no LHB components are active in the legacy configuration.
- Live Android platform stop: `live-service-platform` reached ReadyToDepart,
  completed all 300 dwell ticks (10 seconds), with berth error -0.2496 m and
  zero overspeed/harsh-control ticks. The 45.3304 m formation stayed inside the
  existing 50 m platform (head approximately 140.7504 m, rear 95.4199 m).
- Live service completed all nine objectives, Grade A, 93.1639/100. Saving the
  result granted reputation 10 and the unlock once. Cold restart restored
  generation 45, tick 10236, fingerprint `ff7e0afb56b48956`, the exact settled
  service state and one award. Evidence: `live-service-settled` and
  `live-result-cold-result`. These launches recreated the LHB presentation.
- Unsupported content: both isolated LHB slots were staged with a same-length
  unsupported identity and valid CRC. Recovery stayed safely paused; both
  records remained byte-identical. Originals were restored byte-for-byte.
  `Device/incompatible-slots/{verify,restore}.json` and `incompatible-result`.
  Telemetry `blocked` describes streaming, not recovery; the recovery message
  and `S6RecoveryBlocked` enforce this rejection.

## Installed artifact

`08_Builds/S8_LHB/Android_ASTC/MyProject-arm64.apk`, 262,789,050 bytes.
SHA256: `386B45C8BB784B2BC9205447D3917D8636A7E273DA38C3B897DFE7E882B5E351`.
Install and installed-file hash evidence are retained under Evidence/LHB.
Device: I2304 / iQOO Neo 9 Pro, Android API 36, ARM64, Vulkan, ASTC.

## Measurement method

Completed comparison: one valid run per variant, 6,000 captured frames each;
4,795 frames per run in the analysis window. The inherited profile's planned
`repetitions: 3` is not a claim that three valid repetitions were collected.
Two interrupted baselines are excluded. Results are a measured integration
sample, not a repeated-run confidence interval or sustained thermal budget.

| Metric | WAP-7 + detailed LHB | WAP-7 + coach proxies |
|---|---:|---:|
| Mean FPS (30 cap) | 29.964 | 29.968 |
| Frame p95 / p99, ms | 34.377 / 35.116 | 34.600 / 35.272 |
| Mean game thread, ms | 7.143 | 7.109 |
| Mean render thread, ms | 12.841 | 7.545 |
| Mean GPU, ms | 16.475 | 9.637 |
| Maximum sampled PSS, MiB | 620.95 | 557.38 |

Detailed coach presentation adds about 63.56 MiB peak sampled PSS and 6.84 ms
mean GPU time in this pair. Frame rate remains capped near 30; the small frame
percentile difference is not evidence that detailed coaches improve performance.
Android reported thermal status 1 for all samples. Both runs kept two chunks
resident and preserved the normal S7 checkpoint hashes. Machine-readable results:
`Evidence/LHB/android-measurements.json`; raw folders `Device/lhb-01` and
`Device/baseline-03` include CSV, telemetry, screenshots and logs.

Same installed APK and isolated LHB service map for both variants. WAP7 renders
the detailed locomotive and eight LHB coaches; baseline retains the locomotive
and same logical formation but disables detailed coach rendering. Both exercise
AI traffic, service logic, cab/exterior transitions and control presentation.
This baseline measures added coach presentation cost, not a different old save.

Each run captures 6,000 Unreal CSV frames at a 30 FPS cap. Analysis uses seconds
10–170, nearest-rank percentiles. These are engine frame intervals, not Android
compositor timings. CPU/GPU lanes overlap. Missing GPU timings remain unvalidated.
ADB PSS and thermal telemetry are sampled approximately every 15 seconds;
memory includes startup/export observations. Load latency telemetry repeats the
last recorded loads and is not an independent sample distribution.

The first report analysis rejected the new scope name; the offline profile
schema was extended and the original finalized CSV reanalyzed. The original
failure.json remains. At CSV finalization the first run paused after tick 5980,
outside the analysis window. This is recorded rather than counted as continuous
post-capture gameplay. The legacy overload guard is unchanged.

Baseline-01 was excluded after Android background/deactivation callbacks;
baseline-02 was excluded when focus became null and the device reported Asleep.
The game correctly paused. USB stay-awake was temporarily enabled; original
setting 0 was restored and read back after validation. Commandline now selects
the normal isolated LHB service without profiling flags. Existing S7 saves are
unchanged. The final APK remains installed on the phone.

## Remaining fidelity and reliability limits

The source-derived palette and geometry are not final photorealistic art. The
coach body proportions, gangway ends and source wheel flange retain documented
approximations. WAP-7 prototype dimensional fidelity is not certified. Existing
handling masses remain provisional. AI locomotive presentation is still the
legacy proxy; detailed WAP-7 is the player locomotive.

One earlier cold launch failed in Unreal memory-tracker/allocator startup before
a fresh engine log; the next launch recovered exactly. This intermittent failure
has not been proven fixed. Higgsfield returned “Requires basic plan or higher”;
no generated image or source-asset upload was used. User confirmed the Sketchfab
download through their account; acquisition date/receipt remains unavailable.

All previous evidence, including failed runs, remains preserved. No final art or
full S8 release acceptance is asserted by these integration tests.
