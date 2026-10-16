# Next interior implementation gate — 10 October 2026

Prioritize the playable station interior, not facade or exterior expansion.
Astra read-only audit identifies bounded runtime night lighting as the next
substantive gap after cab/audio acceptance. This is a plan, not completed work.

## Existing sources

- RailEgmoreSceneView.cpp constructs imported module/placement HISM instances,
  but does not bind night/environment lighting.
- v2.1.0-interior/lighting-bindings.json supplies fixture positions, directions,
  colors and source sizes. Engine intensity remains uncalibrated: source watts
  must not be copied as lumens.
- create_egmore_service_map.py derives the playable map from the daylight study.
- Content/IndianRailway/Production/workload.json has four environment combinations
  with no execution evidence. Older generic-station measurements do not accept
  the combined playable Egmore workload.

## Bounded implementation and acceptance

1. Preserve station geometry, cab controls, save formats and exterior scenery.
2. Add a limited mobile interior night-lighting preset using existing fixture
   metadata. Calibrate intensity/exposure; bound dynamic-light cost.
3. Review platform wayfinding, canopy underside, stairs/bridge approach and cab
   view on playable Egmore, both day and night. Confirm labels remain legible.
4. Package immutable candidate and measure the combined Android workload with
   repeated comparable runs. Only then add and accept rain/wet response.

Separate remaining gates: small cab-label readability, actual audio listening,
audio interaction/lifecycle evidence, fresh uninterrupted service/human usability,
and final production catalog/provenance. Preserve the Foundation manifest; do not
rename its empty asset bindings into a production acceptance claim.

Completed historical gates must not re-enter the implementation queue: playable
Egmore, signal binding, isolated cold recovery, earned reward persistence and
resumed scripted Standard itinerary already have scoped evidence. Their existing
limitations remain documented. Do not reopen WAG-9/BLC or exterior polish here.
