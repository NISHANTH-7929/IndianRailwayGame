# Interior supplement — additive to v2.0.0

This supplement adds five small modules: three numbered platform signs, a two-sided
footbridge/exit sign and a canopy light. It changes no base geometry or exterior.
Place `InteriorDelta.glb` at the v2 origin, or instance the new modules according to
manifest.json. Do not import EgmoreInterior.blend as a second complete base scene:
it includes v2 solely for editing and preview context.

The platform numbers 1/2/3 are representative labels for this three-platform scene,
not surveyed real Egmore numbering. Bind them to the final playable configuration.
Existing Tamil/English/Hindi station nameboards remain in the base. New lettering is
English mesh geometry, double-sided with readable glyphs in Blender and GLB.
Text LODs deliberately retain geometry to avoid damaged glyphs; screen-size culling
is preferable when unreadable. The fixture is already low-poly.

lighting-bindings.json provides explicit positions, direction, colour, source size and
source-preview wattage. Calibrate engine intensity; Blender watts are not Unreal
lumens. Use an appropriate baked/limited-light mobile implementation rather than one
dynamic shadowed light per fixture. material-bindings.json supplies scalar materials
including the lamp-lens emission. No new textures are required by the delta; copied
textures support the contextual Blender preview only.

Four views were inspected: PlatformWayfinding (day), PlatformWayfindingNight,
BridgeApproach and WaitingArea. Signs are readable in the walking view and light
fixtures correspond to the night-preview lights. Night sign highlights need engine
exposure review. Clearance checks place all new assets at least 2.5m above platform.

91 source checks pass, covering unchanged base hashes, module/LOD GLB and FBX bounds,
triangle counts, degeneracy, unique placement identities, preview count and head
clearance. These do not establish Unreal/Android acceptance. FBX lettering and basis
must be reviewed using the same importer conventions as v2.

All provenance, machine-readable placements, material/light bindings and hashes are
local to this supplement. No paid generation, exterior expansion or runtime edits.
