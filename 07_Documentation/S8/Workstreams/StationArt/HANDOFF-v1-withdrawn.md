# StationArt handoff — Station.Egmore.Scene.01 v1.0.0

**WITHDRAWN FOR PRODUCTION INTEGRATION (2026-10-07).** v1 is preserved as immutable
rejected evidence. The ready label below is superseded by this notice. File-existence
validation did not establish correct gauge, platform clearance, arch geometry,
bridge accessibility, exported materials or adequate LODs. v2 is in progress.

Status: **READY-SOURCE-PREVIEW / IMMUTABLE**  
Date: 2026-10-07  
Owner boundary: StationArt staging only

## Integrator entry points

Use `04_Assets/S8/Workstreams/StationArt/v1.0.0/placement_manifest.json` as the authoritative machine-readable placement and stable-ID map. Verify `SHA256SUMS.json` before copying. The preferred interchange file is the GLB for rapid visual/bounds inspection; FBX is supplied for the canonical import workflow. The `.blend` remains the editable source for a new version only.

Key anchors (metres, +X longitudinal, +Y lateral, +Z up):

| Contract | Value |
|---|---:|
| Rail-top Z | 0.00 |
| Track centres Y | -7.50, -3.00, 3.00, 7.50 |
| Platform centres Y | -5.25, 5.25, 11.00 |
| Platform top Z | 0.84 |
| Contact wire Z | 5.48 |
| Suggested WAP-7/LHB track | Y = -3.00, longitudinal origin X = 0 |
| Declared site bounds | X ±130, Y ±36, Z -1.15…21 |

## Primary hashes

| File | SHA-256 |
|---|---|
| `ChennaiEgmore_StationArt_v1.0.0.blend` | `4519254A9DAD4D5C337B3EB0A9B2E977A375265E28A9410B9B21D4922B0ACCB6` |
| `exports/ChennaiEgmore_StationArt_v1.0.0.fbx` | `7634EC048BFF7033227519E2E29E8717200CE8DFB10DCDA5C36E6BDEF2BFEAAC` |
| `exports/ChennaiEgmore_StationArt_v1.0.0.glb` | `669489876705FEF3223189F219269F29713412BD2041457E33993BF85D9FE3A8` |
| `placement_manifest.json` | `40A9165FFB608C62E74BC79A58E1BBA6A011CF7141D2BF5E15BC47BA2FC7736B` |

Run the staging validator before import:

```powershell
python 04_Assets/S8/Workstreams/StationArt/tools/validate_package.py
```

## Required integration checks

1. Copy into canonical content under a new integration receipt; do not modify this ready directory.
2. Confirm imported FBX/GLB axis, metre scale, declared bounds and readable stable IDs.
3. Replace proxy LOD policy with engine-specific LOD grouping/instancing as appropriate; preserve IDs in the receipt.
4. Place WAP-7/LHB on the chosen track and check platform/canopy/footbridge clearance from player cameras.
5. Review material packing because the GLB exporter reported multiple image nodes per Blender material; the standalone BC/N/ORM maps are authoritative.
6. Perform Unreal camera, lighting, collision, performance and Android validation in the original integration chat.

## Acceptance boundary and remaining risk

The package passed static presence, stable-ID uniqueness, gauge contract, texture count, preview count and hash generation. It has not been imported into Unreal and has no Android evidence. Fine facade ornament, exact historical/present-day layout fidelity, signage, close-range canopy detail and final production LODs remain visual-review risks. Do not present this handoff as final S8 completion.
