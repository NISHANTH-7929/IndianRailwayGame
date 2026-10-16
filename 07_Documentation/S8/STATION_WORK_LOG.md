# Station, platform and infrastructure continuation

2026-10-03, Asia/Calcutta. User authorizes completing the remaining linked
content and clarifies: very close appearance, independently authored meshes and
textures, not the actual marketplace files. BLC stays deferred. Existing LHB and
S2–S7 identities and saves remain unchanged. USB stay-awake is enabled by request.

## Reference inspection

Public preview images are stored separately in
04_Assets/S8/References/StationPublic. They are visual comparison evidence only,
never imported textures or mesh sources. No marketplace package was downloaded.

- CGTrader station, njaalkailan:
  https://www.cgtrader.com/3d-models/architectural/other/indian-railway-station-591fad17-5784-4b2c-9e1f-8e70f2034be7
  Observed pale worn paving, concrete stair flights, thin metal handrails,
  steel diagonals and blue-grey sheet roof. Preview does not establish a survey.
- Fab platform pack, Poly Valley Studio:
  https://www.fab.com/listings/62f9bf38-d832-4399-ad6f-11d259d9135a
  Observed dark double-member columns, bolted splice plates, lattice roof
  supports, corrugated blue-grey canopy, red/cream column plinths.
- Fab infrastructure pack, Poly Valley Studio:
  https://www.fab.com/listings/2cc7e0fd-f03f-4877-9a74-e4747ed54dea
  Observed lattice OHE portals, cantilever arms/insulators, electrical cabinets,
  concrete sleepers, rail junction, yellow/black signs and striped posts.

Fab pages say Allows usage with AI: No. No source asset, UV, topology, texture
map or archive is used. The user explicitly clarified independent reconstruction
of visible real-world features. Reference images are not shipped in the game.
Color and unsurveyed structural dimensions are estimates, not certified values.

## Production scope

Reusable platform/coping/paving; steel canopy bay; waiting shelter; footbridge
deck and stairs; benches; railings; station façade; vendor kiosk; station signs;
lighting fixtures; OHE mast/cantilever/portal; electrical cabinet; speed/whistle/
termination/EMU boards; rails, sleepers, ballast and junction presentation;
adjoining ground and boundary treatment. A second arrangement must reuse this
kit. Lighting/weather/audio follow only after the visual pipeline is stable.

Every module needs metric dimensions, UVs, PBR surfaces, LODs, appropriate
collision, provenance, runtime ownership, review and Android measurement before
acceptance. S7 platform extent is authoritative; scenery cannot change scoring,
routes, train handling or station permission.

## Tooling

The optional game-dev CLI is absent from PATH and the plugin contains guidance
only. Its package/receipt workflow is unavailable. The existing Blender 5.2 and
Unreal 5.8 project scripts are the local production route; no remote provider or
new software installation substitutes for it. New source goes in
04_Assets/S8/StationKit; original acquired assets are untouched.

## Measured candidate checkpoint

The 26-module candidate was built, installed and measured on the connected iQOO.
Current APK SHA-256: 6fecf68b2aa8b8d2c941da93cfdb3e796674ffda8f663f5013f12dd92819d55a.
Both arrangements and the same-APK WAP-7/LHB baseline ran at approximately
29.96 FPS under the 30 FPS cap. Full metrics and limitations are in
STATION_VALIDATION_REPORT.md. Ten Unreal suites passed. Station save recovery
across the APK update preserved exact logical fingerprint 2c55c3509c5a4284.
Legacy S7 recovered fingerprint 0efccd7c539ec58a; LHB recovered completed Grade A
fingerprint ff7e0afb56b48956, with all nine objectives and one settled award.
The phone remains configured for USB-only stay-awake (setting 2).

## User-directed Egmore continuation

The user supplied nine real station sample images, then explicitly selected
Chennai Egmore. They are realism examples rather than exact station targets.
The measured candidate remains preserved; new Egmore source lives separately
under 04_Assets/S8/EgmoreKit. The heritage facade, domes, arcade and canopy are
being developed from Egmore-specific references. See EGMORE_REFERENCE_REGISTER.md.
Egmore is not yet runtime-integrated, and neither the candidate nor the new
source study is claimed as final high-realism acceptance.
# Continuation — 2026-10-04

- Installed V6 APK (SHA `e6161dab2c953836cbf41a5a16b6cbb9f8754c38e89ff511afe49aa85a5e71ae`).
- Initial station cold recovery passed: fingerprint `2c55c3509c5a4284`, tick 516,
  generation 9, 4,216 station instances and eight LHB vehicles recreated.
- Real-device V6 A/B captures: 29.970 / 29.969 FPS at 30 cap; frame p95
  34.538 / 34.536 ms; GPU mean 21.073 / 21.054 ms; peak PSS
  633.757 / 628.790 MiB. Normal save hashes unchanged in both runs.
- V6 train-only baseline slept near the end; retry hit keyguard. Both excluded,
  raw failures and successful checkpoint restoration retained. Asked user to
  unlock phone. Remaining commands: `profile_lhb.py WAP7 --label
  station-v6-baseline-final --apk 08_Builds/S8_Station/Android_ASTC/MyProject-arm64.apk`,
  then `verify_station_v6_recovery.py`, then `report_station_validation.py`.
- USB-only keep-awake remains 2. Phone was connected via wireless ADB and reported
  AC power when it slept, so the requested USB-only rule did not hold it awake.
- Egmore joinery, tower opening, open portico, relief and readable export lettering
  regenerated/imported. Source build 07 / import lettering v9 / review lettering v9
  logs retained. Five-module source/import validation passes with three LODs.
- Reviewed Unreal façade and canopy previews. These remain an architectural study,
  not a full realistic Egmore station or measured Android Egmore scene. The
  installed V6 APK does not contain this newer art revision.
- Generic-kit signs need the shared export lettering correction applied on their
  next regeneration. Complete Egmore layout, multilingual signage, materials,
  localized weathering, roof lighting, station fittings and scenery remain open.
- Review-map guard preserved repaired S7 hash throughout these edits; no
  simulation or formation algorithm changes in this continuation.

# Unlocked-device continuation and platform fittings — 2026-10-04

Completed the V6 recovery matrix on the real phone. Legacy generation 164,
LHB generation 49 and station generation 9 restored exactly, including their
service state and logical fingerprint. Left the station session paused. The
train-only comparison attempts `station-v6-baseline-final` and
`station-v6-baseline-resumed` lost foreground; excluded both and preserved raw
evidence. Station A/B measurements remain valid; no V6 baseline delta claimed.

Added six original Egmore modules: platform bench, drinking-water point, twin
litter bins, hanging platform board, three-metre drainage grate and ceiling fan.
Egmore now has 11 modules and 69,711 source triangles. Imported totals are
69,433 / 38,187 / 17,354 across three LODs. All source-hash, metric-bounds,
material-file, provenance and LOD checks pass. Dimensions of these architectural
fittings are estimates, not railway-standard certification.

Platform-level Unreal review corrected the water point/board facing direction,
lowered the hanging board, moved the drain away from the safety strip and fixed
positional Rotator arguments to named pitch/yaw/roll. The latter also corrected
the review sun direction. Final preview: `Evidence/Egmore/unreal-canopy-study.png`;
render log `review-fittings-placement-v12.log`. These are editor evidence only.

Regenerated/imported all 26 generic station modules to apply readable exported
glyphs; validation passes. Logs `source-build-lettering-v7.log` and
`import-lettering-v7.log`. V6 import/validation receipts preserved separately;
reporting binds the V6 report to those receipts rather than newer art bytes.
The installed APK has not been replaced by this newer art revision.

Remaining: station-specific layout and pavilion accuracy, multilingual signage,
less uniform PBR wear, roof lighting, proper signals, switch/junction detail,
OHE grade alignment, surrounding ground/vegetation and integration/Android
measurement of the revised Egmore scene. These remain unfinished; BLC remains
deferred. No authoritative railway algorithms or old content IDs changed.

# CLI installation and portable validation — 2026-10-04

User authorized installing game-dev if useful. Installed publisher release
1.1.0, checked published SHA-256, ran capabilities/doctor, configured the existing
Blender through a project wrapper. See GAME_DEV_TOOLING.md for the install path
and receipts. No paid provider calls.

Exported all 11 Egmore modules as portable GLB. Independent CLI checks found
missing tangents on some ngons. Fixed portable triangulation/tangent export;
all eleven now pass. Built and verified all eleven canonical packages, with
source provenance, hashes and explicit candidate status. Original .blend and
Unreal FBX remain unchanged by that operation. Source/import validation also
rerun successfully. This pass does not make the remaining station art complete.


# Egmore corridor, V17 rear elevation and device validation — 2026-10-04

Added two-track architectural review context, metric rail/platform placement,
OHE and original ballast geometry. Twelve modules now replace the earlier eleven.
Portable packages 1.1.0 preserve that corridor revision. The separate Egmore APK
built and rendered on the phone; normal checkpoints remained unchanged.

V17 adds estimated platform-facing windows, doors and cornices with a rigid
rotation (no reflected mesh winding), plus a 58-degree review camera without
forced aspect bars. Source/import checks pass; editor and actual phone renders
were inspected. Twelve portable 1.2.0 packages pass validation and verification.
All three old-fixture/LHB/station cold recovery cases pass on this APK.
See EGMORE_ANDROID_REVIEW.md for exact APK hashes and evidence boundaries.

USB-only keep-awake was restored to 2 when found reset to 0. The phone was
unplugged, so this setting cannot prevent unplugged sleep. It was subsequently
unlocked and V17 device validation completed without bypassing the lock.

V18 material work is a new candidate, backed up under BeforeAggregateV18.
Full realism, surveyed layout and Egmore train performance acceptance remain
unfinished. No simulation algorithms or old content identities changed.

# V18 completed validation pass — 2026-10-04

Added aggregate ballast maps, a review-yard soil surface and mast foundation
extensions. Twelve 1.3.0 portable packages validated/verified. New Android build
installed and actual Egmore images inspected. Normal save hashes unchanged.
Actual WAP-7/LHB/generic-station 6000-frame workload completed on that APK:
29.9634 FPS at 30 cap, p95 34.588 ms, mean GPU 21.2047 ms, mean PSS 602.42 MiB.
263 Android regression cases passed. Final old/LHB/station recovery retry passed.
See EGMORE_ANDROID_REVIEW.md for excluded failures and exact evidence/scene scope.
The telemetry sampler now handles brief empty/partial file reads with bounded,
recorded retries, without changing packaged game code. Current continuation
state and remaining realism work are in CURRENT_CONTINUATION.md. The full
Egmore station and all remaining assets are not yet complete or visually accepted.

# V19 yard material and lighting pass — 2026-10-05

Continuing from V18. Preserved the prior review map, script and preview images
under Evidence/Egmore/BeforeGroundV19. The new isolated M_EgmoreYardGround uses
world XY coordinates divided by 200 cm to keep a two-metre texture period on
the large horizontal ground slab. It reuses project-authored Soil PBR maps;
no photographic pixels or external assets were added. The separate rectangular
soil patch is removed. Added a modular maintenance railing and walkway beyond
the outer track, as estimated review context, not a surveyed station boundary.
Reduced review SkyLight intensity from 2.5 to 0.8; sun settings are unchanged.

Initial material scripting attempts caught API signature and input-pin mismatches
before map save. Both failures were retained. Fixed calls using the installed
MaterialEditingLibrary header and runtime input names; every connection now
asserts success. No simulation code, fixture identity, or save format changed.
V19 still requires completed render/device evidence before any acceptance claim.

V19 completed build/device checks: actual phone captures inspected; normal save
hashes preserved; all three recovery cases pass; 263 Android regression cases
pass. New build 20261005-084049. USB wake wrapper restored 2 after temporary 3.
See EGMORE_ANDROID_REVIEW.md for exact hashes and paths. No new performance
claim; latest measured playable workload remains V18. The full station art and
playable Egmore scenario remain unfinished, as recorded in CURRENT_CONTINUATION.md.

## 2026-10-05: V20 lighting and separate signage candidate

Audited the review camera and directional light. V19 used a 400 m / four-cascade
shadow setting against a two-cascade mobile profile. V20 narrows the review light
to 120 m / two cascades and sets manual camera/capture exposure, physical exposure
off, bias 1.5. Bias 0 was visually rejected as too dark and its render is retained
as lighting-v20-bias0-rejected.png. These settings are review-map-only.

Added Station.Egmore.Signage.01 v1 / Station.Kit.EgmoreNameboard4m as a separate
source/runtime family, not a change to the twelve heritage modules or gameplay
identity. The double-sided 4 m face is on estimated 3.6 m posts; total width 4.66 m
includes foundations. Source 760 triangles. HarfBuzz and FreeType shape Windows
Nirmala UI Bold to a 2048 x 1024 texture; font files are not redistributed.
Independent yellow paint texture, normal and ORM maps; no reference-photo pixels.
Lettering is historical-reference-based and is not a current-station survey.
Source/import hashes, metric bounds, three nonempty LODs and missing-glyph checks
pass. Build-LHB.ps1 -EgmoreReview now requires separate signage validation.
The GLB passed local game-dev policy and canonical package 1.0.0 integrity checks.
No simulation C++, old saves, service identities or S2-S7 maps changed.
V19 APK is preserved under 08_Builds/S8_EgmoreReview_20261005_084049.
Android validation of V20 is pending at this entry; do not treat local validation
as visual acceptance or a new performance measurement.

The initial signage close-up displayed fallback checkerboards despite correct
material slot bindings. Preserved as signage-uncompiled-rejected.png. A real-RHI
material recompilation and repeat close-up were initiated; static validation is
not sufficient evidence of material rendering. Import initially used -nullrhi.
The canonical package is separately verified; this does not certify Unreal pixels.

Final local close-up (unreal-signage-study.png) confirms readable, unmirrored
Tamil/English/Hindi lettering after real-RHI recompilation and FBX U correction.
Rejected fallback and mirrored/mip screenshots remain in Evidence/Egmore.
Canonical GLB digest is unchanged by the FBX-only repair:
d4063fda3bb2997f5a70e767ddc7ee38dc2db0de5d696d9a859036d9f6dc9045.
LOD triangle counts: 760 / 418 / 190. Android build started after this visual check.
The new sign's left post is partly occluded by a canopy column in this fixed close-up;
placement refinement and broader canopy lighting remain open visual tasks.

V21 grade change verification: Win64 Editor and Android compilation succeeded;
10 Unreal Automation suites passed (0 warnings/failures), 263 Android cases passed,
all three cold-recovery modes retained identities/fingerprints and recreated actors.
Performance attempt1 exited through HandleExitCommand and is excluded; retry uses
separate evidence folder, retaining failed run and save-restoration receipts.


V21 retry completed all 6000 frames (4795 analysis frames, 10–170 s window).
FPS 29.9687; frame p95 34.785 ms; GPU mean 21.1144 ms.
PSS mean 621.60 MiB, max 628.72 MiB.
Thermal status ranged 1–2 (mild/moderate). One run, no same-build baseline campaign;
not a sustained budget or Egmore-gameplay claim. Normal checkpoint hashes unchanged,
command line restored, phone wake2->3->2 restored.
Evidence: Station/Device/ohe-grade-v21-wap7-lhb-station-retry.
Combined verification: Station/v21-validation-summary.json. Art remains unaccepted.

## V22 candidate started
Previous V21 APK preserved at S8_EgmoreReview_20261005_092303. Source backup:
Evidence/Egmore/BeforeArchitectureV22. Tower side treatment adds estimated niches,
corner strips and cornice returns; not survey-certified side elevations.
Testing review-only sky fill0.45 against previous1.6 to reduce washed mobile
lighting. This is a candidate, not an accepted correction. No global renderer
settings, simulation definitions or saves changed. Desktop ES3_1 preview does not
match actual phone exposure/shadow appearance; phone verification remains required.

V22 import validation passed: source120596 triangles; imported LOD totals
120300 / 66164 / 30070. Source/import hash and metric-bounds checks pass for all
twelve modules. S7 map SHA256 remains
748ACD747B5E696E23531B7686932A83381A5DCA39B738C9B229B24A6B175D5F.
