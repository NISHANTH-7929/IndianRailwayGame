# StationArt status — 2026-10-07

## Correction after visual and geometry review

**v1.0.0 is rejected for production integration. Work is continuing on v2.0.0.**
The previous readiness statement below is historical and superseded. Its checks only
verified file presence and metadata, not geometry or visual quality. Identified defects
include incorrect horizontal torus arches, platform edges too close to track centres,
rail inside-face gauge narrower than its declared value, disconnected footbridge
stairs, unverified generated-coordinate material export, guessed rather than measured
bounds, and identical incomplete LOD proxies. Preserve v1 as evidence; do not import it
as finished station art. The user explicitly requested continued work on 2026-10-07.

## Current outcome

`Station.Egmore.Scene.01` version `1.0.0` is **ready for integration review** and is now immutable. It is a coherent source-preview scene, not a claim of S8, Unreal, gameplay, performance or Android completion.

The package contains a bounded 260 × 72 m Chennai Egmore-inspired scene with four broad-gauge tracks, two island platforms plus the heritage-side platform, full canopy runs, covered footbridge and stairs, OHE presentation meshes, platform edges, sleepers/rails/ballast, cabinets, boundary treatment, trees and a deliberately simplified urban backdrop. The 126 m heritage composition uses the visible red-and-cream vocabulary, repeated arches, central entrance mass, paired towers, domes and cornices seen in the registered reference set.

## Evidence-backed package state

- Blender source: `04_Assets/S8/Workstreams/StationArt/v1.0.0/ChennaiEgmore_StationArt_v1.0.0.blend`
- FBX/GLB: `v1.0.0/exports/`
- Placement/stable-ID manifest: `v1.0.0/placement_manifest.json`
- Textures: ten original 512 px BC/N/ORM sets (30 PNG maps)
- Preview cameras: platform eye, track arrival and heritage wide
- LOD0: 921 placed mesh objects / 22,712 source triangles
- LOD1: five proxy objects / 60 source triangles
- LOD2: five proxy objects / 60 source triangles
- Static package validation: PASS (`v1.0.0/validation.json`)
- Hash inventory: `v1.0.0/SHA256SUMS.json`

## Dimensional confidence

The 1.676 m gauge and 0.84 m platform-top contract are inherited from the existing S8 railway standard. Track centres, canopy/bridge/architecture proportions, facade extent and context bounds are visual estimates made from registered photographs. They are not a dimensional survey and must not be labelled an exact 2026 layout.

## Honest visual gaps

- The source-preview silhouette and circulation layout are coherent, but fine heritage ornament, accurate bay-by-bay facade sequencing, multilingual signage and present-day redevelopment detail remain below final close-camera fidelity.
- Canopy corrugation is represented primarily by the authored material; gutter/support rhythm is geometric. A later revision can add silhouette corrugation where player cameras justify its cost.
- OHE is presentation geometry, not certified engineering geometry.
- LOD1/LOD2 are integration proxies, not final authored reduction meshes for every component.
- No WAP-7/LHB was added to staging; train placement stays with the integration chat.

## Scope compliance

All writes are inside the two assigned StationArt workstream folders. Existing reference assets, generators, Unreal content, runtime data, device state and prior evidence were not modified. WAG-9 and BLC were not touched. Blender 5.2.2 source rendering/export only was used; no Unreal Editor, UAT, build, ADB or phone operation ran.
