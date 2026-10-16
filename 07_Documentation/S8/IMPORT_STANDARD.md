# Repeatable import and binding workflow

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
