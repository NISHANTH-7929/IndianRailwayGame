"""Write the category matrix and pipeline documentation; no production assets."""
from pathlib import Path
DOC=Path(__file__).resolve().parents[1]

def write(name,text): (DOC/name).write_text(text.strip()+'\n',encoding='utf-8')

categories=[
 ('locomotive','Vehicles/Locomotives','Loco','SK','blend + FBX','SkeletalMesh','body centre on rail plane; bogie/wheel attachment markers','query-proxy','body, bogies and cab independently reducible; retain silhouette'),
 ('coach','Vehicles/Coaches','Coach','SK','blend + FBX','SkeletalMesh','body centre on rail plane; coupler and bogie references','query-proxy','shared body/material family; preserve door alignment'),
 ('wagon','Vehicles/Wagons','Wagon','SM','blend + FBX','StaticMesh','body centre on rail plane; coupler/bogie references','query-proxy','preserve loading outline; shared freight materials'),
 ('formation','Data/Formations','Formation','DA','JSON','DataAsset binding metadata','ordered stable vehicle presentation IDs; no spatial pivot','none','no meshes/textures; individual members own LODs'),
 ('station','Stations','Station','DA','JSON + FBX','DataAsset and modular StaticMesh set','survey/reference origin; platform edge markers','simple-proxy','modular instancing; station-area owner'),
 ('trackside','Environment/Trackside','Trackside','SM','blend + FBX','StaticMesh','base/track-offset reference; never signal truth','simple-proxy','instancing and distant silhouette; safety semantics remain logical'),
 ('environment','Environment/Kits','Environment','SM','blend + FBX','StaticMesh','tile origin and documented seam bounds','simple-proxy','instancing; optional distant pieces unloadable'),
 ('building','Environment/Buildings','Building','SM','blend + FBX','StaticMesh','ground-floor origin on kit grid','simple-proxy','shared facades; distant simplified silhouette'),
 ('vegetation','Environment/Vegetation','Vegetation','SM','blend + FBX','StaticMesh','trunk base at ground; upright +Z','none','alpha overdraw review, instances and reduced distant cards'),
 ('character','Characters','Person','SK','blend + FBX','SkeletalMesh','feet origin; skeleton/root and hand markers','query-proxy','shared skeleton; measured animated crowd cost'),
 ('animation','Characters/Animations','Animation','AS','blend + FBX','AnimationSequence','declared skeleton/root; no authoritative root movement','none','sample/compression profile and skeleton compatibility'),
 ('audio','Audio','Audio','SW','lossless WAV master','SoundWave/profile','spatial emitter anchor; no mesh pivot','none','semantic layers, concurrency and streaming residency'),
 ('vfx','Environment/VFX','Vfx','NS','Niagara source + texture masters','NiagaraSystem','emitter local origin + attachment marker','none','mobile shader support, particle/overdraw measurement'),
 ('ui','UI','UI','T','SVG/PSD master + PNG','Texture2D/Widget','documented anchors, DPI and safe area','none','UI texture group, Devanagari coverage, touch readability')]
intro='''# S8 content standards

Status: PROVISIONAL -> MEASURE -> ACCEPT -> FREEZE. These are import/identity rules,
not measured production budgets. The category matrix below applies together with
the common rules; exemptions must be recorded in readiness evidence.

## Common contract (all categories)

Content root is `/Game/IndianRailway/Production/`; folders below are conceptual
destinations created only when content arrives. Existing S2-S7 assets stay in place.
Logical IDs are `<Category>.<Family>.<Variant>` (case-sensitive, unique, no path or
revision embedded); replacement keeps the logical ID. Never reuse retired IDs for
different content. Asset file names are `<Prefix>_<Family>_<Variant>[_Part]`.
Unreal paths can change independently; manifest references between records use IDs.
Simulation DefinitionId/VehicleId/TrainId remain separate and unchanged.

Source geometry declares units; import bakes to centimetres (1 Unreal unit = 1 cm),
positive +X forward, +Z up, +Y right. Rail simulation retains metres and its existing
100 cm/m conversion. Freeze object scale before export and verify dimensions against
the definition; never modify physics dimensions to fit an incorrect mesh. Nonspatial
categories explicitly declare scale/orientation/pivot not applicable in readiness.

Use shared mobile-compatible base materials with instances. Declare every material
slot and texture, and measured category/tier limits when accepted. Until then
`materialSlotLimit` and dimension limits remain null and cannot be called frozen.
Textures use `T_<Family>_<Variant>_<BC|N|ORM|E|A>`; BC/E color are sRGB, N and packed
ORM are linear, A follows documented UI/VFX use. Texture groups distinguish World,
Character, UI and effects. Android target is ASTC with mipmaps for world textures;
UI exceptions need review. No compulsory virtual textures/Nanite/desktop-only shader
features. Inspect normal orientation, channel packing, alpha and compression on device.

Mesh LOD source names end `_LOD0`, `_LOD1`, etc., imported into one logical mesh.
Declare LOD0 and measured reductions, transition policy and silhouette review. A
single LOD receives a review warning until justified. Non-mesh LOD declaration is
`not-applicable:<reason>`. No arbitrary final triangle, slot or texture budgets.
Collision is a simple/query proxy, never railway authority or a second train physics
system. Complex render meshes must not become simulation collision truth.

Each asset declares an existing chunk ID, essential flag, platform/tier list and
dependencies. Soft paths belong to the presentation owner; release handles and strong
references on unload. Shared dependencies may survive only while another owner needs
them. Optional scenery cannot own signal/occupancy/station truth. Vehicle/audio
packages are lifetime groups, not new simulation or nationwide streaming systems.

Every external asset has complete provenance under the manifest asset record using
Schemas/provenance.schema.json. Creator, owner, source/archive, date, license, rights,
restrictions, attribution and verification evidence are mandatory for release.
Pending/restricted/unknown/rejected are never release-ready. Internal assets still
record their owner, source and preparation in the cost/readiness records.

Validation for every category: schema/ID/name, dimensions, references, materials,
textures, LOD or justified exemption, collision, streaming, provenance, Android and
S2-S7 regression evidence. A metadata pass does not prove that a mesh meets its declared
dimensions, legal rights or visual quality; reviewers attach evidence before approval.

## Category matrix

| Category | Folder | ID example | Name example | Source -> Unreal | Pivot/reference | Collision | Category/mobile checks |
|---|---|---|---|---|---|---|---|
'''
for category,folder,prefix,name,source,target,pivot,collision,checks in categories:
    intro+=f'| {category} | {folder} | {prefix}.Family.01 | {name}_Family_01 | {source} -> {target} | {pivot} | {collision} | {checks} |\n'
intro+='''
Formation JSON describes ordered presentation reuse, never force/mass/route authority.
Station platform extents in the catalog mirror the authoritative S7 stop contract;
the catalog cannot change where a stop counts. Audio, animation and VFX consume copied
semantic state. Signal presentation must retain text/symbol meaning at every tier.
'''
write('CONTENT_STANDARDS.md',intro)
write('PROVENANCE_STANDARD.md','''# Provenance and licensing record

Machine-readable schema: `Content/IndianRailway/Production/Schemas/provenance.schema.json`.
Pending template: `Templates/provenance.json`. No acquisition is represented by it.
`SourceReference` accepts an original URL or repository/archive reference. AssetId,
AssetName, Category, Creator, Source, AcquisitionDate, License, CommercialUse,
ModificationPermission, RedistributionRestrictions, AttributionRequirement,
OriginalArchive, ModifiedAssetReference, VerificationStatus, VerificationEvidence and
Notes are required. An acquisition date may be null only before acquisition/review.

Workflow: unknown -> pending review -> approved / restricted / rejected. Restricted
rights require a later explicit review, never automatic release. Approval needs actual
license/source evidence, verified commercial and modification rights, acquisition date,
attribution/restriction text and an accountable owner. Store license/archive hashes or
immutable references alongside the evidence; do not redistribute an archive merely
because it is referenced. The validator checks declarations, not legal authenticity.

`validate_s8.py --release` rejects unresolved rights, unaccepted assets and the empty
foundation catalog. Foundation validation permits pending drafts, and that permission
does not authorize using those assets in a release. License grants never come from a
filename, a marketplace thumbnail or an AI-generated claim.
''')
write('IMPORT_STANDARD.md','''# Repeatable import and binding workflow

1. Register stable logical ID, category, owner and provenance before import. Retain
   original source/archive reference and evidence; verify rights.
2. Validate source integrity and format, clean duplicate/hidden objects, inspect mesh,
   normals, UVs, skeleton or audio channels as appropriate. Record tools and manual work.
3. Check real dimensions, centimetre conversion, +X/+Z, pivot, attachment markers and
   definition alignment. Store screenshots/export settings and correction work.
4. Name source/object files using CONTENT_STANDARDS; import into the category folder.
5. Assign shared material instances, texture groups/mips/ASTC; assign LODs, collision
   proxy, skeleton/animation and semantic audio profile as applicable.
6. Register the Unreal soft path under the existing logical ID in manifest.json;
   declare dependencies, chunk, essential/optional, platforms and tier variants.
7. Bind presentation ID to the existing DefinitionId/version or station identity.
   Run `python 07_Documentation/S8/Tools/validate_s8.py` and negative foundation tests.
8. Run Unreal Automation, package with Build-S8.ps1, install with hash verification,
   test Android readability/quality/streaming/recovery and capture the declared profile.
9. Complete cost/readiness evidence, review exceptions, and approve explicit measured
   budgets. Then validate release metadata and repeat for content #2.

No DCC importer is implemented here because there are no production source assets.
No final mesh/material/LOD is fabricated by these tools. A second import may edit
catalog/content/profile data and adapters, but must not rewrite physics, authority,
AI, scoring, Level Blueprint logic or create a widget special case.

## Binding contract

Vehicle `definitionId`/`definitionVersion` link to URailVehicleDefinition.Record().
PresentationId is separate and resolves exterior, cab, bogie, wheel, lights, animation,
audio and materialLod. Null means intentionally unbound in the foundation; existing
S7 proxy presentation remains. FormationBindings reuse ordered presentation IDs and
do not override the authoritative formation members, dimensions or handling.

Station binding includes stationId, serviceId, platforms, visual set, signage, canopy,
footbridge, furniture, lighting, people/activity, ambient audio, chunk, regional kit and
landmarks. The S7 platform edge/extents/dwell remain authoritative. Route bindings map
existing RouteId to presentation chunks/stations and cannot grant movement authority.

AudioProfiles declare traction, rolling, brakes, horn, doors, compressor/fans, cab
controls, station ambience, announcements, environment and rain. Each layer references
an audio AssetId or null and a declared semantic observation. Traction/rolling/brakes
read handling, doors/announcements read station phase, horn/control cues read accepted
commands, ambient/rain read presentation state. There is no random audio-driven train
behaviour. Existing S7 synthetic sounds and horn adapter are untouched. Auxiliary and
weather fields are future presentation inputs, not newly implemented systems.

## Streaming and bundles

The foundation mirrors S6 chunks 1/west/L_ChunkA and 2/east/L_ChunkB. The host validator
and Unreal loader reject disagreement. No new chunk is silently accepted by the adapter.
Bundles describe route/station/vehicle/audio/optional-scenery lifetime ownership; Android
download/chunk package partitioning is not enabled. Essential references follow current
S6 readiness gating. Optional detail can be absent. Release a presentation owner's
handles/components at unload; audit retired weak references, activation counts and PSS
after repeated cycles. Do not infer leaks or unloading success from PSS alone.

## Save compatibility

S8 catalog versions describe presentation, while snapshot schema 1 and IR-S6-fixture-1
remain the existing binary identity. Sidecar metadata checks contentId/version, explicit
compatible previous versions, schema, exact service policy, vehicle ID/version and route
contract. Compatibility does not bypass CRC, semantic world validation or S7's exact
serialized ServiceDefinition header. No slots are rewritten, no migration is guessed.
Unknown versions keep both checkpoints and produce an actionable incompatibility.
Replacement tests keep logical IDs and actual S7 capture/restore valid. Changing service
configuration, definitions or topology requires separate compatibility work and evidence.
''')
write('ANDROID_BUDGET_METHOD.md','''# Android measurement and provisional budgets

Profile contract: `Production/profile.json`. Device: iQOO Neo 9 Pro / I2304, Android
16/API36, ARM64, Vulkan, ASTC. S8 foundation profiles the current S7 fixture only.
The future representative workload is `Production/workload.json`, explicitly unexecuted.

Use Build-S8.ps1 after host validation and Automation. Record exact APK SHA256, package
bytes, engine/source hashes, device properties, build ID, graphics tier, workload,
thermal/charging/hotspot conditions and repetition index. Install without clearing saves.
Do not change phone connectivity; the inherited APK excludes INTERNET permission.
Confirm this in the final APK/device dump and verify offline operation at app level.

Capture 6000 engine frames at the existing 30 FPS cap, analyze seconds 10-170, and pause
the simulation before CSV export using the existing S7 pause workflow. Retain failed
captures separately. Use at least three paired repetitions for content comparisons,
with matching service/camera sequence and thermal conditions. Numbers define a repeatable
measurement procedure, not final asset budgets. Record menus/pauses and camera changes.

`measure_s8.py capture.csv --apk build.apk --device samples.json --output result.json`
requires finalized CSV and full analysis-window coverage. Report mean FPS, frame p50,
p95, p99, maximum; GameThread, RenderThread, RHI, GPU, simulation step and service update
separately. Lanes overlap/include waits; simulation is included in service update. Never
sum them. Missing/partial/nonfinite lanes and zero-only GPU are UNVALIDATED, not free work.
Synthetic harness tests are clearly separate from real device output.

The device sample adapter stores PSS in KiB, request-to-ready latency in ms, resident
chunk/asset counts, activation counts and Android thermal status. Preserve raw samples
with timestamps and collection source; the summary hashes its input. PSS includes driver
and allocator caching. Existing S6 telemetry reports chunk load and residency; future
asset activation counters must come from actual owners, never be invented from chunk
counts. Unsupported counters remain absent/unvalidated.

Budget approval: PROVISIONAL (null limits) -> MEASURE (raw before/after evidence) -> ACCEPT
(quality and device review) -> FREEZE (versioned approved limits). Review per-category
LOD transitions, slots/draw calls, textures/PSS, overdraw/GPU, audio residency and load
latency. No final budget is asserted by this foundation, and no representative content
performance is inferred from proxy performance.

Foundation device checklist: package/install, launch S7, load five JSON configurations,
runtime S8 report passes, current save resumes, tier records available, compatibility
negative checks pass, no INTERNET permission, collect a real finalized profile. Pending
device work must stay pending until evidence is collected.
''')
write('CONTENT_COST_TEMPLATE.md','''# Production cost and repeatability

Copy Templates/cost.json and Templates/readiness.json for each future logical AssetId.
All arrays start empty; effort is null until recorded, never an estimate disguised as a
measurement. Record source/tool chain, preparation, manual steps, corrections, integration,
Android optimization, iterations, failures/rework, AI assistance and verification.
Each step holds description, actual effort minutes and evidence references. Effort can
overlap wall-clock time; do not sum simultaneous AI/manual steps into elapsed duration.
An accepted state requires review evidence; this is not proof of an existing production asset.

Instantiate Templates/repeatability.json with distinct content #1 and #2 IDs. Attach both
integration records and a reviewed source-change manifest. Same-pipeline, no physics
rewrite, no authority rewrite, no duplicate AI/scoring, no special Level Blueprint and no
special widgets all require evidence before passing. Automated structure checks cannot
replace a source review. A template pass means it can be instantiated, not that the
future second-content integration passed.

Readiness checks cover identity/source/license/commercial/modification rights, scale,
orientation/pivot, materials/textures/LODs/proxy, animation/audio, streaming/platform/tier,
Android/regression and final approval. Every passed check needs evidence; every exemption
needs a reason. Nothing in the pending templates is marked passed.
''')
write('LOCALIZATION_ACCESSIBILITY.md','''# Localization and accessibility foundation

Production/localization.json is a UTF-8 catalog with stable keys, en/hi samples and English
fallback intent. Use keys in future widgets and convert to localized FText/String Tables
at integration; do not localize simulation IDs or compare translated text for objective
truth. Only four sample strings exist. Current S7 widgets are intentionally preserved;
this task does not claim they are already fully localized.

Future presentation configuration exposes font scale 1/1.25/1.5, standard/large touch
layout, control labels, pause, result and warning keys. Signal meaning must include text
and symbol alongside color. Validate DPI/safe areas and actual Devanagari shaping/glyph
coverage on Android before accepting any font. These sizes are configurable samples,
not a validated accessibility or human comprehension result. S7 human gates stay pending.

Graphics Low/Medium/High records contain only presentation cost fields. Null means use
the existing measured fixture configuration until a tier override has evidence. Unknown
fields (including physics/AI/scoring settings) fail host validation. No runtime CVar or
logical-state changes are made when these records load.
''')
print('Standards and workflows written.')
