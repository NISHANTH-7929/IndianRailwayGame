# Track and surroundings realism rework — 27 October 2026

This rework responds to the overly clean, block-like track and barren ground in
the previous desktop candidate. The final visual review and packaged checks
pass. Scope remains WAP-7/LHB on the existing Egmore–Tambaram corridor.

Run [Start Live Railway Journey.cmd](<D:/Project/IndianRailwayGame/08_Builds/S9_TrackRealism-Desktop-20261027-02/Windows/Start Live Railway Journey.cmd>).
Keep the whole Windows folder together. The previous build remains available
for comparison.

## Visual corrections

The original sleepers exposed their entire 170 mm body above the ballast. The
new tapered sleeper has raised rail seats, a lower centre, chamfered edges and
curved elastic fastening silhouettes. A shaped rail cross-section replaces the
three stacked boxes; only its running band is polished. Rail height, gauge,
sleeper spacing, route sampling and traffic/point logic remain unchanged.

The bed top is now 100 mm higher, embedding the lower sleeper body while keeping
the centre top visible. The flat shoulder extends 355 mm beyond each sleeper
end, followed by a wider, softened slope into soil. Four-centimetre overlap at
module ends closes the exposed wedges produced by adjoining chords on bends.

The first new review exposed an older texture-coordinate mistake: FBX imports
V as 1 minus the authored V. The shader now decodes that before its lateral
blend, so both shoulders and rail-seat stains use the correct local position.
The native editor audit verifies this against actual imported vertex UVs.

The replacement ground shader had silently ignored VegetationCoverage. Its
parameter is restored, and the initial flat green blend was replaced after
visual review with an original generated dry grass, straw and soil texture.
Low scrub and scattered gravel break up the broad central maintenance strip.
Drainage channels and jointed cable-trough covers sit lower on the ground and
use the weathered concrete finish. They remain clear of track and structures.

Concrete is darker with subtle dirt variation. Ambient skylight fill is reduced
from 1.0 to 0.6 while preserving sunlight; the actual cab view is checked for
readability. These are artistic renderer values, not physical illumination data.

## Final assets

Root: `/Game/IndianRailway/S9/TrackEnvironmentV1`.

| Asset | Detail levels / settings |
| --- | --- |
| `Meshes/SM_Track6m_RealismV2` | 4,432 / 968 / 208 triangles; full rails in every LOD |
| `Meshes/SM_Ballast6m_RealismV3` | 446 / 230 / 122 triangles; no collision |
| `Meshes/SM_AngularBallastStoneV1` | Existing 20 / 12 / 8 triangle angular stones |
| `MaterialsV4` | Seven shared rail, concrete, ballast, ground and fastening finishes |
| `TexturesV2` | Ballast, dirt and dry-verge sources; 1024-pixel runtime limit with mips |

Geometry is representative artwork. Normal PSC sleeper length is informed by
[Indian Railways' PSC sleeper reference](https://www.iricen.gov.in/ModelRoom/C7_PSC%20Sleeper.html).
The mesh keeps the existing 2.75 m length and 1.676 m gauge. Its exact sections,
fastenings, ballast profile and terrain are presentation estimates, not a
certified or surveyed reconstruction.

Authored sources: `04_Assets/S9/TrackEnvironmentV1/RailsV2` and `MeshesV3Final`.
Tools: `build_track_rails_v2.py`, `build_track_bed_v3.py`,
`import_track_realism_v3.py`, `import_track_rails_v2.py` and
`refine_track_realism_v4.py`, under `07_Documentation/S9/Tools`.
The final recipe runs V3 imports then the V4 correction. Earlier derivatives
remain available for comparison; they are not the runtime selection.

## Verification

Evidence paths below are relative to `07_Documentation/S9/Evidence`.

| Check | Result | Evidence |
| --- | --- | --- |
| Final editor compilation | Passed | `TrackRealism-20261027-02/editor-build-final.log` |
| Rail/bed imports and authored LODs | Passed | `TrackRealism-20261027-02/rails-v2-import.json`, `surfaces-v3.json` |
| Final material/UV correction import | Passed | `TrackRealism-20261027-02/surfaces-v4.json` |
| Fresh journey simulation run | 77/77 passed | `TrackRealism-Journey-20261027-02/journey-results.json` |
| Final scene review | 9/9 passed, 36 captures | `TrackRealism-Visual-20261027-04/render-results.json` |
| Shipping package | Passed, exit 0 | `TrackRealism-Desktop-20261027-02/result.json` |
| Cooked shaders and archive identity | All seven finishes in SM5/SM6; executable/pak/ucas/utoc hashes saved | `TrackRealism-Desktop-20261027-02/content-verification.json` |
| Two fresh-process recoveries | 2/2 passed | `TrackRealism-Shipping-20261027-02/result.json` |

The editor review checks populated components, compiled texture bindings,
actual imported bed UVs, active vegetation parameters and unchanged logical
state using isolated copies of nine checkpoints. The first intermediate review
caught flat green patches; the next caught visible curved-bed end seams and was
stopped intentionally. Only `TrackRealism-Visual-20261027-04` is the final set.
The imported lateral-coordinate error is below 0.001 m. All nine scenes report
active vegetation parameters and the expected compiled texture references.

The urban review uses approximately 26,258 close stone instances versus 74,916
in the previous candidate. This bounds detail cost while the rail mesh becomes
more detailed; it does not establish a frame-rate improvement. Static views
show opposing vehicles and crossover presentation, not a newly completed
moving service. No mobile qualification or sustained performance result is
claimed for this art pass.

Packaged verification requires scene readiness, unchanged paused checkpoint,
retained cab assets and the exact final mesh/material/texture paths on two
fresh process launches. All three textures must report 1024 × 1024, 11 mips and
streaming enabled. This is checkpoint recovery, not new reward writes or a
cold-cache performance benchmark. Normal player saves are not used.
Both launches recovered tick 9035 with the same logical fingerprint
`75ae0214f9bb165f`; all three textures reported the required dimensions, mip
count and streaming state. The editor component audits complement these
packaged cached-asset checks.

## Generated source and remaining limits

New source: [T_DryVerge_BC.png](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/Textures/T_DryVerge_BC.png).
It was generated with the built-in imagegen tool and copied unmodified into the
project. The [exact prompt and mode](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/dry-verge-generation.json)
are retained. Original ballast/dirt sources and prompts remain in the same
asset folder. These are generated material images, not photographs, measured
PBR scans or route evidence.

The wider city and vegetation still use procedural models. Building repetition,
simplified tree silhouettes and representative terrain remain visible; this
rework does not establish photorealism across the entire journey. The final
screenshots are actual game output and should be used to judge the improvement.
