# S9 acceptance matrix

Entry assessment: **partial desktop candidate, not qualified**. All rows refer to the complete requirement, not an isolated test. Integration must append concrete current-revision evidence before promoting any result. Historical S8 passes stay attributed to S8. G01–G12 are mapped in `S9_SCOPE_AND_ENTRY_AUDIT.md`.

Path prefixes: source = `03_UnrealProject/MyProject/Source/MyProject/`; E8 = `07_Documentation/S8/Evidence/EgmoreTambaram/`; E7 = `07_Documentation/S7/`. Baseline version B = content `IR-S8-MSTBM-6`, service `IR-S8-MSTBM-service-7`; desktop evidence run `OpposingFull-20261015-01`. S9 build identity must come from its manifest, never inferred from B.

| Requirement | Implementation location | Test/scenario and evidence | Build/content | Result | Limitation/blocker |
|---|---|---|---|---|---|---|
| R01 Offline full loop | RailServiceApplication.cpp, Simulation/RailService*, RailCheckpointStore* | E7/S7_IMPLEMENTATION_REPORT.md; fresh packaged cold offline service→result→restart needed | S7 historical; B entry | NOT RUN | No current release-like desktop full-loop evidence |
| R02 Shared truth/commands | Simulation/RailAIDriver*, RailHandling*, RailOperations* | AI/handling suites; E8/OpposingAutomation-20261014/index.json | B historical | NOT RUN | Re-run final S9 changes and shared-command equivalence |
| R03 Streaming-safe state | RailSimulationSubsystem*, Simulation/RailWorldSnapshot* | E8/OpposingFull-20261015-01/native/streaming-reload.json | B historical | NOT RUN | Prior actor replacement pass is not blanket new-roster proof |
| R04 Authoritative railway | Simulation/RailKernel*, RailOperations* | Whole-train conflict/lock/rear-clearance cases; E8 journey and automation reports | B historical | NOT RUN | Current S9 conflict/recovery matrix required |
| R05 5–10 minute Quick Play | Simulation/RailService*, RailServiceApplication/Widget | Authored slices, actual session-duration distribution with legitimate waits | S9 identity pending | NOT RUN | Full/accelerated capture duration cannot prove Quick Play pacing |
| R06 Causal interactions ≥90% | Simulation/RailAIDriver*, RailOperations* | Predeclared seed/scenario denominator with causally logged events | S9 identity pending | NOT RUN | No accepted denominator/result at entry; seeds are not missions |
| R07 Driving identity/quality | RailServiceAudio*, RailWheelPresentation*, cab/LHB assets | E8/OPPOSING-INTEGRATION-20261014.md; human handling/listening/visual review | B historical | BLOCKED | Stylized art, generic/synthetic audio and human reference review |
| R08 Supported-floor performance | Device profiles, renderer, streaming | Combined station/night/rain/traffic Android floor soak | Current candidate | BLOCKED | Android excluded; desktop never substitutes |
| R09 Data-driven variants | Simulation/RailContent*, RailFormationContent.h; S8 importers | Second distinct class and formation normal-path authoring audit | B historical | BLOCKED | WAP7/LHB-only boundary; class/role selection unresolved |
| R10 Saves/upgrades | Simulation/RailCheckpointStore*, RailSnapshotRecord*, RailService* | Corruption/write/storage/missing-content/version/settlement fault suite; E7 report, E8 isolated recovery suites | B historical | NOT RUN | Current final S9 schema/content-specific evidence required |
| R11 Explainable bounded AI | Simulation/RailOperationsAI.cpp, RailAIDriver* | Following/conflict/delay/deadlock/starvation/stopping and feasible completion suite | S9 identity pending | NOT RUN | S8 three-train success does not establish all interaction classes |
| R12 Content residency | RailProductionSubsystem*, streaming presentation/import catalog | Cook/dependency audit and repeated unload/residency traces | S9 identity pending | NOT RUN | Actor destruction alone does not prove asset release or bundle isolation |
| R13 Graphics-independent correctness | Simulation + presentation boundaries | Same seeded state/score/authority/completion across tiers | S9 identity pending | NOT RUN | Final configured tier comparisons absent |
| R14 Complete operational MVP | Full application and blueprint N | All N gates and no critical defects | Current candidate | BLOCKED | Full coverage, human gate and Android evidence absent |

## Blueprint N coverage at entry

| N coverage / acceptance scenario | Implementation/evidence location | Version | Result | Remaining gate |
|---|---|---|---|---|
| Two drivable classes | Current WAP7/LHB; S8/CURRENT_CONTINUATION.md | B | BLOCKED | Second distinct class decision/reference/audio/cab |
| Coherent three-role coach family; freight; three formations | Simulation/RailFormationContent.h; LHB source | B | BLOCKED | General/seating, guard/luggage and AI freight roles not established by all-3AC sample |
| Two corridors/six operational roles | Simulation/RailTambaramRoute.h; route/alignment data | B | BLOCKED | Existing intermediate scenery/stations do not demonstrate proposed branch-role topology |
| Regional three-treatment environment | RailTambaramCorridorView/StationView; E8 native views | B | NOT RUN | Current candidate visual quality/clearances/LOD and final source costs |
| Opposing/following/conflict; three AI plus player stress | Simulation/RailTambaramTests.cpp, RailOperations* | B | NOT RUN | B is two AI plus player; new stress proof and rendered following/hold required |
| Protected signalling and all operational stations | Simulation/RailOperations*, RailService* | B | NOT RUN | Current-content route, platform, timetable and complete-train lock tests |
| Eight authored mission foundations | Simulation/RailService* and application | S9 pending | NOT RUN | Stable mission data, prerequisites/failure/duration/recovery/tests and rendered proof per mission |
| Three-step mastery | Simulation/RailService* profile settlement | S7/B | NOT RUN | Existing single badge/reputation transaction is not three-tier career |
| Dry/light rain; day/night | Service/environment presentation | S9 pending | NOT RUN | Coherent adhesion/audio/save restoration and readable combined presets |
| Audio classes/layers/settings | RailServiceAudio*; 05_Audio/SOURCE_REGISTER.md | B | BLOCKED | Real gameplay stress, preference routing/mute and human stock/listening approval |
| Complete UI/cameras/controls | RailServiceApplication/Widget/View*, cab assets | B | NOT RUN | Current rendered menu→briefing→drive→results→retry/continue plus translations/accessibility |
| Save/offline | Checkpoint, record, service/profile systems | B | NOT RUN | Current packaged interruption/restart/storage/version matrix |
| Three profiles/device support | Config, S8 performance tooling | Current | BLOCKED | Android excluded; current desktop performance also unqualified |
| N human enjoyment/usability and provenance/localization | E7/USABILITY_PROTOCOL.md; S8 provenance/localization docs | Current | BLOCKED | Participant observations, full candidate rights/text review missing |

## Fresh entry baseline result

| Requirement | Implementation location | Test/scenario | Build/content | Evidence | Result | Boundary |
|---|---|---|---|---|---|---|
| Preserve S8 logical baseline before S9 edits | Simulation/RailTambaramTests.cpp and existing shared simulation | All 57 standalone Tambaram journey cases | B; exact input hashes in Evidence/Baseline-20261015/manifest.json | Evidence/Baseline-20261015/journey-results.json; journey-compile.log; Test-Baseline.cmd | PASS | Fresh baseline rerun only; does not qualify subsequent source changes, rendered/audio quality, packaging or full G01 |

The user has retained WAP7/LHB and the existing corridor for this continuation. Full-N missing class/route rows remain BLOCKED for full-MVP acceptance, without blocking authorized desktop work.

## Retained baseline evidence (historical PASS only)

E8/OpposingFull-20261015-01 contains `native/result.json`, `station-capture-timing.json`, `opposing-service-audit.json`, `save-preservation.json`, `native/streaming-reload.json`, `native/bogie-presentation.json`, `native/wheel-presentation.json` and native stereo recording/audit. The 36 views and 16 encounter checks support that bounded S8 scenario. E8/OpposingAutomation-20261014/index.json records 23 successes. E8/OPPOSING-INTEGRATION-20261014.md records 57 standalone journey cases. Rejected prior runs remain evidence of failures and fixes; do not remove them from provenance.

For every added S9 result record exact test name, command/seed, source/build/content hash reference, artifact path, PASS/FAIL/NOT RUN/BLOCKED/NOT APPLICABLE, and remaining limitation. Logical PASS cannot fill a rendered, acoustic, human or packaged gate.

## Desktop hardening tranche � 16 October 2026

| Requirement / location | Test and evidence | Result | Limits |
|---|---|---|---|
| Shared AI/dispatch safety; RailDispatchDiagnostics.h, RailDispatchTests.cpp | Dispatch-20261015-04/results.json:23 cases | PASS | Four-train logical fixture; no product save/render support for this roster |
| Measured causal interaction denominator | Six predefined following pacing/departure variants; 6/6 had actual resource denial then all3 AI completion | PASS |33minute fixture; not R06 5�10minute mission90% gate |
| Bounded hold diagnostics | Moving blocker, pause, backward clock and synthetic cycle tests in same report | PASS | No automatic route release or proven global deadlock resolution |
| Integrated regression | IntegratedAutomation-20261015-01/index.json:24 suites, no failures/warnings | PASS | Before final Shipping guard and cached-HUD suppression edits |
| S8 source baseline recovery | Baseline-20261015/recovery.zip, manifest.json, RESTORE.md | PASS | Source restoration requires rebuild; generated assets unchanged |
| Desktop cook inclusion | DefaultGame.ini Chennai/RunningGearV2 roots; DESKTOP_PACKAGING_AUDIT.md | NOT RUN | Source corrected; actual package/load verification pending |
| Continuous bank visual correction | RailTambaramCorridorView.cpp; IntegratedFull-20261016-01 | NOT RUN | Capture in progress; first bridge compared, complete run pending |
| Eight missions/mastery/Quick Play | Existing service remains compatible | NOT RUN | Not delivered by diagnostic/stress work |

Full G, R and N statuses above remain unchanged unless their complete acceptance condition has evidence; passing constituent tests does not sign off gates.

## Final editor capture acceptance - 16 October 2026

- PASS FinalAutomation-20261016-01:24 suites, zero warnings/failures, final editor revision.
- PASS IntegratedFull-20261016-02:36 views,13 station timings,16 real opposing/audio checks, reload/state, player wheel/bogie and normal-save preservation. 73.237333s stereo PCM peak0.440155 with no clipping. 73.266953wall /73.266667sim recording window.
- FAIL retained IntegratedFull-20261016-01 normal_simulation_pace only; threshold not loosened. Corrected absolute deadline pacing and reran.
- PASS bounded bank correction inspection: final contact sheet plus prior/fresh bridge comparison; remaining stylized artwork is not a G03 pass.
- PASS source asset existence audit:48 literal references. Actual cook discovery remains separate.
- NOT RUN subjective listening;16s original-PCM crop prepared. NOT RUN full mission/night/rain/UI/packaged performance acceptance. Shipping attempt currently running separately.

## 16 October packaged verification addendum

Shipping05 built/cooked successfully. PackagedUI-20261016-02 verifies corrected briefing layout and bounded selection/confirmation/start/pause/save-exit/cold-recovery/settings flow. Recovery tick571 and logical fingerprint match;118 baseline saves unchanged. Final host regression238/238 and journey57/57 pass. These are evidence increments, not human G-gate signoff or full MVP qualification. Full packaged journey/performance and remaining content/user gates remain open.
