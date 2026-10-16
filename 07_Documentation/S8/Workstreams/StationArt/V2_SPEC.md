# StationArt v2 specification

The user authorized completion of the station-art staging deliverable, including local
Blender edits, preview renders, exports, validation and a reviewable package. No paid
provider is used. Blender is available at the explicit local 5.2 executable; the CLI
doctor does not discover it on PATH. Provider configuration is irrelevant to this local
workflow. All package CLI output must be directed to StationArt/cli-workspace.

## Art direction and reference limits

Representative Chennai Egmore heritage-era station, based on the existing registered
2009 panorama, 2011 east approach, 2013 platform and 2022 heritage photographs. Preserve
cream cusped arches, red fields, projecting cornices, curved entrance pediment, domes,
blue corrugated canopies and light steel infrastructure. The facade is a representative
composition, not an exact bay survey. Four tracks and three 216m platform segments form
a bounded playable-distance scene study, not the full Egmore yard. The service graph
and rolling stock belong to the original integration chat.

## Budgets and checks

- Target: at most 1.2 million expanded LOD0 triangles for the entire authored scene;
  reusable modules and two reduced LODs support integration-specific instancing/culling.
- Metric UVs and 512–2048 pixel original PBR textures; no photo pixels as surfaces.
- Up to 40 materials, opaque or simple original sign materials; no expensive foliage
  transparency requirement. Engine material consolidation remains an import concern.
- Three mesh LOD exports per module, unique placement IDs, actual world bounds,
  file hashes, source receipts and license/provenance record.
- Inspect actual GLB accessors, UV/normal/tangent attributes, triangles and gauge.
- Reimport GLB/FBX in Blender and compare bounds to source; render imported scene.
- Check platform setback and bridge stair endpoints, then inspect eye-level images.

## Exclusions and acceptance boundary

No WAG-9/BLC, new gameplay, Unreal import, UAT, APK, ADB or phone work. Those remain
outside this workstream. Source acceptance requires passed binary checks and inspected
renders. Runtime/device acceptance requires the integration chat's independent checks.
