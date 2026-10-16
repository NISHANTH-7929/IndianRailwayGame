# Source art review — 7 October 2026

Accepted by the producing assistant for integration review; user visual sign-off and
runtime acceptance remain pending. This is a bounded representative heritage scene,
not a surveyed replica of the complete Egmore station or its 2026 redevelopment.

The final street view was compared with the registered facade panorama. It retains
the two-storey red/cream arcade, cusped openings, projecting cornices, entrance
pediment and dome/tower vocabulary. Bay sequence, tower spacing, returns and roof
depth are estimates. Advertising and crowds in the reference are omitted.

Player-distance platform views show continuous tiled paving, clear yellow/coping
edges, roof corrugation and trusses, separate rail heads/webs/feet, sleepers and clips,
ballast shoulders and stones, multilingual nameboards, benches, bins, drinking water,
fans, ducts, portal steelwork and a connected footbridge. Bridge entries have openings
in the western guardrail. The portal feet sit beyond the platform corridor. Two dark
signal housings are presentation-only and are not bound to operating aspects.

The vegetation and adjoining buildings are intentionally simplified background
geometry. The source tree silhouette is less uniform than the rejected first pass,
but foliage is visibly low-poly at close range. Keep the trees behind the boundary.
512px tiled materials meet the declared mobile texture policy; the inspector still
notes their close-up resolution limit. Their mapping is metric and the exported GLB
render was inspected for correspondence with the source. Geometry LODs are exported
and triangle-checked; switching distances require an engine camera review.

`PlatformEye.png` is day/dry. `DayWet.png`, `NightDry.png`, and `NightWet.png` demonstrate
source material/light response. They do not include falling rain, dynamic weather,
audio, engine lighting, or performance evidence. The source look-development script
records its estimated lights and material changes separately from the source asset.

No train model is included. A conservative 3.25m wide stationary vehicle envelope,
from 0.90 to 4.80m above rail, clears scenery in the four authored tracks. This does
not replace actual WAP-7/LHB placement, moving-envelope or curved-track tests.

## Evidence

- `Evidence/binary-audit.json`: 253 passed checks, zero degenerate triangles, complete
  UVs/normals/tangents, actual inside-head gauge 1.676m within numeric tolerance.
- `Evidence/roundtrip.json`: 70 passed module/format comparisons, bounds error <1mm.
- `Evidence/clearance.json`: no intersections with the stated conservative envelope.
- `Evidence/game-dev-validation.json`: policy passed, 25 scene materials, 40 bound
  texture resources, 1,095,960 expanded LOD0 triangles.
- `Evidence/package-verify-02.json`: packaged GLB bytes and hashes verified.

Earlier failed package requests are retained in Evidence for transparency: one PNG
preview was rejected because that optional CLI field accepts only USDZ. The successful
package omits that field; all nine PNG previews remain in this source handoff.
