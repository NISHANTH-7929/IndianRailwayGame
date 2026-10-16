# S9 desktop implementation report � 16 October 2026

Status: partial S9 desktop candidate; production-entry and full MVP qualification remain incomplete. User decision: retain WAP-7/LHB and existing Egmore�Tambaram corridor. No Android operations, paid services, new stock/corridor approvals or publishing.

## Changes from preserved S8

- Added read-only dispatch resource/owner diagnostics and a watchdog: 900 simulation ticks without at least 0.5 m observed movement or changed blockers prompts attention. Cyclic dependencies are warnings, not permission to clear occupancy. Runtime samples once per simulation second; pause does not age the clock; recovery and new runs reset history. Player HUD reports prolonged/cyclic waits; fallback route holds name a blocker.
- Added an independent logical stress fixture with four actual formations (three AI plus player), normal shared commands/dynamics/occupancy/authority, six delayed-departure/pacing combinations, emergency isolation and repeated pauses. All three AI eventually complete. This fixture is not a playable fourth-train corridor, a cold-save-compatible scenario, or a claim of arbitrary traffic capacity. See S9_DISPATCH_HARDENING.md.
- Replaced stepped bank structural panels with continuous graded banks; 176 fewer instances per active bank. Existing graph alignment/materials/streaming ownership retained. Straight engineered banks remain representative artwork.
- HUD headline now displays the applicable route speed restriction. Selection/briefing explicitly labels the Tambaram service representative, not an exact dated timetable/stock assignment.
- Added cook roots for dynamically referenced Chennai scenery and RunningGearV2 assets. Shipping startup no longer executes embedded regression suites; runtime validation remains enabled. Desktop-only packaging script retains unique evidence/archive paths.

## Verification

| Evidence | Result | Boundary |
|---|---|---|
| Evidence/Baseline-20261015/journey-results.json | PASS 57 cases | Fresh starting source, not all S9 |
| Evidence/Baseline-20261015/UnrealAutomation/index.json | PASS 23 suites, no warnings/failures | Fresh starting editor binary |
| Evidence/Regression-20261015/host-results.txt | PASS 238 cases | Core/S7 safety, AI, snapshots, checkpoints and settlement before final source integration |
| Evidence/Dispatch-20261015-04/results.json | PASS 23 cases | Includes 6/6 causal logical following stress cases; not roadmap R06 mission coverage |
| Evidence/IntegratedAutomation-20261015-01/index.json | PASS 24 suites, no warnings/failures | Dispatch integration; later cook/startup guard requires final build checks |
| Evidence/editor-build-20261016-01.log | PASS | Editor build including cook/startup corrections |
| Evidence/IntegratedFull-20261016-01 | FAIL recording pace; other functional checks pass | 36 views; retained5s threshold exceeded by0.172412s |
| Evidence/IntegratedFull-20261016-02 | PASS full wrapper and all16 encounter/audio checks |36 views,13 station checks, streaming/wheel/save preservation; sheet and bridge views inspected |
| Evidence/FinalAutomation-20261016-01/index.json | PASS24 suites, zero warnings/failures | Final editor revision |
| Evidence/Shipping-20261016-05 | PASS build/cook/package | Win64 Shipping archive; package-manifest.json binds output files; full packaged qualification pending |

Failed attempts retained: dispatch-01 link failure (missing shared test-report implementation); dispatch-02 run bound insufficient for serial safe travel; editor-build-02 Unreal check macro collision. These were corrected without changing safety or audio thresholds. See exact retained logs.

## Compatibility and restoration

Content remains IR-S8-MSTBM-6; service IR-S8-MSTBM-service-7; normal saves remain ServiceV6; ServiceV5 retained. No checkpoint fields or roster changed. Derived watchdog state is intentionally not authoritative or serialized. S8 recoverable manifest/archive and exact restoration procedure: Evidence/Baseline-20261015/RESTORE.md. New source files must be handled separately when restoring; existing assets were not regenerated.

## Remaining gates

Eight authored mission foundations, Quick Play selection, complete mastery/UI/localization/night/rain and release-like packaging/performance qualification are not delivered by these changes. Two classes, AI freight and second corridor remain outside this continuation. Human usability, stock-specific listening, platform/yard/rake evidence and art acceptance remain pending. Android-specific gates BLOCKED/excluded. See scope audit, acceptance matrix and known limitations for complete G01�G12 / R01�R14 / N mapping.

## Integrated capture result

Final retry recording:73.237333s stereo, peak0.440155, RMS0.065508, no clipping. Explicit window73.266953 wall /73.266667 simulation seconds. A16second unnormalized crop is retained as opposing-pass-listening-16s.wav with source hash/crop telemetry; subjective listening remains PENDING. The actual pass is preserved, not a synthetic acoustic fixture.

Editor-only recording pacing now uses absolute simulation-tick deadlines instead of accumulating per-frame sleep overshoot. No audit threshold, game dynamics, source audio or normal-save format changed. Source review also corrected stale cached hold text after a route grant; warnings are suppressed immediately when authority is granted.

Visual inspection confirms continuous bank crests and retained route/stock presentation. Remaining toy-like glass, mechanical proportions, repetitive buildings, sparse foliage and engineered river shape prevent production-art acceptance. These are native Unreal fallback captures; the existing game-dev adapter only declares an origin scenario, so no sealed full-journey bundle is claimed.

## Packaged continuation verification

Shipping01 failed the empty profiling-macro conditional; braces corrected it. Shipping02 failed the existing GameFeatures Asset Manager cook rule; the installed engine rule was added. Shipping03 packaged but exposed briefing overflow. Shipping04 was interrupted and is not accepted. Shipping05 successfully built and cooked the corrected scrollable briefing and fixed action area. All earlier failures are retained.

FinalRegression-20261016-01 passes all 238 host cases; FinalJourney-20261016-01 passes all 57 journey cases on the final source. These do not replace packaged interaction checks. PackagedUI-20261016-02 verifies selection, readable briefing, confirmation and sound-settings navigation; 118 pre-existing checkpoint files remain unchanged. Service start, pause, Save and Exit, cold paused recovery and volume preference restoration now PASS after resolving the temporary window interruption. Cold recovery retained tick571 and an identical logical fingerprint. The current acceptance record is in that directory's VERIFICATION.md.

The final editor automation/full capture preceded the final widget/config corrections; their hashes identify the tested revision. Shipping05 compilation/cook and packaged UI observations cover the later layout/configuration changes. No full packaged journey or desktop performance qualification is claimed.

