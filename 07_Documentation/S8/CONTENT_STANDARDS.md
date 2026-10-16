# S8 content standards

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
| locomotive | Vehicles/Locomotives | Loco.Family.01 | SK_Family_01 | blend + FBX -> SkeletalMesh | body centre on rail plane; bogie/wheel attachment markers | query-proxy | body, bogies and cab independently reducible; retain silhouette |
| coach | Vehicles/Coaches | Coach.Family.01 | SK_Family_01 | blend + FBX -> SkeletalMesh | body centre on rail plane; coupler and bogie references | query-proxy | shared body/material family; preserve door alignment |
| wagon | Vehicles/Wagons | Wagon.Family.01 | SM_Family_01 | blend + FBX -> StaticMesh | body centre on rail plane; coupler/bogie references | query-proxy | preserve loading outline; shared freight materials |
| formation | Data/Formations | Formation.Family.01 | DA_Family_01 | JSON -> DataAsset binding metadata | ordered stable vehicle presentation IDs; no spatial pivot | none | no meshes/textures; individual members own LODs |
| station | Stations | Station.Family.01 | DA_Family_01 | JSON + FBX -> DataAsset and modular StaticMesh set | survey/reference origin; platform edge markers | simple-proxy | modular instancing; station-area owner |
| trackside | Environment/Trackside | Trackside.Family.01 | SM_Family_01 | blend + FBX -> StaticMesh | base/track-offset reference; never signal truth | simple-proxy | instancing and distant silhouette; safety semantics remain logical |
| environment | Environment/Kits | Environment.Family.01 | SM_Family_01 | blend + FBX -> StaticMesh | tile origin and documented seam bounds | simple-proxy | instancing; optional distant pieces unloadable |
| building | Environment/Buildings | Building.Family.01 | SM_Family_01 | blend + FBX -> StaticMesh | ground-floor origin on kit grid | simple-proxy | shared facades; distant simplified silhouette |
| vegetation | Environment/Vegetation | Vegetation.Family.01 | SM_Family_01 | blend + FBX -> StaticMesh | trunk base at ground; upright +Z | none | alpha overdraw review, instances and reduced distant cards |
| character | Characters | Person.Family.01 | SK_Family_01 | blend + FBX -> SkeletalMesh | feet origin; skeleton/root and hand markers | query-proxy | shared skeleton; measured animated crowd cost |
| animation | Characters/Animations | Animation.Family.01 | AS_Family_01 | blend + FBX -> AnimationSequence | declared skeleton/root; no authoritative root movement | none | sample/compression profile and skeleton compatibility |
| audio | Audio | Audio.Family.01 | SW_Family_01 | lossless WAV master -> SoundWave/profile | spatial emitter anchor; no mesh pivot | none | semantic layers, concurrency and streaming residency |
| vfx | Environment/VFX | Vfx.Family.01 | NS_Family_01 | Niagara source + texture masters -> NiagaraSystem | emitter local origin + attachment marker | none | mobile shader support, particle/overdraw measurement |
| ui | UI | UI.Family.01 | T_Family_01 | SVG/PSD master + PNG -> Texture2D/Widget | documented anchors, DPI and safe area | none | UI texture group, Devanagari coverage, touch readability |

Formation JSON describes ordered presentation reuse, never force/mass/route authority.
Station platform extents in the catalog mirror the authoritative S7 stop contract;
the catalog cannot change where a stop counts. Audio, animation and VFX consume copied
semantic state. Signal presentation must retain text/symbol meaning at every tier.
