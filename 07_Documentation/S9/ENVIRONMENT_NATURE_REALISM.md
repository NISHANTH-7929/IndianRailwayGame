# Corridor ground and vegetation — 29 October 2026

This pass improves the track surroundings on the existing WAP-7/LHB
Egmore–Tambaram journey. It retains the route, traffic rules and save identity.

## Changes

- Shallow median relief, a worn maintenance path and a drainage depression.
  The grass roots follow the relief; foundations under the tracks remain level.
  Station precincts and river approaches exclude this new terrain.
- Curved grass blades and low scrub replace flattened tree-leaf ground clumps.
  Dry vegetation texture varies the outer ground. Empty building plots and
  distant open land receive bounded, irregular grass/shrub groups.
- Fuller tree crowns with differently inclined leaves, staggered branches,
  lower inner foliage, and continuous palm ribs. Near trees form groves and
  gaps, with varied setbacks, rather than repeating a fixed row.
- More variation in roof tanks, small pitched roofs, panel banks and facade
  details. Existing city geometry remains representative procedural artwork.
- Wider outer terrain overlaps close potential gaps on the tightest bends;
  the riverbank footprint is preserved.

All placements are deterministic, graph sampled, instanced and collision free.
There is no per-frame terrain generation or new gameplay dependency. Grass
culls at 300 m, casts no shadows and has three authored LODs. The new terrain
and vegetation use a fixed shared material palette and bounded streaming cells.

## Assets and reproducibility

Runtime folder: `/Game/IndianRailway/S9/CorridorNatureV1`.

| Mesh | LOD triangle counts |
| --- | --- |
| SM_CorridorVerge12m | 1968 / 552 / 216 |
| SM_CorridorLeafClusterV2 | 664 / 240 / 96 |
| SM_CorridorGrassPatchV1 | 140 / 55 / 18 |
| SM_CorridorBranchV1 | 48 / 36 / 24 |

Sources, Blender projects and hash receipts are under
`04_Assets/S9/CorridorNatureV1`. The tools are
`build_corridor_ground_v1.py`, `build_corridor_vegetation_v1.py`,
`build_corridor_vegetation_v2.py`, and their import companions under
`07_Documentation/S9/Tools`. `import_corridor_nature_ground.py` creates the
terrain material by reusing the previous soil/dry-verge textures and blending
the authored vertex masks. No new generated images or external assets were used.

Import checks verify source hashes, mesh bounds, all authored LODs, material
sections and absence of collision. The original V1 canopy remains available;
V2 is the runtime selection. The first visual review found sparse, edge-on
foliage; V2 corrects the leaf-plane angles and density before final review.

## Validation

Validation is being recorded under `07_Documentation/S9/Evidence`.
The import/build evidence is `EnvironmentRealism-20261028-01` and the
77-check simulation run is `EnvironmentJourney-20261028-01`.
Final scene, motion and package results are recorded below after completion.

The native scene audit examines populated components and their assigned
materials, authored LOD availability and disabled collisions. It also retains
the rail material/texture/UV checks. Scene review uses isolated checkpoint
copies and verifies unchanged logical state and source checkpoint hashes.

`Review-CorridorMotion.ps1` performs a bounded real-time drive, using normal
simulation resume and actual cab/forward cameras. Its per-frame CSV and result
separate functional traversal from frame timing. Screenshot and loading cost
are included. This is not a full manually driven service or a mobile test.

These assets remain representative artwork, not a surveyed terrain or botanical
scan. The work improves the existing procedural scene; it does not establish
photorealism for the whole corridor. Mobile qualification requires another
device run.
