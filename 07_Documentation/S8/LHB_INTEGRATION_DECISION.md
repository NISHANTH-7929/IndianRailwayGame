# LHB formation integration decision

Historical proposal, superseded by user authorization and implementation.
See LHB_ARCHITECTURE.md for current contracts and limitations. Latest scope
defers all other S8 content; the old stop condition below is historical.

2026-10-02 (Asia/Calcutta). AC 3-tier selected by the user.

## Verified structural mismatch

RailKernel.cpp: MakeFixture() constructs definition 2 with length 22 m and bogie
offsets +8/-8 m. Short()/Long() use this definition with a 1 m formation gap.
OperationsWorld::InitializeFixture() constructs both trains from that fixture.

The LHB reference dimensions are 23.540 m body, 24 m over CBC, 14.900 m bogie
centre spacing (offsets +7.45/-7.45 m), and 2.560 m bogie wheelbase. Reference:
https://secr.indianrailways.gov.in/uploads/files/1622203445123-MMLHB.pdf
These are physical dimensions, not instructions to reinterpret the existing
formation's length/gap convention. Coupler spacing must be mapped explicitly.

ServiceDefinition::Validate() derives its platform-fit properties from
MakeFixture().Short(). ServiceRuntime::Start() calls InitializeFixture().
WorldSnapshotCodec::Restore() also initializes this fixture; ValidateAndRebuild()
checks its expected vehicle IDs, definition IDs, orientations and formation gap.
SnapshotRecord accepts only the existing ContentManifest::ContentId.

A new URailVehicleDefinition or visual manifest entry alone does not replace
these runtime definitions. Thus scaling a new visual onto the old coach would
not satisfy accurate dimensions, bogie locations, formation footprints and save
compatibility together. This is a code-inspection finding, not a failed test.

## Concrete proposed integration change

Add a versioned LHB content configuration consumed consistently by operations
initialization, service validation/start and checkpoint restoration. Preserve
the old fixture and its content identity for existing saves/tests. Give the LHB
definition its own ID/version and explicitly specify physical dimensions and
the coupling-gap convention. Bind only compatible formation members to LHB
presentation. Keep the motion, handling, authority and AI algorithms unchanged.

The implementation must reject unsupported content identities without overwriting
their saves. Test old fixture recovery and new LHB capture/restore separately,
including dimensional/formation incompatibility rejection, platform fit, train
footprints and all S2-S7 regressions. Then run the actual WAP7+LHB Android workload.

This touches the S6 recovery and S7 service content contracts, beyond a narrow
presentation-only adapter. It has NOT been implemented. The original request's
explicit architecture-change stop condition requires a scope decision first.

An alternative is an isolated Unreal visual-review scene for asset/LOD profiling
only. That can preserve all existing contracts, but cannot count as successful
WAP7+LHB service, formation, streaming or recovery integration.

## Preserved work

Source identity confirmed by user; unknown download date is not a development
blocker. AC 3-tier is the target; existing source lettering is not yet corrected.

Measured bolt-simplification study created:
04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/Review/LHB_OptimizedStudy.blend

The selected source coach evaluated to 835,562 triangles. Initial cleanup yielded
481,398; measured bolt simplification yielded 210,610 (74.8% below selected source).
Trials and sampled surface errors are in Review/bolt-optimization.json. The
1.5 mm error threshold applies to source-scale geometry, must be reassessed after
dimensional correction, and is not a final Android budget. Sampling does not prove
a global Hausdorff bound or visual equivalence.

No corrected production scale, accepted LODs, runtime integration, Android LHB
performance, regression pass or content acceptance is claimed. No simulation
source was edited. Station/platform/props remain ordered after LHB acceptance.
