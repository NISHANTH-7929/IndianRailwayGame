# Station candidate validation

The 26-module station kit is integrated and measured as a candidate. It is not final high-realism acceptance or completion of all S8 content.

Current Android ASTC APK: 278,422,382 bytes. SHA-256 `e6161dab2c953836cbf41a5a16b6cbb9f8754c38e89ff511afe49aa85a5e71ae`.

Unreal Automation: 10 passed, 0 failed. Source/import byte checks: True. Each module has three imported LODs; aggregate triangle counts are [48892, 26900, 12317].

Android APK-update/cold recovery: True. Generation 9, service tick 516, exact logical fingerprint `2c55c3509c5a4284`. Service state, eight LHB vehicles and all 4,216 station instances were restored.

| Capture | Current APK | FPS | Frame p95 / p99 ms | GPU mean ms | Peak PSS MiB | Saves preserved |
|---|---|---:|---:|---:|---:|---|
| station-a-01 | earlier | 29.956 | 34.729 / 35.552 | 23.375 | 650.87 | True |
| station-apk-baseline-01 | earlier | 29.967 | 34.515 / 35.181 | 16.405 | 622.72 | True |
| station-v5-a | earlier | 29.964 | 34.654 / 35.394 | 21.655 | 657.03 | True |
| station-v5-b | earlier | 29.962 | 34.610 / 35.484 | 20.676 | 644.13 | True |
| station-v5-baseline | earlier | 29.962 | 34.717 / 35.524 | 15.967 | 605.03 | True |
| station-v6-a | yes | 29.970 | 34.538 / 35.255 | 21.073 | 633.76 | True |
| station-v6-b | yes | 29.969 | 34.536 / 35.279 | 21.054 | 628.79 | True |

Each capture uses 6,000 engine frames, a 30 FPS cap and a 10–170 second analysis window on the connected iQOO Neo 9 Pro, Android API 36, Vulkan/ASTC. CPU/GPU intervals overlap. These are engine intervals, not compositor measurements. Peak memory includes the whole sampled run.

Only compare rows sharing an APK hash. The changed exterior camera also changes visible geometry, so earlier-versus-current differences are not isolated geometry optimization results. One run per configuration cannot establish sustained thermal behavior or a final performance budget.

The first captures inherited a planned `repetitions: 3` profile label despite containing one capture; this report treats them as one. The profiler now writes the actual count. Early station context/device-metrics descriptions also inherited LHB-only wording; launch flags, measurement scope and actual station telemetry identify their station workload. Raw evidence is retained.

Normal S7 and LHB checkpoint hashes are checked around each capture. Later captures additionally check station-service slots. The station service has its own save directory; old fixture content identity and authoritative simulation algorithms are unchanged.

USB-only stay-awake was verified as Android setting `2`. It does not request battery-only wakefulness. Visual limitations and comparison evidence are documented in STATION_VISUAL_REVIEW.md; architecture changes are documented in STATION_ARCHITECTURE.md.

V6 corrects the reflected FBX Y axis. Authored and imported bounds agree within 1 mm, and Unreal Automation checks the actual platform edge and height. V5 timings remain historical evidence; its platform alignment is superseded.

V6 used wireless ADB. Battery and thermal snapshots accompany each capture. The APK contains the earlier Egmore study assets but these measurements exercise the generic station kit; they do not measure the new Egmore architectural revision.

Legacy, LHB and station cold-recovery results are recorded in Evidence/Station/recovery-matrix-v6.json. The S7 map was repaired after an art-review script mistakenly edited it; retained actors were audited against prior evidence. Its file hash changed through serialization. See STATION_ARCHITECTURE.md for the repair record.

V6 train-only comparison attempts lost foreground, including sleep/keyguard and later backgrounding. They are excluded and preserved with failure/restoration logs; normal checkpoints stayed unchanged. No valid V6 train-only comparison is claimed unless a completed row appears above.

Failed import-v4.log is preserved: LOD generation rejected commandlet execution. Full-editor imports passed. The V6 Android build succeeded and installed with `adb install -r`.
