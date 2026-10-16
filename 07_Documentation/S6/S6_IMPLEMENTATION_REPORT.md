# S6 — Streaming and safe continuity

**S6 PASS**  
**Acceptance: 30/30**  
Validated 2026-09-27 on the iQOO Neo 9 Pro. Technical prototype scope only.

## 1. S6 objective

Prove that the protected player/AI railway survives presentation streaming and Android interruption. This is a two-chunk technical prototype, not the finished world or S7 gameplay loop.

## 2. S5 baseline

Read the S2–S5 reports and inspected actual source/configuration before editing. S5 passed 22/22. Pre-edit Source, Config and project-descriptor copies are in `Evidence/Baseline`, with `baseline-hashes.json`. Existing S2 39, S3 28, S4 39 and S5 36 tests remain passing. Core algorithms and their existing test implementation files are byte-identical to the baseline. Four headers add narrowly scoped codec friendship without exposing mutable state to views. See `Evidence/source-review.json`.

## 3. Architecture changes

`RailContent` supplies pure visual demand. `RailSnapshotRecord` supplies immutable versioned records and selection. `RailWorldSnapshot` serializes and reconstructs the logical world. `RailCheckpointStore` implements alternating-slot transactions over an injected filesystem adapter. `RailContinuity` integrates S6 streaming, camera, files, lifecycle and evidence in the existing subsystem. `RailChunkView` is owned by a real streamed ULevel.

`URailSimulationSubsystem` remains the sole mutable Unreal owner. Candidate snapshots are isolated value copies. Views receive const logical state. S2–S5 mode branches retain their behaviour. A separate continuity pause is necessary because the older input-cancel pause intentionally clears queued controls, which would break exact recovery.

## 4. Presentation chunk design

Persistent map: `/Game/IndianRailway/S6/L_RailContinuity`. Actual streamed levels: `L_ChunkA` and `L_ChunkB`, loaded using `ULevelStreamingDynamic::LoadLevelInstance`. A covers world X < 0 metres; B covers X >= 0, at the E1 junction/240 m station on A's upper path.

Each level owns one chunk view, train-body instances, track drawing, signal/point symbols and 512 scenery proxies. Members are assigned by their derived body centre; the complete body remains in its owning view. A formation can have members in both levels. Graph, formation and path history are never partitioned by visual ownership. `Tools/Create-S6Assets.py` authors both levels and unique proxy mesh/material assets reproducibly.

## 5. Logical versus visual ownership

RailKernel owns traversal and committed history; RailHandling owns longitudinal state; RailOperations owns whole-train occupancy, resources, routes, locks and authority; RailAIDriver owns deterministic intent under that authority. ULevel, Actor, component and asset lifetimes are visual concerns. Actor transforms never become logical train position.

Unloading a signal/point view cannot grant a route, unlock a point or clear occupancy. Missing essential presentation explicitly gates simulation updates without changing signal aspects or railway authority.

## 6. Content manifest

`content-manifest.json` identifies `IR-S6-fixture-1`, levels, rebind keys, essential meshes, optional material/scenery presentation, dependencies and ownership. It is a loading contract, not topology. Graph, formation/handling definitions, operations definitions and AI contract are bound to the compiled content ID. A topology change requires explicit content/version policy; recovery never guesses a new edge from old coordinates.

## 7. Asset lifetime and streaming policy

Demand uses complete front/rear vehicle references, speed/direction, near-term committed route samples and the following inspection camera's viewing band. Formation margin is `max(30 m, 40 m, 8 × |speed|)` normally or `max(60 m, 40 m, 8 × |speed|)` for look-back, conservatively applied both ways. Route preview extends `max(40 m, 8 × |speed|)`. The 45-degree camera contributes a ±160 m band before those margins.

Irrelevant levels are hidden, unloaded and removed; strong references are released and garbage collection requested. Weak references audit retired Actors and unique meshes without retaining them. Shared materials may remain referenced by the other chunk. Bounds: two levels, two chunk views and 1,024 proxies. Engine/driver reclamation latency is measured separately from logical correctness.

The debug recreate action pauses and removes actual levels for three seconds before reloading required content. Explicit content/save/background pauses exclude inactive wall time; the active fixed-step overload policy is unchanged.

## 8. Snapshot schema

Envelope: `IRSNAP06`, schema u32, generation u64, content-length u32, payload-length u32, content bytes, logical payload, CRC32; little-endian; maximum 1 MiB. Generation zero/exhaustion, malformed sizes, duplicate keys, invalid booleans, non-finite values, unsupported versions and trailing payload data are rejected. Payload fields use explicit 64-bit integers/enums/lengths, IEEE-754 doubles and bounded containers, never raw C++ memory dumps.

Captured state includes shared tick/accumulator; TrainIds, vehicle identities/definitions/orientations and formation gap; head/leading typed positions, stations, prior interpolation head, choices and path occurrences; handling speed/acceleration/forces, applied and commanded controls, queues, sequence/cancellation flags; operation requests, generations, occupancy, reservations, routes, point state/locks and authority; AI binding, phase, retries, observation/decision counters, flags, bounded trace/hash. Definitions are rebuilt from version-bound content. Wall-clock metrics, Actors/UObjects/components, addresses and presentation transforms are excluded.

Diagnostic FNV fingerprints use a canonical generation-1 full record for cross-generation logical equality. CRC32/FNV do not establish adversarial-client or multiplayer authority.

The offline `Tools/Inspect-S6Checkpoint.cpp` tool validates a real checkpoint using the same pure recovery codec and exports its logical state, committed path occurrences, vehicle IDs, definitions and stations. `Evidence/moving-checkpoint-decoded.json` shows the 181 m/eight-member player formation across edges 1, 3, 4, 5, 6, 7 and the 43 m/two-member AI formation on edge 2. `Evidence/profile-checkpoint-decoded.json` captures the later saved state. The inspector never mutates a checkpoint or running world.

## 9. Save transaction

At a consistent game-thread boundary, semantically validate both slots, select the newest complete valid predecessor, capture an immutable candidate, write only the other slot, flush/close, compare readback bytes and validate decoded state. The predecessor remains intact; timestamps are irrelevant. Conflicting equal-generation records fail safe.

If existing records have no valid predecessor, retain them and reject saving. A schema/content-incompatible target is retained even when the other slot is a valid fallback; explicit archival is required before overwriting it. Corrupt/incomplete records can be replaced while preserving the valid predecessor.

Triggers: explicit checkpoint, controlled pause, first boundary milestone per fixture run, background/deactivation and supported clean world exit. No production periodic-save/30-second guarantee is claimed. Abrupt termination can lose progress after the last verified generation. Alternating slots handle torn records; hardware power-loss durability is not claimed.

## 10. Recovery sequence

Validate integrity/schema/content; initialize exact definitions; decode an isolated candidate; validate roster, IDs, histories, choices, clocks, handling and queues; rebuild derived poses and verify typed positions; validate saved whole-train occupancy, ownership, committed route paths and authority endpoints; recompute occupancy/authority; derive signals afresh; then publish the complete candidate.

Selection requires semantic recovery, so a newer checksummed but unsafe world cannot outrank a valid backup. Load essential presentation and rebind from recovered logical state. Resume remains paused until explicit user action; inactive wall time is discarded without resetting actuators, queued controls or AI timing. With no compatible checkpoint, Resume remains visibly and logically paused. Explicit fixture reset does not delete incompatible records.

## 11. Android interruption handling

Lifecycle callbacks pause/save on the game thread; callbacks from another thread defer capture there rather than race mutable state. Foreground does not silently resume. Back follows the existing Android exit path, using available lifecycle/world-exit saves. Force-stop recovers the last verified local generation.

S2's removal of Android INTERNET permission remains in effect. Recovery is tested without application network access while the phone hotspot stays available to the workstation. Network settings and airplane mode are not changed.

## 12. Corruption handling

Pure tests cover interrupted writes, corruption/truncation, unsupported schema/content, semantically invalid newer worlds, conflicting generations and transactional rejection. Negative worlds include forged occupancy, partial locks, expanded/stale authority, unknown resource IDs, mismatched AI binding, invalid actuator state and vehicle identity. Signals are re-derived rather than trusted.

A review found that a later save after compatible fallback could overwrite an incompatible record. The store now refuses it and preserves both records. Dedicated tests cover future-schema and missing-content records with a valid fallback. Final packaged compatibility tests passed 10/10, including unchanged hashes after attempted saves over incompatible targets and blocked Resume when both slots are unsupported. Original checkpoints were restored and successfully recovered afterward. See `Evidence/Device/compatibility-final/results.json`.

## 13. Actor recreation

Actual ULevel removal destroys train, track, signal, point and proxy views together. The protected logical world survives. Reload constructs new Actors and fills them from current state. Full-state fingerprints, level counts and retired weak references provide before/during/after proof; Actor positions alone are not used as evidence.

## 14. Boundary-crossing test

The 181 m, eight-vehicle player train crosses the 240 m junction while the two-vehicle AI waits on its lower holding track. At head station 240–421 m, front and rear span the boundary. Final APK control tests paused at tick 831, head 302.691688440501 m, speed 8.182472611685 m/s. Ten vehicle views were present across two levels. Full-state fingerprint `26576a44f567ddd6` remained unchanged through look-back and actual 2 → 0 → 2 level removal/recreation; retired Actor and unique-mesh weak references returned to zero.

The later brake test stopped at approximately 387.06 m, still spanning the boundary with R1 retained and the AI waiting. `Evidence/Device/controls-final/12b-boundary-stopped.png` was visually inspected: all eight player bodies, both locked points and the two AI bodies are visible. Raw checkpoints, state JSON and screenshots are in that directory.

## 15. Performance and memory results

Final capture: 6,000 engine frames. Analysis window 10–170 seconds contains 4,795 frames, averaging 29.967 FPS. Frame intervals: p50 33.377 ms, p95 34.555 ms, p99 35.260 ms, maximum 37.898 ms; 0 frames above 50 ms and 0 above 100 ms. The scenario includes controlled pauses and real level cycles; these are engine intervals, not compositor presentation timing. See `Evidence/profile-final-summary.json` and its raw CSV source.

The final sustained run completed 451.00 seconds with 225 logical samples, 9 forced paired unload/reload cycles, 22 load requests and 21 completed unloads. Actual residency stayed within 2 levels / 2 chunk Actors / 1,024 proxies. Demand-driven single-level operation was observed while simulation advanced. Retired Actor and unique-mesh references returned to zero at the end; every sampled logical state was valid and no railway overload/fatal/assertion match occurred in the fresh engine log.

Sampled PSS range: 425.28–445.22 MiB. First-three median 438.89 MiB; last-three median 432.78 MiB; change -6.12 MiB, within the predeclared +32 MiB bound. Android thermal statuses observed: [0, 1]. Maximum sampled chunk request-to-ready latency 68.849 ms; maximum sampled save transaction 5.097 ms. These samples are not exhaustive per-operation percentiles. See `Evidence/Device/sustained-final/residency-summary.json` and `Evidence/residency-audit-policy.json`.

| Counter (ms) | Mean | p95 | p99 |
|---|---|---|---|
| GameThreadTime | 6.7835 | 8.7470 | 9.9620 |
| RenderThreadTime | 5.2342 | 6.6150 | 7.2540 |
| RHIThreadTime | 3.6125 | 4.6970 | 5.4180 |
| GPUTime | 1.8169 | 2.7140 | 3.6490 |
| RailS6/AIUpdateMs | 0.0082 | 0.0446 | 0.0548 |
| RailS6/SimulationStepMs | 0.1557 | 0.3529 | 0.4374 |

CPU lanes can overlap and include waits; do not sum these columns into frame time. AI/simulation averages include intentional paused frames.

Raw CPU/GPU/AI/simulation counters and their validation status are in the profile summary. Zero-only or absent GPU/temperature counters are explicitly UNVALIDATED and must not be interpreted as zero cost. PSS includes allocator/driver caches; this finite proxy-fixture run does not establish asymptotic leak freedom or final-art/world-scale capacity.

Earlier profile attempts, including USB-disconnected captures and the pre-camera-fix run, remain separately labelled and are excluded from final acceptance. The profiling launch override was restored after all attempts, including cleanup after reconnecting.

## 16. Android results

Final rebuilt APK passed all 21 control/recovery checks and all 10 compatibility checks. Device: iQOO Neo 9 Pro/I2304, Android 16, ARM64, Vulkan/ASTC. Cold launch after final installation completed in 1,172 ms (ADB TotalTime). Tests include process kill while running, exact moving/braking recovery, same-process warm foreground with unchanged paused state, AI waiting, corrupt newest fallback, continued operation, boundary stop, Back exit/recovery and absent INTERNET permission. Automated touches require the game to be foreground.

Artifact: `08_Builds/S6_Android/Android_ASTC/MyProject-arm64.apk`, 164,034,818 bytes, SHA256 `BF5173EBD75E270C42BFDF6FF043D30548F385D33BB4A67A55B815E43C319619`, build `20260927-093401`. Installation verified the remote hash and reported Success. Host tests pass 50 S6 + 142 S2–S5 cases; Unreal Automation passes 5/5 suites. Final sustained/performance validation passed; earlier attempts remain labelled separately. Packaged startup reran all 192 pure cases successfully; see `Evidence/test-summary.json`.

## 17. Problems encountered

An approval-review usage limit temporarily prevented execution. Unreal's `check` macro conflicted with a test helper. Initial assertions confused an AI retry phase with unsafe motion and current save generation with the immutable recovery choice. A diagnostic reader raced a telemetry rewrite. The long profile exposed a stale look-back camera position. Compatibility review found the later-save overwrite risk described above. Repeated USB drops interrupted host-driven measurement; a later residency attempt stopped safely at 278 seconds when the launcher became foreground.

## 18. Fixes

Renamed the helper; asserted denied/stopped AI behaviour across valid retry phases; compared recovery choice and full fingerprints; retried incomplete diagnostic reads with bounds while preserving failed evidence; followed the complete formation and included the camera band in demand; explicitly retained incompatible files. A bounded on-phone shell runner executes the same measurement schedule independently of the USB transport, with foreground guards before every action. The interrupted residency run is retained separately and rerun in full. Earlier failed/incomplete attempts are not counted as final passes.

## 19. Known limitations

Two finite non-partitioned levels and fixed-cost proxies only. No representative final train/station/people/vegetation/audio/weather/night cost, nationwide capacity or final-quality claim. Arbitrary rosters/topology and future migrations require explicit version work. PSS includes allocator/driver caches; finite runs do not prove asymptotic leak freedom. Larger-world save scheduling is unproven. No S7 service/station gameplay, missions, career, rewards, timetable gameplay, online or multiplayer systems were added.

## 20. Complete acceptance checklist

All 30 gates pass for the stated two-chunk fixture. The exact 39 requested test IDs map to raw evidence in [test-mapping.json](Evidence/test-mapping.json); these mappings are not 39 independent executions. Machine-readable gates: [acceptance.json](Evidence/acceptance.json).

| # | Requirement | Result | Evidence mapping |
|---|---|---|---|
| 1 | Two presentation chunks exist and can be activated/deactivated. | PASS | S6-T01 |
| 2 | Logical railway state survives visual unload. | PASS | S6-T02, S6-T03, S6-T04 |
| 3 | A train can span/cross a presentation boundary. | PASS | S6-T05 |
| 4 | Whole-train logical state remains correct across the boundary. | PASS | S6-T06 |
| 5 | Occupancy remains correct across the boundary. | PASS | S6-T07 |
| 6 | Route/point/authority state remains correct across the boundary. | PASS | S6-T08, S6-T09, S6-T10, S6-T11 |
| 7 | Actor destruction/recreation reconstructs presentation from logical state. | PASS | S6-T12 |
| 8 | Reverse/look-back behaviour is safe. | PASS | S6-T13 |
| 9 | Repeated load/unload has bounded residency behaviour. | PASS | S6-T14 |
| 10 | Versioned snapshots exist. | PASS | S6-T15 |
| 11 | Snapshots contain all state required for safe continuity. | PASS | S6-T15, S6-T16, S6-T17 |
| 12 | Actor pointers/UObject identity are not authoritative save state. | PASS | S6-T15 |
| 13 | Recovery validates schema/content/integrity. | PASS | S6-T23, S6-T24, S6-T25, S6-T26 |
| 14 | Occupancy is rebuilt/validated after restore. | PASS | S6-T18 |
| 15 | Route locks/reservations/authority are validated after restore. | PASS | S6-T19 |
| 16 | Signal state is re-derived after restore. | PASS | S6-T20 |
| 17 | Corrupt newest snapshot falls back safely to the last valid snapshot. | PASS | S6-T22 |
| 18 | Unsupported/missing content does not silently corrupt the save. | PASS | S6-T25, S6-T26 |
| 19 | Android interruption/restart recovery works offline. | PASS | S6-T27, S6-T29, S6-T30, S6-T31 |
| 20 | Player and AI state remain coherent after recovery. | PASS | S6-T16, S6-T17, S6-T28 |
| 21 | S2 regression passes. | PASS | S6-T32 |
| 22 | S3 regression passes. | PASS | S6-T33 |
| 23 | S4 regression passes. | PASS | S6-T34 |
| 24 | S5 regression passes. | PASS | S6-T35 |
| 25 | Unreal Automation passes. | PASS | S6-T36 |
| 26 | Android packaged streaming test passes. | PASS | S6-T37 |
| 27 | Android packaged recovery test passes. | PASS | S6-T38 |
| 28 | Sustained streaming/memory audit passes for the fixture. | PASS | S6-T39 |
| 29 | No S7 systems are implemented prematurely. | PASS | source/evidence audit |
| 30 | The result is documented with reproducible evidence. | PASS | source/evidence audit |

Reproduction steps: [REPRODUCTION.md](REPRODUCTION.md). Source changes and baseline comparisons: [source-change-manifest.json](Evidence/source-change-manifest.json), [source-review.json](Evidence/source-review.json). Artifact identity: [artifact.json](Evidence/artifact.json). Hash index: [evidence-hashes.json](Evidence/evidence-hashes.json).

## 21. Exact next dependency

**Exact next dependency: S7 — Vertical slice: complete player loop**

Stop after S6 acceptance; do not implement S7 in this task.
