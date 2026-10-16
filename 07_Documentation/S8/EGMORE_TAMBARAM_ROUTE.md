# S8 Chennai Egmore to Tambaram route extension

Current status (13 October 2026): content 5 / service 6 has completed a rendered desktop journey with 33 captures, streaming reload, wheel checks and save preservation. Speed/braking audio is integrated and native mixer output is verified. Final realistic-art acceptance and real platform assignment remain open. Android is excluded from the current task. See `Evidence/EgmoreTambaram/AUDIO-DRIVING-CONTINUATION-20261013.md` for current implementation and measured evidence.

The sections below retain the historical version-2 implementation/research record; their placeholder geometry, speed and acceptance status are superseded by that continuation note.

## Implemented scope

- New simulation content identity `IR-S8-MSTBM-2`, version 2.
- New service identity `S8-MS-TBM-01` / `IR-S8-MSTBM-service-2`.
- Version 1 scaffold snapshots are rejected rather than reinterpreted after
  removal of fictitious one-position points. Existing Egmore identities remain unchanged.
- Existing `IR-S8-EGMORE-1` and `S8-Egmore-01` are unchanged.
- New route station labels `Station.ChennaiEgmore.01` and `Station.Tambaram.01`.
  The origin label is not a replacement for the existing art identity
  `Station.Egmore.Heritage.01` or its scene/interior/signage families. Binding the
  existing Egmore presentation to the extended route remains unfinished.
- Fifteen-location, 25 km kilometre chain and four parallel graph tracks.
- Fifty-six stable track edges, 60 stable nodes and reciprocal transitions.
- Separate protected player and AI corridors, route-derived signals,
  whole-formation occupancy, path history and snapshot recovery.
- Ordinary continuations no longer create point machines. Branching infrastructure
  requires mutually exclusive governing point positions; the route still lacks
  a researched meaningful junction and does not satisfy that acceptance gate.
- WAP-7 plus seven LHB coaches retained at native logical dimensions.
- A terminal-stop contract: a correctly berthed, stationary train completes at
  Tambaram after the full dwell. It is not teleported and does not depart beyond
  the destination to trigger completion.
- World coordinates are centred around the inherited S6 boundary (`x=0`), near
  the Guindy–St. Thomas Mount portion, while station distance remains measured
  from Chennai Egmore. Presentation removal cannot remove logical graph state.
- Runtime opt-in is `-S8Tambaram` on the existing rail-service application map.
  It uses an isolated save directory under `Saved/S8/EgmoreTambaram`.

## Reference-backed sequence

The Southern Railway weekday down timetable publishes Chennai Beach as km 0,
Chennai Egmore km 5 and Tambaram km 30. Rebased to the playable origin, the
implemented sequence is:

| Route m | Published km | Reference location |
|---:|---:|---|
| 0 | 5 | Chennai Egmore |
| 2,000 | 7 | Chennai Chetpet |
| 4,000 | 9 | Nungambakkam |
| 5,000 | 10 | Kodambakkam |
| 7,000 | 12 | Mambalam |
| 8,000 | 13 | Saidapet |
| 11,000 | 16 | Guindy |
| 13,000 | 18 | St. Thomas Mount |
| 14,000 | 19 | Palavanthangal |
| 16,000 | 21 | Minambakkam (commonly Meenambakkam) |
| 17,000 | 22 | Tirusulam |
| 19,000 | 24 | Pallavaram |
| 21,000 | 26 | Chrompet |
| 24,000 | 29 | Tambaram Sanatorium |
| 25,000 | 30 | Tambaram |

Primary route-sequence source: Southern Railway, *Chennai Beach – Tambaram –
Chengalpattu Down Week Days*, pages 6–12:
https://sr.indianrailways.gov.in/cris/uploads/files/1563341095383-2-15%20MSB-TBM-CGL%20DOWN%20WEEK%20DAYS.pdf

Station identity/category cross-check: Southern Railway, *List of Stations as on
01.04.2022*, which identifies Chennai Egmore (MS) and Tambaram (TBM) in Chennai
division:
https://sr.indianrailways.gov.in/cris/uploads/files/1659695525713-SR.pdf

## Explicit geographical approximations

The sources above establish order and kilometre chain, but not survey centreline,
curvature, gradients, exact track centres, points, platform allocation, signal
chainage or OHE mast positions. The current lateral centreline is therefore a
placeholder polyline (0–550 m lateral change over 25 km), with
four tracks at 5 m centres. Its straight edges have heading discontinuities;
this is not accepted curve geometry. It must not be described as geodetically or
engineering exact. Gradients are zero until an authoritative vertical profile is
available. The current operational pair is representative and does not claim a
verified real WAP-7 platforming assignment.

### Alignment research update, 12 October 2026

Southern Railway's *Maximum Permissible Speed in Sections*, row 17, records
Egmore 4.320 to Tambaram 29.140, length 24.820 km, distinguishing suburban and
A/B lines. This is historical reference evidence, not a current speed authority:
https://sr.indianrailways.gov.in/cris/uploads/files/1632378106476-Sectional%20Speed.pdf

The 25 km table above is rounded timetable data and remains only the scaffold's
chain; it must not be treated as precise track length. Current official sequence
confirmation and main-line platform allocation remain open before final authoring.

Retained OSM API extracts and SHA-256 records are in
`06_Routes/ChennaiEgmoreTambaram/References/OSM`. Run
`python 07_Documentation/S8/Tools/analyse_tambaram_osm.py` for an offline audit.
The audit enumerates 16 connected candidate paths (24.776–24.872 km), retaining
every node and segment's source way ID. It does not bridge missing topology,
select a train service path, establish direction, or assign platforms. Metro/MRTS
station POIs are retained with their own tags, not treated as corridor stops.
OSM data attribution: © OpenStreetMap contributors, ODbL 1.0;
https://www.openstreetmap.org/copyright . No OSM geometry has yet replaced the
runtime placeholder. Redistribution of derived databases must preserve the licence.

## Verification

`IndianRailway.S8.EgmoreTambaramRouteContract` verifies graph validation, stable
station identity, 25 km scale, parallel edge inventory, service validation,
operations initialization, authority-derived signal state, the eight-vehicle
formation, multi-edge path history, cold world restoration, and old-Egmore save
isolation. Evidence is in `Evidence/EgmoreTambaram/UnrealAutomation`.

The standalone `Tools/Test-TambaramJourney.cmd` additionally exercises the full
logical journey with cold-recovery checkpoints at edge/station phase changes and
the spanning boundary. Latest `journey-results.json`: 20 checks passed, 189,627
ticks at 30 Hz (105.35 simulated minutes), 300 stationary dwell ticks, berth error
−0.420 m, no departure authorization, idempotent settlement and result recovery.
The existing serialized `departureTick` field holds terminal completion time;
it does not indicate actual departure. This is not a rendered desktop/device run.

12 October topology revision: editor build succeeded; all 20 Unreal suites passed,
zero failed/not run, 7.06 seconds of test execution. See
`Evidence/EgmoreTambaram/TopologyRegression-20261012/index.json` and
`Evidence/EgmoreTambaram/CONTINUATION-20261012.md` for hashes and continuation gates.

## Open acceptance gates

This change is not a completed representative-content route. The following remain
open and must not be inferred from the passing logical test:

- surveyed/OSM-derived horizontal alignment and evidence-backed gradients;
- a verified main-line platform/track choice for a WAP-7 + LHB service;
- meaningful crossover geometry and conflicting-route exercise;
- authored corridor presentation, Tambaram station meshes, OHE, signals, roads,
  bridges, surroundings, signage, LODs and chunk ownership;
- visual review from every required camera and screenshot evidence;
- a full uninterrupted end-to-end desktop service run;
- Android build/install, offline run, device completion, performance, memory,
  thermal, streaming-latency and package-size measurements.

The inherited handling model currently limits this full-scale route to 5 m/s;
with 4 m/s assisted cruise, the measured logical journey takes about 105 minutes.
Raising the limit requires
measured braking/authority validation rather than a configuration-only change.

