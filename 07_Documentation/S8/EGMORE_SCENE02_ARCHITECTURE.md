# Egmore Scene02 presentation boundary — 8 October 2026

## 9 October: service boundary implemented; map binding in validation

ScenarioContent now resolves IR-S8-EGMORE-1 consistently for operations creation,
service validation, snapshot admission and snapshot rebuild. Old FormationContent
IDs/definitions remain unchanged. Egmore uses two eight-vehicle WAP7/LHB trains on
separate protected straight tracks. No fictitious shared junction is introduced.
Pointless graphs may have zero point machines; graphs with transitions still require
point definitions. Route/authority/handling/AI algorithms otherwise remain unchanged.

MakeEgmoreService supplies S8-Egmore-01 / IR-S8-Egmore-service-1, platform492..708m,
berth700m, finish1000m. Its traffic objective observes authoritative adjacent AI
traffic; legacy services retain the required shared-junction hold. The new optional
traffic-policy field is serialized only for Egmore, preserving legacy service JSON.
The Egmore-only departure check requires current stopped, fully berthed state.

Thirteen Automation suites passed the initial service boundary, including approach,
dwell/departure/completion, settlement/cold recovery, deterministic continuation and
legacy/unknown-content rejection without save overwrite. Later map-binding changes
and the stricter departure guard are undergoing validation; do not inherit that pass.

Unreal application binding uses -S8Egmore and its own Saved/S8/Egmore/Service slots
(AssetReview is separate). New chunk-map paths preserve S6 assets; both player/AI
locomotives load WAP7 and coach placement uses the same logical poses. The station
scene can extend rails/ballast from authored-120..120m to logical-600..600m without
adding outside scenery. New map L_RailService_Egmore removes stationary review trains.
Real signal-mesh binding and broader phase-specific recovery testing remain open.
The frozen Android diagnostic remains the earlier static scene, not this service.

Historical architecture notes below describe the staged evolution; this section
supersedes the earlier reserved/unsupported scenario status.

The sealed station base v2.0.0 and additive interior v2.1.0 are imported as new
families. Old station, train definitions, service fixtures and checkpoint identities
remain intact. No RailKernel, RailHandling, RailOperations or AI code is changed.

`ARailEgmoreSceneView` validates the two fixed identity/version manifests and all
references/transforms before replacing presentation. It uses one hierarchical
instance component per module (40 total, 1,316 placements). Meshes have authored
three-level LODs. Collision is disabled; this actor has no simulation authority.
Rebuild and destroy/recreate tests check component/instance counts and transforms.

The isolated `L_EgmoreScene02_Study` contains a stationary WAP7 plus seven LHB coaches
using existing part transforms and native dimensions, with no fit-to-platform scale.
Front 100m, rear -89.330432m, platform -108..108m. Coupling-plane spacing follows the
existing LHB v1 contract: 24m per coach, zero additional gap. This is a stationary
presentation arrangement, not a new service or prototype WAP7 dimension approval.

The review actor activates its camera only in this exact study map. Six views are
selected with `-S8EgmoreView=0..5`, or cycled every 12 seconds with
`-S8EgmoreCameraTour`. Review telemetry goes only to
`Saved/S8/EgmoreScene02/Review/telemetry.json`. The map name does not initialize
RailSimulationSubsystem gameplay; no checkpoint read/write is introduced.

Canonical FBX copies perform a tested handedness conversion. Textured nameboard UVs
and interior mesh lettering require an additional readability correction. Source
GLB/FBX/hash inventories remain sealed. Both sign faces require actual render review.
Shared glTF PBR adapter has no implicit locomotive import side effects.

The interior fixture lens has bounded source emission; source Blender light watts
are not converted into Unreal lumens. A final mobile night-lighting implementation,
walkable collisions, route/platform/signal bindings, motion and service/save identity
remain separate unfinished work. Source sign numbers describe this representative
scene, not surveyed Chennai Egmore platform numbering.

Build-LHB -EgmoreScene02 creates a new timestamped archive, retaining existing maps
for regressions. Phone tests belong to the station chat; publish exact APK hash and
desktop evidence in PHONE_HANDOFF before device testing.

## Next playable boundary: spatial contract verified, not admitted

RailEgmoreLayout reserves IR-S8-EGMORE-1/version1 and defines the four authored
track centres, rail datum and a1200m approach/departure extent (-600..600m).
Player edge2 is Y0. Logical station distance is worldX+600; platform492..708,
start480, berth700, finish1000. The same WAP7+seven-LHB definitions derive a
189.330432m formation. No scaling or content-ID alias is used.

A new Automation suite moves that formation through220 one-metre steps on each
of four tracks, checks every body and bogie, berth/world coordinate mapping,
platform footprint and reversal. Twelve suites now pass (zero failures/warnings).
This later code is NOT in diagnostic APK20261008-143136 (that APK had11 suites).
FormationContent::Supports intentionally still rejects the reserved identity.

Do not enable it by merely adding a string to Supports: current operations content
initializes train1/2 at200/100, hardcodes legacy block/point routes, and service
validation bounds player edge2 to160m. Service success also requires actual traffic
priority/clearance. A parallel-track graph alone must not fake a shared junction.
Next integration needs an authored, validated protected-route layout, consistent
initialization and service limits, snapshots rebuilt against the same definition,
new save directory, signal bindings and a tested completion/settlement path. Old
22m/LHB/Shantipur bytes and identities must remain compatible throughout.

A real desktop player capture (desktop-player-waiting.png) shows more underside
structure than SceneCapture2D exports. Treat phone feedback as separate evidence;
lighting acceptance remains open. No engine-wide renderer switch was made.
