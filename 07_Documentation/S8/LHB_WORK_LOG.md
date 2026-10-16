# LHB passenger content — inspection and cleanup study

Current continuation (2026-10-03): versioned integration and Android validation
are complete; see LHB_VALIDATION_REPORT.md and Evidence/LHB/final-validation.json.
Final art acceptance remains pending. User authorized versioned integration;
implemented architecture is documented in LHB_ARCHITECTURE.md. Earlier entries
below are historical. Other S8 content is deferred. Higgsfield image request failed because
the connected account requires Basic or higher; no reference was generated.

Date: 2026-10-01, Asia/Calcutta. NOT ACCEPTED; no runtime integration.

## Scope

User clarified LHB first, then station/platform and infrastructure props. BLC
remains deferred; BLC_REFERENCE_GATE.md and its earlier evidence are unchanged.
No station, props, environment, additional locomotive or unrelated systems were
started while the passenger asset's provenance and technical gates remain open.

## Source and license

AssetId: Coach.LHB.Acquired01. Third-party source by shubham jha 3d
(shubhamjha201030), not original project geometry.

Source: https://sketchfab.com/3d-models/indian-railway-lhb-coach-c6c3d4313425487d91f6174a64dd96d9

Existing archive: C:/Users/nisha/Downloads/indian-railway-lhb-coach.zip.
No alternative model was downloaded. The archive contains one Blender file and
no license receipt. It was extracted to the separate Acquired/LHB_Sketchfab_c6c3d431
folder and the original archive/source were hashed. The source blend was never saved
over; Blender opened it with script execution disabled.

The exact listing API and HTML identify Free Standard. The full agreement was
retrieved after initial failed/empty requests, and saved locally. Sections 1.8,
2.1–2.4 cover derivative use and distinguish editorial restrictions. Commercial
and modification permissions are recorded as verified under those terms; standalone
redistribution is restricted. This does not establish the original downloader,
acquisition date or receipt. Those were requested from the user; provenance is
pending review, not release-approved. No NoAI/noimageai marker was found in the
retrieved model HTML. No external image-generation tool was used.

Evidence and schema-compatible provenance:
04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/provenance.json.
The schema check passed; it checks record structure, not legal authenticity.

## Actual source condition

- 225 scene objects: 135 meshes, 69 font objects, 17 curves, 4 empties.
- 646,498 base mesh triangles; 1,671,063 evaluated triangles including converted
  curves/fonts and viewport modifiers. These are scene totals containing two coaches,
  not a per-coach Android cost. Website face counts use a different evaluation.
- 26 mesh objects lack a UV layer. Presence of UVMap elsewhere is not proof of good
  UV packing, texel density or absence of overlaps. Detailed UV validation is pending.
- Body Cube bounds imply 12.025 m length and 2.936 m width at scene metric scale 1.
  A second body is a translated/rotated copy. Source wheel assembly bounds examined
  have approximately 0.735 m vertical diameter. These measurements are mesh bounds,
  not certified tread diameter or engineering dimensions.
- Railway LHB manual section 1.2 gives 23.540 m body length, 3.240 m maximum body
  width, 14.900 m pivot spacing, 2.560 m bogie wheelbase and 0.915 m new wheel diameter:
  https://secr.indianrailways.gov.in/uploads/files/1622203445123-MMLHB.pdf
  Those values were obtained from indexed primary-source text. Full drawing and
  subtype inspection remain pending. Independent per-component correction is needed;
  a uniform scale cannot reconcile the current measurements.
- No action datablocks or object animation data found. Wheel geometry exists, but
  production wheel/bogie pivot hierarchy and door movement are not prepared.
- Door, window, roof, gangway and underframe shapes are visible in the render.
  Detailed passenger interior and mechanical coupler completeness are not verified.
  Do not add a detailed interior under this task.
- Some bolts carry tens of thousands of triangles; wheel subdivisions also add
  substantial geometry. The source contains ngons (196 in the base body mesh).
- Cambria Bold font path is missing. Source text includes AC TWO TERM, AC TWO TER,
  3D AC and inconsistent numbering. Lettering is not approved as realistic signage.
- Unpacked indian rail.png is missing. A packed 4096-square texture points to a
  different Sketchfab import in its saved path; its material has no scene-object
  users. Neither dependency is authorized for reuse merely by being in the file.

## Work completed

Created read-only inspect_lhb.py and its JSON inventory/log/render. Created
prepare_lhb_review.py for a NON-RELEASE cleanup study: select the first coach by
inspected bounds, disable subdivision, evaluate mesh/curve/font geometry, apply
3-degree coplanar dissolve to bolt objects while respecting normal/material/UV
boundaries, and remove unused material/image datablocks in the candidate.

Output: Review/LHB_CleanupStudy.blend, cleanup-report.json, cleanup-study.png.
Selected single-coach source evaluation: 835,562 triangles across 111 objects.
Candidate result: 481,398 triangles (42.4% reduction for that selected coach).
This is an experiment, not an Android budget
or finished optimized asset. Selection and geometry need further visual review.
The render retains source materials and exposes the source lettering defects.
Dimensions remain deliberately uncorrected; silently stretching the full coach
would distort wheels and other components. No LOD, collision or production UV
claim is made. Missing font fallback in the study is not an accepted repair.

Actual effort categories: AI-assisted research, inspection scripting, local Blender
inspection and cleanup experiment, visual review, provenance/documentation.
No manual DCC artist work, purchased service cost or elapsed labor estimate is
claimed. Tool logs retain observed executions; no fabricated time/cost estimate.

## Integration and device status

Existing locomotive loader was inspected: it accepts only WAP7/WAG9 and locomotive
asset paths. A coach must use a formation-owned presentation path; renaming it to
pass the locomotive allowlist would be wrong. No such workaround was added.
No simulation, vehicle definition, manifest, save, authority, AI or station logic
was modified. No Unreal import, cook, package/install or S2–S7 regression run occurred.

ADB reports the I2304 connected. Device presence/API snapshots are saved alongside
source evidence. Presence is not an Android acceptance or performance result.
FPS/frame percentiles/thread times/GPU/PSS/residency/streaming/package/thermal costs
for the LHB are all unmeasured. Existing locomotive results do not apply to this coach.

## Remaining gates

1. User has confirmed direct download from the specified listing through their
   account. Evidence is user-source-confirmation.json, tied to the archive hash.
   Download date remains unknown and no receipt was supplied. The origin question
   is resolved; do not request the same confirmation again. Development can proceed
   under the inspected license. Release metadata remains pending because the existing
   release standard requires a supported acquisition date; do not fabricate one.
2. Resolve subtype/lettering and per-component dimensions from verified references;
   retain this model as the base, with explicit correction records.
3. Finish geometry/UV/material correction, moving-part pivots, LODs and collision;
   compare source/corrected model at player camera distances before accepting reductions.
4. Bind through the existing formation presentation with stable logical identity;
   test WAP7 + LHB, curves, braking, cameras, station approach, streaming/recreation.
5. Run S2–S7 regressions and measured Android build/profile/adjust/reprofile cycles.
6. Only after LHB acceptance proceed to the authorized station/platform and props.

LHB accepted: NO. Station/platform/props ready to begin: NO.

## Continuation: 2026-10-02

User selected AC 3-tier. Measured bolt simplification now yields 210,610 triangles
in Review/LHB_OptimizedStudy.blend. The script tests candidate reductions against
bidirectional sampled surface distances before retaining a result. This remains
a source-scale study, not accepted production content.

Runtime inspection found a structural integration issue: the live coach fixture
is 22 m with 16 m bogie spacing and is hard-coded into operations, service and
checkpoint reconstruction. See LHB_INTEGRATION_DECISION.md for exact call sites,
the proposed versioned content configuration and regression requirements. The
user's architecture-change stop condition applies; no simulation code was changed.
