# S8 representative content: BLC reference gate

Review date: 2026-10-01 (Asia/Calcutta).
Status: STOPPED BEFORE MODELING — reconstruction reference unresolved.

## Reference versus original game asset

The requested CGTrader listing is a reference target, not an acquired asset:
https://www.cgtrader.com/3d-models/vehicle/train/indian-railways-blc-container-flat-wagon

No original BLC game asset was created in this pass. No seller geometry, texture
maps, UVs, topology, archives or purchased files were downloaded or converted.
No Fab assets were accessed for generation or modification. No license approval
or commercial rights verification for any marketplace asset is asserted.

## Evidence and unresolved configuration

The listing text specifies 14,566 mm over couplers, 2,200 mm width, 1,009 mm
platform height and CASNUB 22 HS bogies. These are seller assertions, not verified
engineering measurements. The page's public image links were resolved; initial
image fetches timed out. A later direct image request returned a reference without
a model-visible image. No visual comparison, color sampling or geometry inspection
was completed. Do not interpret the listing's realism claims as validation.

Manufacturer reference inspected:
https://jessop.co.in/wagons/blc.php

Jessop distinguishes A and B cars: respective headstock lengths 13,625 and
12,212 mm; 840 mm wheel tread diameter; 1,676 mm gauge; 1,009 mm platform height.
It specifies one CBC and one drawbar for A, and two drawbars for B. This makes
an undifferentiated BLC with identical ends an unsupported modeling decision.

Railway technical reference located:
https://rdso.indianrailways.gov.in/works/uploads/File/TECHNICAL%20PAMPHLET%20No.%20G-112.pdf

Search-index text identifies LCCF 20(C) under BLC wagons. The PDF fetch timed out;
full-document inspection remains pending. This flags a discrepancy with the
listing's bogie description, rather than proving which configuration its images
actually depict. Another located reference, also not successfully fetched:
https://rdso.indianrailways.gov.in/uploads/CH-10.pdf

## Stop condition and resumption evidence

The user's explicit instruction is to stop when reference information is
insufficient for accurate reconstruction, and not silently make assumptions.
The current evidence does not establish a consistent target variant and bogie.
Do not manufacture a hybrid to reconcile the references.

Before modeling, establish the intended A/B variant and bogie from an inspected
general arrangement drawing and matching real-world photographs. Obtain readable
side, deck, both ends and bogie views sufficient for visible component placement,
including couplers/drawbars, brake equipment and twist locks. Resolve the listing
discrepancy in this record. Record source-specific confidence; color estimates
must remain estimates. Public reference availability does not grant permission
to incorporate source images as textures.

## Existing integration contract inspected

Reviewed IMPORT_STANDARD.md, PROVENANCE_STANDARD.md, ANDROID_BUDGET_METHOD.md,
the production manifest and RailVehicleDefinition.h. The main manifest currently
has an empty assets array and proxy vehicle bindings. This observation does not
describe every separate locomotive review pipeline in the project.

Vehicle definitions carry stable identity/version, length, mass and front/rear
bogie offsets. A future BLC needs verified dimensions and a normal definition/
presentation binding; do not overwrite a proxy's dimensions just to fit the mesh.
Keep train movement, VehicleId, station contracts and save authority unchanged.
No architecture change has been demonstrated necessary or attempted.

When the reference gate is resolved: create independent Blender geometry, UVs and
PBR materials; document provenance; validate scale/orientation/pivots and visual
comparison; import using existing manifest/streaming ownership; validate actual
Unreal loading and regressions; then measure the real content on the existing
iQOO Neo 9 Pro Android 16/API36 ARM64 Vulkan ASTC harness. Follow the existing
paired-run method. No polygon or texture budget is approved in this record.

## Work and acceptance record

- Actual work: request review, local contract inspection, public listing and
  manufacturer research, technical-reference search, discrepancy documentation.
- AI assistance: research and this written assessment only.
- Blender/manual modeling/correction/import/optimization: not performed.
- Elapsed labor, monetary cost: not instrumented; no estimates asserted.
- Created assets: none. Unreal/source/runtime manifest changes: none.
- Android cost: unmeasured; no BLC build, install or profiling run performed.
- Tests: not run; documentation-only change. No regression pass claimed.
- Screenshots/render comparisons: none produced; visual acceptance pending.
- Content #1 accepted: NO.
- Content #2 ready to begin: NO; BLC reference, build, integration and measurement
  gates remain open. Station, props, environment and later systems were not begun.
