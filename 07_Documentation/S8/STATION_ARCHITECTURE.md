# Station presentation boundary

The original simulation identities remain unchanged. Station scenery is selected
with `-S8Station -S8LHB`; `-S8StationB` selects a second arrangement of the same
modules. Save directories are S8/Station/Service and S8/Station/AssetReview,
separate from original S7 and LHB records. The new map is
`/Game/IndianRailway/S8/Stations/L_RailService_Station`; old maps remain intact.

## Runtime changes

- RailStationPresentation.cpp adds ARailChunkView::LoadStation. It reads the
  station JSON, resolves mesh references before component creation and instances
  repeated modules. Chunk actor ownership follows the existing x=0 split.
- RailChunkView owns StationParts as reflected strong references. In station
  mode it suppresses the 512 diagnostic cubes and cyan track debug lines.
  Legacy mode retains its original rendering. Rails, sleepers and contact wire
  sample existing graph geometry. These meshes have no collision/authority.
- RailContinuity waits for station components when this mode is requested,
  records component/instance counts, and includes station meshes in retired
  reference audits. No route, clock, handling or AI algorithm changes.
- RailServiceView suppresses its proxy platform and debug berth geometry in
  station mode. The actual S7 platform contract remains 95..145 m. Its controls,
  objectives and signal semantics remain in the existing service UI.
- DefaultGame cooks the separate Stations directory. Build-LHB.ps1 -Station
  archives to S8_Station, preserving the earlier S8_LHB package. Profile harness
  selects the station map/save directory only with --station; both old S7 and
  normal LHB checkpoint hashes are checked before/after measurement.
- StationPresentation automation tests both arrangements, idempotent loading,
  destroy/recreate, mesh references, disabled collision and unchanged snapshot.

## Asset contracts

Source: 04_Assets/S8/StationKit/StationKit.blend and Parts/*.fbx. New 512 px
tiling PBR sets include base color, tangent normal and ORM (AO/Roughness/Metallic).
Fourteen reusable materials, 26 mesh modules. Texture and geometry source is the
reproducible build_station_kit.py script, with no external images read by it.
Unreal imports three LODs at 100/55/25 percent targets; actual triangle totals
are recorded separately. Query collision boxes are stored on eligible meshes;
runtime scenery disables collision. This is not navigable station gameplay.

Broad gauge is authored as 1.676 m between inside rail-head faces. Platform
surface is 0.84 m above rail and coping 1.675 m from track centre. Reference:
https://irimee.indianrailways.gov.in/instt/uploads/files/1454673172611-sod.pdf
These selected design constraints do not certify all railway engineering details.
Remaining architectural dimensions and colors are visibly estimated from public
reference previews; they are not measurements of the sellers' models.

Non-operational speed and EMU sample boards are kept in the source kit rather
than placed as invented restrictions on the live route. Electrical equipment
and contact wire are presentation only. Junction rails follow the graph, not a
new points/authority implementation. Additional switch detail and final signal
appearance require visual review; current signal debug indicators retain meaning.

## Acceptance

The revised station exterior camera uses the opposite side of the train to avoid
canopy occlusion; other modes retain their existing camera position. Scenery
telemetry counts actual proxy instances rather than assuming 512 per chunk.
The importer records SHA-256 hashes of its geometry, textures and assembly;
station packaging rejects a missing or stale import receipt before cooking.
The stair flight now has a closed stepped profile with an inclined 220 mm
soffit, and station-board lettering is authored on both faces. Texture variation
uses periodic interpolated random fields to remove obvious sinusoidal bands.

2026-10-04 correction: the FBX force-front-X conversion reflected authored Y.
The exporter now compensates Y only for exported geometry, restores its source
mesh afterward, and reverses winding for each reflection. The importer rejects
any module whose imported bounds differ from authored bounds by over 1 mm.
Automation checks the actual placed platform coping, top and extent. Earlier
v5 captures remain performance evidence, but their visual platform alignment
was wrong and is superseded by this correction. Source-only dimensions were not
sufficient evidence of imported placement.

The Egmore review script also now checks creation/loading success and exact world
identity before editing. An earlier rerun failed to create an already-existing
review map and added 28 actors to S7's editor map. Those exact additions were
removed, retaining all 28 original S8 exhibits. Base actor transforms/classes
match the earlier clean LHB map (s7-base-actor-audit.json). S7 was reserialized;
its original byte hash is not claimed. No APK containing that mistaken map was
built or installed. The installed v5 legacy and LHB recovery evidence predates
that editor-only event; the rebuilt candidate is validated separately.

Technical validation is recorded in Evidence/Station. Import or metadata passes
do not establish close visual likeness. No final high-realism or full S8
acceptance is asserted until the actual player-camera review and Android tests.
# 2026-10-04 continuation: Egmore art review and V6 device checks

The Egmore study remains an independently versioned art package and a separate
editor review map. No Egmore geometry replaces the Shantipur service or changes
the logical content identity. Recessed window joinery, louvred lower doors, open
portico arches, tower openings and raised frieze/pediment details were added.
The review map now includes platform/apron modules and a second canopy camera.
The inner apron deliberately has no yellow edge line.

The shared Blender exporter now tags text meshes and reflects their glyphs in
their local reading direction on a temporary export mesh. This compensates for
the handedness difference while retaining the already validated XYZ placement.
It preserves the unmodified readable Blender source and reverses the affected
face winding before FBX conversion. This was applied to the Egmore revision;
the installed V6 generic-kit APK predates this lettering change. Existing
generic station signs still require regeneration and a visual lettering audit.

The Egmore source/import bounds and three-LOD checks pass. This is not visual
acceptance, a survey-accurate station plan, or an Egmore Android measurement.
Its newer art assets are not in the measured V6 APK. The separate review-map
guard remains mandatory; the repaired S7 map hash stayed
`748ACD747B5E696E23531B7686932A83381A5DCA39B738C9B229B24A6B175D5F`.

The V6 Android station A and B captures both completed with normal checkpoint
hashes unchanged. Initial station cold recovery restored fingerprint
`2c55c3509c5a4284`, tick 516, generation 9. The first train-only comparison lost
focus when the device slept; its retry encountered the keyguard. Both failed
captures were excluded and their restoration logs preserved. Further device
validation awaits unlock; the USB-only stay-awake setting remains 2.


## Egmore review boundary revisions (2026-10-04, V17/V18)

Build-LHB.ps1 accepts -EgmoreReview with -Station to cook the isolated art-review
map into a separate S8_EgmoreReview archive. It preserves the existing launch
mode, fixture definitions, operations and save identities. Static review
capture backs up/restores the device command line and compares all six normal
checkpoint file hashes. V17 actually rendered on device; see
EGMORE_ANDROID_REVIEW.md for exact APK and recovery evidence. Earlier paragraphs
about pending Egmore device capture refer to their historical revisions.

The cold recovery helper now takes --prefix, allowing new APK evidence without
overwriting earlier V6 reports. This changes evidence filenames only.

V17 rear joinery uses rigid rotation of newly generated objects, preserving
normals and UV orientation. Existing mesh IDs stay stable. Its extra geometry
is reflected in fresh LOD counts and separate portable package version 1.2.0.
The review CameraActor alone uses a 58-degree FOV and unconstrained aspect.

V18 introduces an opt-in aggregate material kind in the procedural source helper.
Existing kinds keep their original 512px generation path. Only EgmoreStone opts
into a 1024px periodic cellular field, with shared boundaries in base colour and
normal relief. This does not change the generic station's imported assets or
any railway simulation code. Source and prior review images are backed up.
This remains an art candidate; source validation is distinct from visual and
Android acceptance.

## V19 review-only material and layout boundary — 2026-10-05

Added M_EgmoreYardGround under EgmoreHeritage01/Materials. The review map holds
its reference, so it cooks as a map dependency. It samples existing original
Soil BC/N/ORM textures with world XY / 200 cm rather than stretched slab UVs.
It is intended only for horizontal ground. Mesh source packages and runtime
formation definitions are unchanged. Material-expression connections assert
success using input names discovered from the running engine.

The review map replaces its rectangular soil overlay with continuous ground,
adds a 99 m maintenance walkway and 33 three-metre railing modules outside the
outer track, and supplies foundations under both electrical cabinets. These
are estimated architectural context, not railway infrastructure certification
or a surveyed Egmore boundary. Track centres, rail plane and platform remain
unchanged. The final SkyLight fill is 1.6 (2.5 previously); a too-dark 0.8 trial
was retained as evidence and rejected after canopy review.

Test-EgmoreDevice.ps1 wraps installation, static capture and cold recovery on
the authorized wired phone. This phone reports wired charging as AC, so the
wrapper temporarily enables AC+USB wake (3), then restores and verifies the
previous setting in finally. It never unlocks the device or disables its lock.
The existing USB-only requested setting remains 2 outside the validation run.

V20 candidate adds an independent EgmoreSignage01 asset family and JSON with
Station.Egmore.Signage.01 v1. The review script consumes it explicitly; it is not
inserted into the old station/formation configuration or checkpoint identity.
A separate validation gate checks source receipt, shaped text and imported bounds.
Review-only exposure/shadow changes are serialized in L_EgmoreHeritage_Study.
No algorithm, save schema, platform logic or service initialization changed.

V20 sign review found an FBX-handedness mirrored texture and low streaming mip.
The export now flips only the temporary FBX face U coordinates; source/GLB UVs
remain unchanged. The one 2048 x 1024 BC name atlas is explicitly resident in
Unreal; normal/ORM streaming settings remain default. This adds resident texture
cost which must be measured before any performance acceptance claim.

## V21 presentation-only OHE grade correction

The optional generic-station presentation now samples twelve explicit fixture
track anchors on edges 1, 2 and 7 instead of placing all mast feet at z=0.
The four climbing-track masts follow true 3D arc distance at 40/90/140/190 m,
so their foundations rise 0.8/1.8/2.8/3.8 m. Lateral arms remain 3.3 m from
track centre and 5.5 m vertically above the sampled rail; masts remain vertical.
No RailKernel, RailHandling, RailOperations or AI algorithms changed. No identity,
checkpoint schema, service/platform definition or fixture geometry changed.
Existing station Automation test now checks all twelve placements, grade,
contact height and verticality during both variants and actor recreation cycles.
V20 APK and pre-edit C++ files are preserved. Build/tests pending at this entry.

## V22 geometry boundary
Only EgmoreTower4m geometry changes: 18,936 -> 29,538 source triangles. Side
niches, vertical trim and cornice returns are presentation estimates based on the
heritage vocabulary; side elevation measurements are unavailable. Total kit source
triangles120,596; twelve stable module IDs and station asset identity retained.
No formation, service, platform, checkpoint or simulation code changes.
Review SkyLight fill candidate0.45 affects only L_EgmoreHeritage_Study.
Old source/import receipts and map are under BeforeArchitectureV22; validated V21
APK archived before packaging. Pending import/visual/Android evidence is not
release acceptance. Historical portable1.3.0 packages describe the earlier source.
