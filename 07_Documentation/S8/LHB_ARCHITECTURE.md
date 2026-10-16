# Versioned LHB integration

Final integration/device evidence is in LHB_VALIDATION_REPORT.md and
Evidence/LHB/final-validation.json. The requirements below are implemented;
historical pending-device notes describe the earlier stages, not current status.

## 3 October device corrections

The installed newer WAP-7 cab build was recognized by SHA256 before testing;
its source/asset changes are retained in the subsequent LHB package. Device
review found a hardcoded 43 m briefing label: this now uses the live formation
length and platform extent. The unavailable locomotive switch is hidden for LHB.

Android logs confirmed missing InstancedStaticMeshes usage on coach materials,
which substituted default grey. The importer and saved materials now explicitly
enable that usage; window two-sided settings remain intact.

Map audit found historical static S8 review exhibits in the S7 map. A separate
/Game/IndianRailway/S8/LHB/L_RailService_LHB copy removes only those StaticMeshActors.
The original map hash is unchanged. Build and device scripts explicitly select
the new map. Logical service/snapshot identity is unchanged by this visual copy.

The offline profile schema now accepts the two explicit LHB workload scopes,
in addition to the existing foundation scope. This enables analysis of the
installed APK's CSV captures and does not change simulation/save schemas or
runtime content identities. summarize_lhb_device.py checks running telemetry,
advancing ticks and preserved old save hashes; excluded captures remain on disk.
validate_lhb.py derives androidMeasured from successful measurement evidence,
while releaseAccepted remains false pending final art acceptance.

One cold relaunch of the earlier cab APK failed before a new engine log appeared.
Symbolized frames point into Unreal memory tracking/allocator startup. A retry
restored generation 10, tick 1803, fingerprint 8ab5f7a44f48bbc2 exactly. This
intermittent failure is retained and is not claimed fixed by the material change.

2026-10-02, Asia/Calcutta. User authorized the configuration boundary change.
This supersedes LHB_INTEGRATION_DECISION.md. All other S8 content is deferred.

## Contracts

RailFormationContent.h is the immutable shared configuration. IR-S6-fixture-1
retains MakeFixture, original definition IDs, 20 m locomotive, 22 m coaches,
original bogie offsets and 1 m gaps. IR-S8-LHB-3A-1 introduces coach definition
3001 version 1: body 23.540 m, coupled length 24 m, bogies +/-7.450 m, bogie
wheelbase 2.560 m and new tread diameter 0.915 m. Length includes coupler
projection, so additional formation gap is zero; body ends are 0.230 m inside
coupling planes. Reference: https://secr.indianrailways.gov.in/uploads/files/1622203445123-MMLHB.pdf.

Definition 3000 version 1 binds the existing acquired WAP-7 to its measured
imported envelope, 21.33043212890625 m, and authored bogie pivots +/-5.85 m.
These are ASSET measurements, not certified prototype dimensions. Neither the
old definition 1 nor WAP-7 mesh is changed. Short/long formation footprints are
45.33043212890625 / 189.33043212890625 m (one/seven coaches). The WAP-7 source
differs from published prototype dimensions; detailed fidelity remains limited.
Mass/handling calibration is copied (80 t locomotive, 40 t coach), not newly
calibrated. RailKernel, RailHandling and AI algorithms are unchanged.

The LHB-only service berth head is 141 m, versus legacy 140 m. The same existing
50 m platform spans 95..145 m. At the new target the rear is 95.6696 m and the
front is 141 m, so the measured formation fits without changing platform assets.
The old target was correctly rejected after the locomotive envelope correction.

## Architecture changes

- OperationsWorld::InitializeContent validates/resolves before mutation;
  InitializeFixture remains legacy. New IDs receive existing handling parameters.
- ServiceDefinition defaults to legacy. S8 has separate service ID/version and
  content identity. Validate/platform fit and Start consume the same resolver.
- SnapshotRecord retains identity in its existing header; Read/Create reject
  unsupported IDs. Legacy layout is retained.
- WorldSnapshotCodec validates/rebuilds against resolved definitions, and restore
  initializes matching content. Service restore checks outer/nested identities.
- CommitCheckpoint rejects mixed-identity predecessor slots before writing.
- Unreal -S8LHB selects the new service and Saved/S8/LHB/Service; scripted review
  uses Saved/S8/LHB/AssetReview. Existing S7 saves remain separate.
- RailChunkView validates coach JSON identity/version/dimensions, then loads seven
  ISM components (body, two bogies, four wheelsets). Spatial state drives placement
  and bogie direction. Existing body-centre chunk ownership is retained; definition
  3001 suppresses the old coach proxy. Assets enter the retired-residency audit.
  Presentation collision is disabled. A separate coach directory is always cooked.
- The LHB service rejects another locomotive presentation and disables its
  appearance-switch action. Legacy services retain the appearance switch.
  LHB startup runs all 25 additional configuration tests on Android as well.
- JSON mirrors the C++ contract; it does not independently define simulation
  dimensions. A breaking dimensional change needs another content identity.

## Content and evidence

Original archive/source remain intact. Prepared/LHB3A_Candidate.blend corrects
axes, subtype lettering, body proportions, pivots and bogie/wheel positions,
removes unused data, unwraps UVs, reduces bolts and exports seven parts.
Nonuniform body correction addresses inconsistent source proportions. Gangway
ends are compressed to the coupling envelope, an approximation requiring review.
Source palette plus vertex variation is not a complete realistic texture set.
Wheel flange shape is inherited. No vehicle numbers were invented.

Engine review caught inward-facing source window panes that Blender rendered
two-sided. Materials Material.012 and Material.019 now explicitly render both
sides in the LHB importer. The before/after screenshots and side-face normal
inspection are retained under Evidence/LHB.

Imported triangle totals: LOD0 188689, LOD1 103771, LOD2 47170. These are measured
counts, not accepted Android budgets. Evidence/LHB retains original failures and
repaired runs. Automation world double initialization was corrected and actor
destruction now checks success with a registered world context. Static review
captures are not gameplay measurements.

Higgsfield generation returned "Requires basic plan or higher" on 2026-10-02;
no generated image or external source-asset upload was used.

The user requested desktop preparation first, then phone connection. Final
acceptance requires actual packaged workload, cold recovery and paired frame/
memory measurements on that phone. No device result is claimed prematurely.

The user subsequently connected the phone and authorized continuation. The
I2304 serial 10BE1Y2EL40003J is detected. Pre-install Saved data and checkpoint
hashes are preserved under Evidence/LHB/Device/pre-install. Host: 263 cases PASS
(238 legacy +25 LHB). UnrealAutomation-Berth: all nine suites PASS; an unrelated
editor HTTP connectivity timeout is retained in the report.
