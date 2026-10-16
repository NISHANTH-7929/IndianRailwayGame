# StationArt v2.0.0 integration handoff

## Additive interior supplement available: v2.1.0

`04_Assets/S8/Workstreams/StationArt/v2.1.0-interior/` is sealed. Overlay
`InteriorDelta.glb` at the v2 origin or use its own `manifest.json` to instance five
new modules. **Do not duplicate the base scene:** `EgmoreInterior.blend` includes base
v2 for preview context; only the delta GLB/new modules are additive imports.

57 placements; 24,092 expanded triangles; five scalar materials; no delta textures.
15 module GLBs and 15 FBXs carry three readability-preserving LOD levels. Platform
numbers are scene labels requiring scenario binding, not surveyed Egmore numbering.
`lighting-bindings.json` and `material-bindings.json` describe fixtures and source
look-development settings. Calibrate runtime intensity/exposure and use a mobile
lighting strategy. No runtime light operations were made here.

91 source checks and the independent GLB policy pass; v2 hashes are unchanged.
Four reviewed views focus exclusively on the platform, waiting area and bridge.
Full provenance/instructions: supplement README.md. Hash inventory: SHA256SUMS.json.
Delta SHA-256: `d6b0080c4e2199b96fab43732b091b1944d923641e269bda787f81b646500d92`.

The base v2 handoff below remains valid. The supplement can be reviewed after the
ongoing base import without replacing or regenerating its accepted source bytes.

User priority update (7 October): station interior/platform and train-camera quality
take precedence. Exterior facade, road and surrounding context need no further polish
for now. Review the platform-eye and cab/train views first during integration.

**Immutable source handoff, ready for integration review.** v1 is withdrawn.
The historical handoff is retained in HANDOFF-v1-withdrawn.md.

## Files

Source: `04_Assets/S8/Workstreams/StationArt/v2.0.0/`

| Purpose | File |
|---|---|
| Editable source | EgmoreScene.blend |
| Assembled portable scene | EgmoreScene.glb |
| Module exports | Modules/*_LOD0.glb through *_LOD2.glb, with matching FBXs |
| Placement/bounds | manifest.json |
| Identity mapping | stable-id-map.json |
| Provenance | provenance.json |
| Complete hash roster | SHA256SUMS.json |
| Visual review and limits | SOURCE_REVIEW.md |

GLB SHA-256: `c09777218b90a8d796f4a7a92f90e76905e41f1684d38378ab3712d014766cda`.
Manifest SHA-256: `34c264ad95f34fa873ed35cee72b758345a59b3794e60576d4d27447b5eeb8ca`.

Read-only verification:

```powershell
python 04_Assets/S8/Workstreams/StationArt/tools/verify_v2.py
```

Verified CLI package:
`04_Assets/S8/Workstreams/StationArt/cli-workspace/.game-dev/packages/chennai_egmore_station_art/2.0.0`
Package ID: `pkg_99dad883d1882e8e3ec4ab94`.
The CLI package contains the assembled GLB; the version directory additionally
contains source, all module LODs, renders and evidence.

## Coordinates and placement

Blender: metres, X longitudinal, Y lateral, Z up. GLB: standard Y-up.
FBX: -Y forward/Z up, without the old kit's bespoke Y reflection. Import as a separate
family and validate Unreal bounds; do not blindly reuse old force-front-X compensation.
All 70 module/format reimports matched source bounds within 1mm in Blender.

| Anchor | Metres |
|---|---:|
| Track centres Y | 0, 5, 16.35, 21.35 |
| Inside rail-head gauge | 1.676 |
| Rail-top Z | 0 |
| Platform top Z | 0.84 |
| Platform Y intervals | -9.675…-1.675; 6.675…14.675; 23.025…31.025 |
| Platform X extent | -108…108 |
| Contact wire Z | 5.50 |
| Bridge deck top Z | 7.56 |
| Bridge west edge / stair top X | 64.20 |
| Stair start X | 52.02 |
| Stair rise / tread | 42 × 0.16 / 0.29 |

Suggested initial WAP-7/LHB placement: track Y=0, rail Z=0, aligned with X.
No train is bundled and no route/service identity is changed.
New family: Station.Egmore.Scene.02. Existing source IDs are mappings, never replacements.

## Evidence and integration requirements

253 binary checks, 70 reimport comparisons, stationary-envelope clearance and independent
policy validation pass. Package hashes also pass. See Evidence/ and SOURCE_REVIEW.md.

Use shared module instances, LODs and culling. Full-site triangle counts
1,095,960 / 557,437 / 213,101 are static source totals, not measured phone budgets.
Keep simplified foliage behind the boundary. 512px tiled textures meet the source
policy but require engine streaming and close-camera review.

SignalUnbound has dark lenses. Bind authoritative runtime aspects; these meshes do
not invent operating states. Actual train placement, motion, curves and bridge/canopy
clearance must be verified in integration. Night/wet images are source light/material
studies; runtime rain, audio and Android are not tested here.

The heritage composition and four-track scene are representative estimates, not a
survey of the full yard or the 2026 redevelopment. User visual sign-off remains pending.

## Reproduction

Executable staging tools: StationArt/tools/. Snapshots: v2.0.0/Reproduce/.
Restore snapshots into tools and redirect output to a new version before running.
Never regenerate into this sealed directory. Canonical source meshes are read-only
inputs, with hashes in the manifest. Reference photos do not become texture pixels.
No paid generation occurred. Earlier builds and preview passes remain in documentation
staging. No Unreal/UAT/ADB/phone operation or full S8 completion is claimed.
