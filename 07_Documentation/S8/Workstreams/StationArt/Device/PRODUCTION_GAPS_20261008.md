# S8 production gaps — 8 October 2026

Read-only source/contracts audit, recorded as a new StationArt handoff document.
No canonical asset, configuration, source code or existing document was changed;
no device, Unreal, build or regression process was run for this audit. This is a
completion proposal, not an acceptance report. Phone results produced separately
by the station owner supersede this document's snapshot of device readiness.

Scope is inside-station WAP-7 + LHB AC 3-tier gameplay. WAG-9 and BLC are deferred.
A second reusable scenario may use WAP-7/LHB again. Exterior polish is not the next
critical path. Desktop integration owns canonical changes; station chat owns ADB.

## Exact identity and evidence map

Paths below are relative to `D:/Project/IndianRailwayGame`.

| Item | Observed identity / contract | Evidence and production gap |
|---|---|---|
| Foundation catalog | `Content.Foundation.01`, version `0.1.0`, simulation `IR-S6-fixture-1`, scope `foundation` | `03_UnrealProject/MyProject/Content/IndianRailway/Production/manifest.json`: `assets` is empty; vehicle bindings remain `Vehicle.Proxy.01/.02`, definitions 1/2 version 1; formation `Formation.Foundation.01`. Existing foundation identity must remain valid. Introduce a separately versioned production catalog/binding without relabeling the old fixture. |
| Existing service | `Station.Shantipur.01`, `Platform.Shantipur.01`, `S7-Shantipur-01`, `IR-S7-service-1` | Same manifest: logical edge 2, platform 95–145 m, route 2, chunks 1/2. These are legacy fixture bindings, not Egmore. Preserve their compatibility behavior. |
| WAP-7 simulation | Definition `3000`, version `1`, content family `IR-S8-LHB-3A-1`; measured envelope `21.33043212890625 m`, bogie offset `5.85 m` | `Source/MyProject/Simulation/RailFormationContent.h` under the project: these are existing measured presentation dimensions, explicitly not certified prototype dimensions. Do not silently change this version to fix art proportions. |
| WAP-7 presentation | Runtime family `WAP7`, `schema_version: 1`, cab revision `cab-instruments-20261007` | `Production/locomotives/WAP7.json`: 15 parts, cab controls and gauges. This JSON does not declare a production `asset_id`/content version equivalent to the LHB contract. Resolve a stable catalog presentation ID linked to the exact source receipt and cab revision. Early `limitations` strings about no furnished cab/unmeasured Android cost are stale relative to later cab evidence; reconcile metadata without erasing history. Parked pressure/electrical gauges remain documented limitations. |
| LHB | Asset `Coach.LHB.Acquired01`, content `IR-S8-LHB-3A-1`, definition `3001`, version `1`; 24 m coupled length, zero extra gap | `Production/coaches/LHB3A.json` and `RailFormationContent.h`. `RailContinuity.cpp:75` binds the existing LHB service as `S8-Shantipur-LHB-01` / `IR-S8-LHB-service-1`. This is still Shantipur. Preserve it when adding Egmore. |
| Scene02 base | `Station.Egmore.Scene.02`, version `2.0.0` | `Production/stations/EgmoreScene02.json`; 35 source modules. Catalog status still says ready for source integration review although imports now have separate passing evidence. Do not confuse this source-stage label with runtime or visual acceptance. |
| Interior supplement | `Station.Egmore.Interior.01`, version `2.1.0`, base `2.0.0` | `Production/stations/EgmoreInterior01.json`; five added modules. Base scene SHA256 `c09777218b90a8d796f4a7a92f90e76905e41f1684d38378ab3712d014766cda`. Wayfinding/platform numbers are representative and need actual scenario binding. |
| Current assembled map | `/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study`; `ARailEgmoreSceneView` | `EGMORE_SCENE02_ARCHITECTURE.md`, `Evidence/EgmoreScene02/DESKTOP_REVIEW.md`, `Source/MyProject/RailEgmoreSceneView.cpp:101`: 40 modules / 1,316 placements; `staticArtReview=true`, `gameplayBound=false`. No route/service/save execution, authoritative signal binding or walkable collision. Stationary cab needles/controls are not simulation-bound. |
| Workload/profile | `Workload.Representative.01`, `Profile.Foundation.01`, `Material.Mobile.01` | `Production/workload.json`: defined-not-executed, missingProductionContent true, no executionEvidence. `profile.json`: foundation-S7-fixture, finalBudgetClaim false, three repetitions. `graphics.json`: all Low/Medium/High settings null, PROVISIONAL. Update production workload/profile identities and measured settings only with evidence. |

All `Production/...` and `Source/...` abbreviations in the table refer to
`03_UnrealProject/MyProject/Content/IndianRailway/Production/...` and
`03_UnrealProject/MyProject/Source/...` respectively. Documentation abbreviations
refer to `07_Documentation/S8/...`.

## Five remaining completion gates

1. **Playable Egmore service.** Add a separately identified scenario/route/service
   configuration, bind moving WAP7/LHB, platform stopping/door phases, operational
   cab and authoritative signal presentation. Reuse existing simulation/AI rather
   than introducing station-specific algorithms. Decide collision according to
   supported player movement; walking requires traversable platform/stair/bridge
   collision. Verify fresh start, service completion, save/reload, unavailable
   content rejection and recreation while old fixture identities remain intact.
2. **Interior and train-camera acceptance.** Use the diagnostic phone results to
   resolve canopy underside and fine cab-label darkness. Accept platform,
   waiting/wayfinding, stair, bridge and driver views; exterior view is secondary.
   Check clearance during movement/stopping/door use. The current 189.330432 m
   formation within a 216 m platform and 47 mm static body-edge difference do not
   certify swept clearance or prototype WAP7 dimensions.
3. **Night/rain/audio integration.** Source light lenses/emission are not mobile
   night lighting. Implement the four conditions below, bind audio to accepted
   commands/service/weather/residency, and record intentional limitations.
4. **Combined Android acceptance.** Measure the final playable scene with AI,
   bounded activity, quiet adjoining section, streaming and camera changes. Run
   repeated same-build captures, demanding condition and thermal/recovery checks.
   The passed cab baseline and single generic-station workload do not establish
   Scene02 combined performance. Derive usable tiers from those measurements.
5. **Catalog/release closure.** Register exact production assets, dependency
   versions, provenance, compatible identities and readiness evidence. Reconcile
   metadata and publish one final report only after visual/gameplay/cost gates pass.

## Four-condition acceptance matrix

These are proposed acceptance checks, not recorded passes. Every row needs the
same immutable APK hash, explicit scenario/content/service versions, configuration,
camera coverage, service outcome, telemetry and linked capture/audio evidence.

| Condition from workload.json | Required visible/functional checks | Measurement emphasis |
|---|---|---|
| `day-dry` | Legible interior signs/cab labels, canopy exposure, train/platform fit; moving approach, stop/doors/departure, signal aspects and normal audio | Baseline final-scene frame/GPU/memory and streaming behavior |
| `day-rain` | Wet surfaces and rain visible at player cameras; cab wiper presentation and visibility consistent with documented implementation; rain/rolling/station audio balanced | Weather/VFX/wet-material cost relative to same-build day-dry |
| `night-dry` | Platform/stair/bridge illumination, signage and cab instruments readable without excessive glare; operational lights and appropriate ambience | Lighting/shadow cost and readability at the same camera positions |
| `night-rain` (demanding) | Combined wet/light/rain visibility, signal discernibility and cab/wayfinding readability; audio layers remain intelligible during service | Demanding combined cost, sustained thermal behavior, recovery/streaming stability |

Keep normal and demanding activity settings explicit. The existing profile requests
6,000 frames, 30 FPS cap and three repetitions; document a final workload procedure
and actual usable frame counts rather than assuming the old fixture profile proves
this scene. Measure FPS/frame distributions, CPU/GPU, PSS, load/residency, activation
counts, thermal status and package size. Preserve original saves and phone settings.

## Provenance and readiness work that can proceed before another build

| Source | Existing evidence | Action for canonical owner |
|---|---|---|
| Selected WAP7 | `04_Assets/S8/Acquired/WAP7_Selected/receipt.json`: user replacement 2026-10-01, creator knitro_vedant, CC BY 4.0, source URL and archive SHA256 `3404dfaac45d595cf5ec519d85e12f87987c866bd6825436c182aebba2b8aa48` | Link this selected source, not the older WAP7 archive, to production presentation/cab records, attribution and modifications. Normalize into release provenance/readiness schema as needed. |
| LHB | `04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/provenance.json`: `Coach.LHB.Acquired01`, account-origin confirmation recorded, commercial/modification permissions verified, AcquisitionDate null, VerificationStatus pending review | Retain null date and missing receipt honestly; do not ask again for already-confirmed download origin. Preserve restrictions against reusable source distribution and identify unused third-party packed texture as unapproved. Close release review with existing evidence and documented limitations. |
| Scene02 / Interior01 | `04_Assets/S8/EgmoreScene02/2.0.0/provenance.json`, `04_Assets/S8/EgmoreInterior01/2.1.0/provenance.json`: project-authored geometry/materials; original-license label, source/reference receipts; no external photo pixels | Link the sealed source versions and canonical import corrections. Preserve separate reference-photo attribution. Source paid-provider cost of zero is not total implementation/Android cost. |

`Production/Schemas/readiness.schema.json` requires evidence/reason/status for 19
checks: identity/source/license/commercial/modification rights, scale/orientation/
pivot/materials/textures/LODs/collision, animation/audio/streaming/tier, Android,
regression and final approval. Populate records from actual evidence; a passing
import cannot stand in for pending audio, gameplay, mobile cost or final approval.
The foundation manifest currently has null assets for all eleven audio layers;
existing S7 sounds elsewhere do not establish production catalog bindings.

## Immediate recommendations to desktop integration owner

- Make playable Egmore bindings the next integration milestone after the phone
  diagnostic feedback. Keep the static study as a reproducible review artifact.
- Correct the production workload's stale `second-distinct-locomotive` requirement
  to reflect the approved second reusable WAP7/LHB scenario; preserve WAG9/BLC files.
- Prepare additive production identity/catalog/readiness records alongside the
  service work. `RailProductionSubsystem.cpp` currently validates foundation
  Shantipur bindings, so account for validator behavior rather than merely replacing
  strings in the manifest. Preserve foundation and LHB compatibility tests.
- Batch the required interior lighting/material fixes, gameplay bindings and
  weather/audio into meaningful integration candidates; exchange immutable paths
  and hashes through PHONE_HANDOFF.md. Do not spend this milestone on exterior
  detail or repeat the already-passed cab baseline without a relevant change.

Audit sources: `S8_TWO_CHAT_PLAN.md` (current scope/ownership), `PHONE_HANDOFF.md`,
`CURRENT_CONTINUATION.md`, `EGMORE_SCENE02_ARCHITECTURE.md`,
`Evidence/EgmoreScene02/DESKTOP_REVIEW.md`, current production JSON/contracts and
the provenance files above. The 2026-10-07 status audit is historical where current
handoffs supersede it. No final completion or new verification result is claimed.
