# Indian Railway — Architecture Review and Technical Blueprint

Review date: 18 September 2026. Status: proposed architecture for review, not authorization to implement.

Reviewed inputs: `PROJECT_MASTER_BRIEF.md` v2, `01_GameDesign/Game_Vision.md`, and the user's detailed review instructions. The existing project's saved editor log reports UE 5.8.2; Android packaging, runtime performance, and project settings have not been validated. No implementation code or production assets accompany this review. The master brief is preserved unchanged.

The sections below follow the requested A–Z order. Numerical budgets and content counts are design hypotheses with validation gates, not measured results or externally imposed deadlines. Engine guidance was checked against Epic documentation; platform requirements must be rechecked against the selected engine patch and storefront at release.

## A. Executive Summary

Build an offline-first railway operations game whose central experience is driving a believable train through an operating Indian railway. The technical foundation should be a compact, authoritative railway simulation, with Unreal actors serving as its streamed visual and interactive representation.

Five decisions determine whether this vision scales:

1. **Represent railway state independently of actors and loaded levels.** A train, block, point, or station must continue to exist logically when its visual representation unloads.
2. **Use constrained longitudinal train dynamics on an explicit railway graph.** Do not begin with a chain of rigid bodies and wheel/rail collision physics. Preserve mass, gradients, traction, braking, and train length where they affect driving.
3. **Make movement permission a railway operations decision.** Signals display the result of occupancy, route locking, and operating rules; they do not independently decide whether a train can proceed.
4. **Build for a tested Android device floor.** Use conventional mobile rendering, authored LODs, bounded crowds, and predictive streaming. Desktop visual features are not the baseline.
5. **Define the MVP by coverage and quality, then content counts.** It needs multiple meaningful driving and operational experiences, a complete progression loop, and sustained Android stability. A single successful route is a vertical-slice gate, not sufficient player-release evidence.

The recommended candidate MVP has two meaningfully different locomotive classes, local and express passenger formations plus AI freight, two connected playable corridors sharing a junction, six functionally distinct stations, coherent signalling, timetable-driven traffic, limited station activity, day and night, dry and rainy conditions, and a short-session career. These quantities arise from an experience-coverage model in Section N and remain subject to prototype evidence.

Preserve extensibility through stable identifiers, explicit ownership, versioned data, shared player/AI commands, and verified safety invariants. Do not prebuild national dispatch, multiplayer, a route editor, detailed passenger lives, or a generic simulation framework.

## B. Problems and Contradictions in the Current Brief

| Brief reference | Problem or tension | Recommended resolution |
|---|---|---|
| §7 definition of done versus §7 philosophy and §9 | A stable loop on one route could satisfy the literal completion clause while failing the promised variety. | Rename that clause “vertical-slice acceptance.” MVP release additionally requires the coverage, usability, content, and device gates in N. |
| §7 scope emerges from tests versus request for precise MVP | Freezing counts now would contradict evidence-led scope; leaving all counts open prevents production planning. | Adopt a candidate content baseline, explicit reasons for each count, and measurable promotion/cut rules. Freeze it after the representative slice. |
| §5 interaction every 5–10 minutes versus believable operations | Forced delays or spawned crossing trains can make the railway feel contrived. “Unscripted” also conflicts with reproducible testing if interpreted as uncontrolled randomness. | Seed and validate traffic scenarios that create interaction opportunities. Dispatch remains causal. Measure actual exposure across seeds; never manufacture unsafe authority or unfair holds. |
| §3 casual and enthusiast play | Different control complexity, information needs, tolerance for failure, and session lengths are unspecified. | Share one simulation with assisted and manual control profiles, information assists, separate score labels, and resume support. |
| §12 one-handed use versus accurate multi-control driving | One-handed operation of a landscape phone cannot be assumed ergonomic; simultaneous throttle, brake, horn, and camera input is difficult. | Offer a dominant-thumb assisted Quick Play layout. Manual driving may use two hands. Validate reach and stopping precision on real phones. |
| §9 short sessions versus route scale and realism | Long realistic interstation distances do not naturally fit a complete 5–10 minute trip. | Quick Play starts at a valid operational checkpoint and completes one leg or approach. Longer services chain these legs. Do not speed up trains to fit a timer. |
| §10 convincing physics versus §8 future shunting | A permanently indivisible train object would block coupling, reversals, and locomotive changes. | Store ordered vehicle instances with stable IDs; aggregate their dynamics initially. Design consist mutation, but implement it only when shunting is funded. |
| §11 mobile-first versus broad visual ambition | Crowds, detailed cabs, long consists, night lighting, rain, shadows, and AI can peak together at stations. | Profile their combined worst case, not isolated demonstrations. Spend detail on visible operational cues and constrain station density. |
| §18 sequence | Track follows “core train,” while save, mobile tests, audio validation, and optimization appear late or are absent. | Track topology and movement must co-evolve. Test Android, saves, asset cost, and sound throughout, before large content production. |
| §13 offline-first versus §14 ads/DLC | Ads, initial purchases, entitlement restoration, and asset acquisition may need connectivity. | Installed owned core content works offline. Online adapters are optional, fail gracefully, and cannot block a resumed journey. |
| §6 customer research | No sources, sample selection, frequency, or prioritization evidence is supplied. | Treat demands as hypotheses. Keep an evidence register and run observation-based playtests with casual players and enthusiasts. |
| §15 legal wording | “Facts generally safe” and “lightly fictionalized” do not establish rights to source images, recordings, marks, or appearances. | Record provenance and permission for every shipped asset. Use a release rights review; do not treat this brief as legal clearance. |
| §16 no deadline | Unlimited calendar time does not remove finite budget, learning capacity, or maintenance cost. | Use scope freeze, exit gates, change requests, and a bounded active backlog. New features must state what evidence justifies them. |
| §17 extensive AI assistance | Generated systems can be individually plausible but globally inconsistent. | Require a named state owner, dependency review, data contract, verification evidence, and human review for each change. AI does not replace engine or railway validation. |

The expensive assumption is not simply “large routes.” It is that a large route can also sustain unrestricted train variety, bespoke stations, crowds, dynamic lighting, and background simulation on unspecified Android hardware. Specify the supported envelope before committing to production volume.

## C. Missing Requirements

| Area | Requirements to add before the relevant gate |
|---|---|
| Product and authenticity | Chosen region and era; fictional/composite versus reproduced geography; intended simulation depth; reference sources; authenticity reviewer; content budget and acquisition strategy. |
| Gameplay | Service start/end, dwell and departure conditions, reversals, overspeed grace, signal-passed-at-danger behavior, collision handling, rescue/retry, pause, checkpoint, abandoning a run, deadlock recovery, and treatment of dispatch-caused delays. |
| Physics | Units and sign conventions; credible traction/braking reference curves; stopping accuracy; train-length and speed envelope; gradient/adhesion limits; assisted versus manual control behavior. |
| Railway operations | Selected rule family and territory; running direction; signal meanings; block boundaries; overlap policy; point locking; route release; platform compatibility; timetable feasibility; failure handling. |
| Mobile | Named device floor, GPU families, RAM, tested OS versions, orientation, safe areas, touch target sizes, audio focus, background suspension/process death, storage exhaustion, interrupted downloads, frame pacing, thermals, and battery-mode behavior. |
| World | Route length policy, geographical compression policy, coordinate reference, elevation datum, scenery density limits, streaming boundary rules, cab/external camera visibility envelope. |
| Content production | Vehicle pivots/sockets, units, coupler and bogie positions, clearance envelope, wheel radius, LOD/material budgets, texture standards, modular station kits, reference approval, source asset licensing, and import validation. |
| UI/UX | Menu-to-driving flow, service briefing, signal explanation, hold reason, loading/install state, readable speed/distance, results breakdown, pause/resume, left-handed layout, and failure recovery. |
| Audio | Recording provenance; reference engine states; looping standards; loudness/headroom; voice prioritization; announcement localization; cab/exterior mixing; headphone and phone-speaker verification. |
| Progress/save | Stable IDs, schema versions, profile versus active-session state, transactional rewards, migration, backups, incompatible content handling, active service continuation, and maximum accepted progress loss. |
| Testing/debugging | Railway invariant tests, seed capture, replayable input logs, graph validation, content cook checks, packaged-device soak tests, save fault injection, and a readable dispatch inspector. |
| Optimization | Per-device budgets, representative stress scene, sustained frame targets, memory headroom, streaming latency targets, asset dependency audits, and quality degradation order. |
| Accessibility/localization | Non-color signal cues, scalable text, optional haptics, captions, camera-motion limits, screen-reader feasibility assessment for menus, script/font coverage, text expansion, and unit conventions. |
| Release/business | Storefront, distribution model, crash reporting consent, telemetry policy, entitlement behavior offline, restore purchases, plugin maintenance, and source-control/backup ownership. |

These are design obligations, not a request to implement every future feature now. Unresolved items must have an owner and the gate at which they become blocking.

## D. Revised Project Requirements

The following replace ambiguous intent with proposed acceptance obligations.

| ID | Requirement | Acceptance evidence |
|---|---|---|
| R01 | The installed core game works offline without account creation. | Fresh offline start after installation, full mission, rewards, save, restart, and resume succeed. |
| R02 | Player and AI use the same infrastructure truth and train-control contract. | Identical command traces produce equivalent outcomes within defined numerical tolerance. |
| R03 | Safety-relevant state is independent of streaming and presentation quality. | Unloading all nearby signal/train actors does not release occupancy or authorize conflicting routes. |
| R04 | No train movement is governed by a camera-visible signal mesh or collision trigger alone. | Authoritative topology, whole-train occupancy, locks, and rule evaluation are inspectable without rendering. |
| R05 | Quick Play delivers a complete objective in approximately 5–10 minutes using suitable scenario slices. | Observed session-duration distribution, including legitimate traffic waits; long services remain available. |
| R06 | Living-railway interactions are causally generated. | In a seeded acceptance suite, at least 90% of intended 5–10 minute interaction scenarios expose one relevant traffic event; events are logged with causes. This percentage is a proposed product threshold. |
| R07 | Driving identity remains high quality across tiers. | Consistent braking, readable signals, characteristic horns, recognizable stock/cabs, and credible station arrivals pass user and reference review. |
| R08 | The supported lowest tier sustains a paced 30 fps target under representative prolonged load. | Device traces from the combined station/night/rain/traffic stress case, with targets defined in Q; no claim based on desktop previews. |
| R09 | A new variant within a supported vehicle family uses data/assets without core changes. | Add a second vehicle and formation through the normal authoring path; validation catches invalid combinations. |
| R10 | Saves survive interrupted writes and supported version upgrades. | Recovery, migration, missing-content, and duplicate-reward tests pass. |
| R11 | AI waits and delays remain understandable and bounded where the scenario promises completion. | Hold reasons are exposed; deadlocks are detected; generated/curated scenarios are checked for feasible completion. |
| R12 | Content additions do not force all content into memory. | Dependency and cooked-package audits demonstrate route/vehicle bundle isolation. |
| R13 | Gameplay correctness never degrades with graphics settings. | The same seeded scenario preserves authority, scoring, occupancy, and completion across tiers. |
| R14 | MVP completion requires operational variety and polish, not merely the first working route. | All N coverage and release gates pass; unresolved critical defects block release. |

Baseline assumptions: single-player; landscape presentation; one controlled train at a time; no walking avatar; broad-gauge-themed contemporary railway; one researched regional visual/operating context; no real-world live timetable connection. These are proposed defaults to finalize in X.

## E. Complete Game System Architecture

### Architectural boundaries

The system has five layers:

1. **Definitions:** immutable vehicle, infrastructure, scenario, rules, and presentation data.
2. **Simulation:** mutable train, occupancy, route-lock, dispatch, schedule, and station-service state.
3. **Application:** commands, mission lifecycle, scoring, progression, save orchestration, and content availability.
4. **Presentation:** train actors, scenery, cab controls, animation, sound, cameras, and HUD.
5. **Platform:** input devices, files, lifecycle, performance signals, optional commerce and telemetry.

Definitions feed simulation; application submits validated commands; simulation publishes snapshots and events; presentation consumes them. Presentation never writes occupancy, rewards, or route locks directly. Optional online services depend on application interfaces, not the reverse.

The following catalogue covers the final vision without requiring a separate class, plugin, or subsystem for every row. **Every system is modular at its ownership boundary.** “Reusable” means reused across trains, routes, or scenarios inside this game; it does not justify a general-purpose framework.

Legend: **M** = MVP at intended release quality; **S** = MVP simplified; **P** = post-MVP; **L** = long-term; **N** = do not build yet. “Data” identifies configurable definitions; mutable state never lives in shared Data Assets. Inputs identify hard dependencies; outputs identify principal communication consumers. Each row states purpose/responsibility, ownership, extensibility, release depth, and principal risk.

### Train and railway systems

| System and responsibility | Inputs/dependencies → outputs/consumers | Unreal ownership; data/reuse | MVP → long-term; risk |
|---|---|---|---|
| Player train: connect player intent to an assigned train | Controller commands + train ID → validated commands, feedback to HUD/cab | PlayerController + thin TrainPawn; data control profiles; reusable across stock | M: one assigned train → switch locomotives/cabs; risk: possession confused with state ownership |
| Locomotive: powered-vehicle capabilities | Vehicle definition + command + traction availability → force request/status to dynamics/audio | C++ traction model, visual components, Primary Data Asset; reusable capability families | S: two classes → MU, failures, rescue; risk: class-specific logic scattered in Blueprints |
| Coaches/wagons: vehicle properties and representation | Vehicle type + load + orientation → mass, brakes, dimensions, appearance | C++ instance record + data + pooled visual actor; shared schema | S: passenger and AI freight stock → cargo, doors, variants; risk: one class per livery |
| Formation: ordered physical vehicle identity | Vehicle instances + compatibility → train length, load, brake/traction aggregation | C++ formation service within train model; formation assets; reusable | S: validated fixed formations → runtime split/join; risk: loss of IDs and ordering |
| Train physics: longitudinal motion | Force, mass, grade, adhesion, restrictions → position, speed, acceleration | Fixed-step C++ simulation; curve assets; shared player/AI model | S: aggregate dynamics → segment brakes/slack; risk: unstable integration or implausible stopping |
| Wheel/axle movement: plausible rail following | Vehicle path samples + speed + geometry → bogie/body/wheel transforms | C++ path sampler + visual component/animation; geometry data; reusable | S: kinematic bogies/wheels → suspension/slip detail; risk: corner cutting and switch divergence |
| Throttle/brakes: command arbitration and actuator state | Player/AI intents + interlocks → traction/brake state to physics | C++ control model; curves/profile data; shared command interface | S: service/emergency/parking and traction cutout → independent/dynamic/full pneumatic detail; risk: instant universal braking |
| Cab controls: readable manipulation | Cab layout + train snapshot + touch hit → commands and instrument state | Thin BP assembly/components + authored cab asset; reusable control IDs | S: operational essentials → deeper startup/protection systems; risk: cosmetic control claims |
| Cameras: usable viewpoints | Train pose + camera mode + gestures → view/audio-listener/streaming interest | PlayerCameraManager + camera profile assets; reusable | M: cab/chase/trackside → cinematic tools; risk: unloaded scenery and motion sickness |
| Track geometry: physical path sampling | Authored alignment and vertical profile → arc-length pose/grade/curvature | Editor authoring + cooked C++ geometry data; reusable graph edge reference | M: validated track geometry → richer transitions; risk: render spline becomes logical truth |
| Railway network: connectivity and constraints | Nodes/edges/region connections → paths, legal transitions, spatial lookup | C++ graph owned by rail world subsystem; data-driven; reusable per region | M: local graph → regional hierarchy; risk: hard actor references |
| Stations/platforms: service locations and eligibility | Platform spans + formation + service → valid stop/dwell/departure events | Logical station service + optional station actors; station data; reusable | S: counted boarding and stop markers → varied operations; risk: head-only berth check |
| Signals: display operating permission | Rule policy + authority state → aspect/meaning to AI, HUD, lamp actor | C++ policy evaluator + BP signal presentation; typed data; reusable | S: coherent selected family → regional rule packs; risk: color directly controlling routing |
| Points/junctions: conflict-safe transitions | Route request + occupancy + lock state → proved/locked connection | C++ interlocking state + visual actuator; junction data; reusable | S: instant logical proof with visual motion gate → failures/local operation; risk: changing beneath train |
| AI driver: operate a train | Authority, route, speed profile, timetable, dwell → shared train commands | C++ finite-state driver; tuning assets; reusable for all compatible trains | S: fixed services → freight/shunting planners; risk: route planning inside visual AI actors |
| Traffic/dispatch: allocate movement opportunities | Service intentions + conflicts + priorities → route requests/hold causes | C++ rail operations service; policies data; reusable | S: bounded conflict resolution → network regulation; risk: starvation/deadlock |
| Scheduling: define intended service | Timetable + dwell + paths + actual clock → readiness, lateness, dispatch demands | C++ schedule records; imported tables; reusable | S: feasible curated schedules → dynamic timetable generation; risk: physically impossible timings |
| Railway operations: enforce permissions | Occupancy + proved points + route table + block state → authority, locks, release | C++ interlocking/block/rule services under rail world owner; rules data with typed algorithms | S: safe constrained operations → rule-specific complexity; risk: distributed authority |
| Freight: transport cargo | Cargo manifest + wagon capacity + service → load mass, objectives | Data + train/load model + scenario objectives; reusable | S: AI through freight only → player freight P; risk: cargo visuals disconnected from dynamics |
| Shunting: local formation operations | Yard topology + vehicle states + permissions → moves, coupling transactions | Future planner + existing train/network interfaces; reusable yard definitions | P: none at MVP → advanced yards L; risk: trying to retrofit indivisible train state |

### World, experience, and application systems

| System and responsibility | Inputs/dependencies → outputs/consumers | Unreal ownership; data/reuse | MVP → long-term; risk |
|---|---|---|---|
| Passengers: demand and boarding | Station/time/service/capacity → counts, dwell/load inputs, visual requests | Logical station counts + pooled local visual agents; profiles data; reusable | S: aggregate counts → journey demand; risk: individual persistent agents |
| Station activity/workers: believable activity | Service events + clock + authored slots → local animation/ambience | Station activity component/coordinator; role data; reusable kits | S: small crowds and staff roles → richer activity; risk: NavMesh and animation cost |
| Missions/scenarios: authored goals and initial conditions | Scenario definition + world events → objectives/results/checkpoints | C++ evaluator + data objectives; BP only bounded authored sequences | M: curated parameterized missions → generated scenarios; risk: scripts overriding safety |
| Career/progression: meaningful learning and unlocks | Completed results + profile → capabilities/content unlocks | GameInstance profile service; graph data; reusable definitions | S: compact mastery ladder → career branches; risk: grind or stranded progression |
| Economy/rewards: consistent settlement | Validated result + reward rules → idempotent profile transaction | Profile service, data rewards, typed ledger; reusable | S: reputation/unlocks, no spendable currency → optional economy; risk: duplicate payouts |
| Time/day-night: common simulation time and presentation | Clock policy/pause → schedule time + lighting parameters | Simulation clock in rail world; environment component consumes snapshot | S: fixed day/night presets and advancing service clock → smooth cycle; risk: multiple unsynchronized clocks |
| Weather: operational and visual conditions | Weather definition + scenario/seed → adhesion/visibility/audio/VFX parameters | Logical weather record + environment components; reusable profiles | S: dry/light rain presets → transitions/fog/storms; risk: rain becomes costly cosmetic-only layer |
| Environment: regional visual context | Region kit + time/weather → terrain, structures, foliage presentation | Streamed levels/instances + material parameters; reusable kits | M: one coherent region → multiple regions; risk: bespoke asset explosion |
| Streaming/loading: keep needed presentation ready | Route intent + speed + camera + memory budget → asset/cell requests and readiness | World streaming coordinator + UE streaming/Asset Manager; configuration data | M: bounded corridor chunks → bigger regional packages; risk: synchronous stalls or unloading critical data |
| Audio/Indian railway sound: convey machine and place | Train telemetry + environment + events → prioritized spatial/nonspatial voices | Audio components + reusable MetaSound/Sound Cue graphs and profiles | M: distinctive horns/traction/rail/station mix → richer regional sets; risk: generic samples/voice saturation |
| UI/HUD: explain state and choices | Read-only snapshots + profile + errors → widgets and command requests | UMG + C++/BP view models, localization tables; reusable widgets | M: complete session/menu/results loop → expanded tools; risk: widget polling and hidden holds |
| Input/touch: deliberate control commands | Enhanced Input + touch widgets + assistance settings → semantic intents | LocalPlayer/controller adapter + input data; reusable profiles | M: thumb-assisted/manual layouts → controller support; risk: stuck input after focus loss |
| Settings: persist user choices | User preferences + device capability → applied settings | GameUserSettings for engine options + profile preferences; reusable schema | M: audio/control/accessibility/graphics → more options; risk: unsupported combinations |
| Graphics/scalability: sustainable visual cost | Device profile + measured thermal/frame state → bounded quality changes | Device Profiles + performance coordinator; tier data; reusable | M: three tested tiers → broader device support; risk: changing simulation with graphics |
| Save/load: preserve trustworthy state | Versioned immutable snapshot + profile transactions → validated recovery state | GameInstance save service + custom SaveGame records; reusable migrations | M: profile and resumable service → cloud reconciliation; risk: torn multi-file state |
| Offline/content availability: gate installable content | Local manifest + entitlement cache → available selection/install states | GameInstance content service, platform adapters; reusable pack definitions | M: installed core offline → downloadable regions; risk: catalog hard-loads absent packs |
| Monetization: optional commercial flows | Explicit menu request + store response → entitlement/reward transaction | Optional platform adapter behind interface; product data | N: disabled → P after model chosen; risk: SDK dependency blocks gameplay |
| Analytics/telemetry: answer specific product questions | Consented events + bounded diagnostics → optional upload/local reports | Optional application adapter; event schema | S: local metrics/debug export → opt-in remote telemetry; risk: personal data/cost/noisy events |
| Tutorial/onboarding: teach operation in context | First-use flags + scenario events → prompts and practice objectives | Scenario objectives + UI; authored lesson data; reusable triggers | M: integrated lessons → vehicle-specific lessons; risk: teaching rules inconsistent with simulation |
| Accessibility/usability: alternate presentation/control | Preferences + UI/audio states → readable cues/captions/input layout | UI/input/audio profiles; reusable common controls | M: non-color cues, text size, left-hand and camera options → broader assistive testing; risk: inaccessible small instruments |
| Localization: stable language content | String keys + locale + fonts/audio pack → translated presentation | Unreal localization/text/string tables; reusable IDs | S: English/Hindi launch candidate → region languages; risk: rasterized text and missing glyphs |
| Performance management: enforce budgets | Frame/memory/thermal/stream metrics → scheduling and presentation budgets | Small platform-aware coordinator + profiler markers; tier data | M: bounded work and degradation → more automation; risk: self-amplifying quality oscillation |
| Debug/development tools: expose causes | Simulation snapshots + IDs + events → overlays, validation, repro traces | Editor-only module + development-only inspector; reusable schemas | M: graph/occupancy/dispatch/save tools → large-network editor support; risk: shipping debug dependencies |
| Content/data management: consistent authoring | Definitions + schemas + references → validated cooked catalog | Editor validators, import tools, Primary Data Assets/Data Tables; reusable pipeline | M: manual reviewed imports → regional content pipelines; risk: invalid data accepted silently |
| Asset management: control references and lifetime | Manifest + bundle requests → loaded handles, unload eligibility | Asset Manager + soft references; reusable bundle policy | M: preview/cab/exterior/route bundles → DLC dependency graph; risk: accidental all-content residency |

## F. Complete Unreal Engine 5 Architecture

### Ownership and lifecycle

Use one runtime game module and one editor-only module initially. Keep simulation, application, presentation, and platform directories distinct inside the runtime module. Split into plugins or additional runtime modules only when separate packaging, dependency enforcement, or reuse provides demonstrated value.

| Unreal pattern | Recommended responsibility | Explicitly excluded |
|---|---|---|
| GameInstance | Startup, service initialization, menu-to-world handoff | Train ticking, railway graph state, references to streamed actors |
| GameInstanceSubsystems | Profile/save orchestration; installed-content/entitlement access; optional telemetry adapter | Active-world occupancy or route locks |
| UWorldSubsystem rail owner | Current simulation clock, graph registry, train records, operations services, state snapshots | Global player unlocks or permanent settings |
| UWorldSubsystem presentation coordinator | Visual train activation, scenery streaming requests, local station activity budgets | Permission to move trains |
| LocalPlayerSubsystem | Input profile, accessibility/view preferences, local UI context | Mission or railway authority |
| GameMode | Start/end/pause/retry policy and scenario session orchestration | Per-wheel or per-signal ticking |
| GameState | Public read-only session summary: mode, elapsed time, result status | Duplicate canonical train or profile state |
| PlayerController | Input translation, assigned train ID, menus and possession coordination | Physics or interlocking |
| TrainPawn | Possessable view/control anchor and component assembly linked to a train ID | Sole owner of the logical train |
| Train/vehicle actors | Visible body/bogie/cab/audio representation; IDs and subscriptions | Global graph or traffic planning |
| Signal/point/station actors | Render current logical state and submit authorized interaction requests | Self-authorized aspect/point changes |
| Actor Components | Cohesive presentation abilities: cab instruments, wheel transforms, sound, camera anchors | One component per data field, block, axle simulation record, or passenger count |
| SaveGame | Versioned serializable records | Live UObject graph serialization as the persistence contract |
| Asset Manager | Discover, cook, load, audit and release content bundles | Vehicle instance state or gameplay entitlement decisions |

Subsystems provide managed lifetimes, but they are not justification for many globally reachable managers. Initialize dependencies explicitly and stop the world owner before discarding visual subscribers. Epic's [subsystem documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine) describes the lifecycle mechanism; the ownership split here is a project design decision.

### C++, Blueprint, interfaces, and events

- **C++:** graph/path math, fixed-step integration, occupancy and interlocking, AI driving, validation, save schemas/migration, and performance-sensitive batching.
- **Blueprint:** train/cab visual assembly, menu composition, bounded animation/VFX reactions, and authored scenario presentation. Keep graph depth and inheritance shallow.
- **Data Assets:** structured definitions containing curves, asset references, and typed configuration. Use composition rather than deep Blueprint inheritance to represent locomotive variants.
- **Data Tables:** flat importable timetable, reward, speed restriction, localization-key, and reference parameter lists when tabular editing helps.
- **Interfaces:** train commands; read-only train telemetry; content availability; save snapshot producer; optional commerce/telemetry; presentation binding. Avoid an interface for every local function.
- **Events:** typed notifications such as route granted, block entered/cleared, station served, result settled, and content ready. Use request/response for decisions that need success/failure; do not turn safety transactions into loose asynchronous event chains.

Every request carries stable identity and a result reason. Simulation transitions occur in a controlled update phase. Events are published after committed state changes. Listeners cannot mutate the state that is currently being iterated. Presentation subscriptions must unbind on unload, and newly loaded actors must request a current snapshot rather than relying on historical events.

### Update model

Start with one authoritative simulation thread on the game thread. Fixed-step longitudinal integration begins at a proposed 30 Hz, independently of render rate; UI and transforms interpolate. Operations are evaluated at boundary events and each relevant fixed step. Dispatch can run less frequently, but occupancy protection cannot wait for a distant AI update.

Bound catch-up work. Under overload, reduce presentation first; if necessary slow simulation time coherently rather than skipping safety transitions. Match mission time to simulation time. Expensive path searches may later use workers over immutable snapshots, with version checks before applying results. Do not begin with parallel mutation of Unreal objects or a custom job framework.

### Asset, rendering, and world choices

Prefer Mobile Forward as the first benchmark baseline, with conventional meshes, materials and LODs. Compare Mobile Deferred on the actual device matrix before freezing the renderer: it may improve the selected lighting workload. Neither option is automatically fastest for this content. Epic documents these [mobile shading trade-offs](https://dev.epicgames.com/documentation/unreal-engine/mobile-rendering-and-shading-modes-for-unreal-engine). Do not depend on Nanite, Lumen, hardware ray tracing, or Virtual Shadow Maps for baseline presentation. Epic still labels its Android desktop-renderer Lumen path experimental and targeted to high-end configurations in the [Lumen mobile guidance](https://dev.epicgames.com/documentation/unreal-engine/using-lumen-global-illumination-on-mobile-in-unreal-engine).

Use a small persistent route shell with explicit corridor/station sublevels as the initial streaming candidate. Compare against World Partition in the prototype before authoring the real route. Choose one primary streaming mechanism per region. The simulation graph remains independent either way. World Partition offers streaming sources and grid-cell management, but hard references between actors can cause them to load together; it is not a guarantee of bounded memory. See [World Partition documentation](https://dev.epicgames.com/documentation/unreal-engine/world-partition-in-unreal-engine).

Conventional LODs reduce individual mesh cost. HLOD proxies represent distant static scenery; never merge working signal lamps, moving trains, or interactive points into scenery proxies. Instancing is for repeated poles, sleepers, furniture, and vegetation within sensible culling cells. Unreal's [HLOD documentation](https://dev.epicgames.com/documentation/unreal-engine/world-partition---hierarchical-level-of-detail-in-unreal-engine) explains its static proxy role.

### Presentation architecture

**Materials:** a small family of mobile masters for opaque vehicles, buildings, terrain, foliage and limited glass/wetness. Use material instances, shared trim sheets/atlases, bounded texture samplers, and few material slots. Avoid deep layered masters and unconstrained static-switch permutations. Track rails need anti-aliasing/LOD review because thin geometry can shimmer.

**Lighting:** MVP day/night presets, one selected time-of-day sun/moon directional light, restrained ambient/skylight, short dynamic-shadow distance, and authored night emissives with a small local-light budget. Static lighting alternatives must be validated with the chosen streaming system. Do not bake midday shadows then claim a continuous dynamic sun is correct. Continuous transitions require a separate lighting production decision.

**Animation:** procedural bogie/wheel/body transforms, limited cab controls, and low-cost local NPC loops. Avoid a skeletal mesh or AnimBlueprint per axle. Evaluate pose sharing/animation throttling only if crowd profiling calls for it. No full suspension simulation or passenger interaction tree at MVP.

**VFX:** pooled camera-local rain, small brake/diesel effects where appropriate, and authored tunnel/weather zones; strict translucency/overdraw limits. Niagara is presentation only.

**Audio:** reusable parameterized sources, central concurrency/priority policies, cab/exterior mixes and regional banks. No always-active source per wheel or carriage.

**UI:** UMG reusable screens and view models; event-driven updates plus controlled-rate instrument updates. Keep simulation commands out of widget graphs. UI loading cannot synchronously resolve full train or route assets.

**Input:** Enhanced Input actions and touch widgets converge into the same semantic commands; the input adapter owns dead zones, detents, handedness and focus recovery.

**Configuration:** project defaults and platform INI files for technical behavior, Device Profiles for capability/performance tiers, GameUserSettings for user choices, and Data Assets for gameplay tuning. Changes requiring shader recook or restart must not appear as immediate runtime toggles.

**Gameplay Tags:** classification and filtering such as service passenger/express, traction diesel/electric, weather rain, and scenario skill. Use enums/typed records for safety states and stable IDs for entity identity. A tag does not enforce a valid state machine.

Epic's [Asset Manager documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine) supports primary assets and explicit loading bundles. Use separate metadata/preview, exterior, cab, and sound bundles, with measured residency and explicit release handles; soft references still require correct cooking and bundle configuration.

## G. Train System Architecture

### Identity and composition

A **vehicle definition** describes a locomotive, coach, or wagon class. A **vehicle instance** identifies a particular vehicle, its orientation, load, operational state, and definition version. A **formation definition** is an authored list and compatibility rules. A **train instance** contains the ordered vehicle instances, command source, path history, motion state, and current service assignment.

Do not model “locomotive owns coaches, coaches own wagons.” Locomotives, coaches, and wagons are peer vehicle entries within a formation. Some provide traction, some passenger capacity, some freight capacity; all contribute length, mass, rolling resistance and braking as applicable.

Each definition includes:

- Stable ID, content version, display/localization keys, provenance/reference status.
- Length, bogie centers, axle spacing/wheel radius, coupler offsets, collision/clearance envelope, orientation and reference datum.
- Empty and permitted loaded mass, axle-load constraints, maximum speed, brake family and effectiveness.
- Traction force versus speed, power limits, adhesion behavior, response lag, notch mapping, and compatible power supply where powered.
- Cab layout/control profile, cameras, livery options, exterior/cab meshes, LODs, materials, audio, lights and animation bindings.
- Capability/compatibility rules: brake connection, coupler type, electrification, permitted formations, supported reversing and multiple-unit behavior.

Unknown parameters must be labeled estimates with source confidence and calibration status. A familiar class name is not evidence that its physics has been reproduced.

### Dynamics model

Use physical SI units internally, with explicit conversion at Unreal's spatial and UI boundaries. Integrate train distance and speed along the traversed path. The baseline force balance is traction minus braking, rolling/aerodynamic/curve resistance, and signed gravitational force. Acceleration follows net force divided by effective train mass, including an optional calibrated rotating-mass allowance.

Important requirements:

- Traction is bounded by force-speed characteristics, available power, adhesion and interlocks; power-based force must be well behaved near zero speed.
- Grade resistance considers where the formation lies. Sampling at vehicle or bounded formation segments is preferable to using only the locomotive's gradient on a crest.
- Braking has response time and application/release behavior. An aggregate delayed brake model is acceptable initially, with delay/effectiveness related to formation characteristics.
- Keep service, emergency, parking and any supported independent/dynamic brake semantics distinct. Do not expose controls whose effects are not modeled.
- Coasting, low-speed settling, rollback, holding at rest and brake/throttle conflicts have explicit rules. Prevent integration noise from moving a stopped train.
- The maximum allowed speed is constrained by stock, route, turnout, active restrictions and current authority. This limit informs assistance/AI; manual player overspeed is possible and evaluated by rules.
- Wet adhesion affects both acceleration and stopping estimates consistently. Never display dry braking advice while applying wet braking forces.

The numerical model need not be perfectly deterministic across every processor. Require reproducibility within defined tolerances and fixed seeds; do not promise future deterministic multiplayer lockstep.

### Path placement and vehicle transforms

Store the actual ordered edge history traversed by the train, not just the front's current spline. Place each vehicle's bogies by walking distances backward along that history. Derive body orientation from bogie poses and wheel rotation from traveled distance and radius. Coupler spacing is a geometry constraint; tight curves need validation for overlap and chord effects.

Once the front chooses a junction, following vehicles follow that same recorded connection. They must never sample the point's new setting independently. Keep the junction locked until the rear plus applicable clearance has passed. Reversal changes the leading end and movement direction without scrambling vehicle IDs or orientation.

No physics collision contacts determine routine rail following. Decorative suspension and cab sway may respond to filtered acceleration and track roughness, with accessibility controls. Collision/SPAD handling at MVP can safely end or recover a scenario; physical derailment spectacle is not required.

### Controls, AI, and extensibility

Player input and AI both submit semantic intents through one arbitration layer. Only one command source controls a train at a time. Emergency intervention has explicit precedence; pause/focus loss cannot leave a stale touch command latched accidentally.

Locomotive characteristics + vehicle loads → formation properties → shared dynamics. The path and operations systems supply geometry, restrictions, and authority. The driver consumes these to issue commands. Stations evaluate berth, speed, dwell, and departure conditions. Missions consume resulting events and never bypass these checks.

Adding a new class within supported capabilities should require new data, validated assets, audio, and calibration tests. A genuinely new capability—steam traction, distributed power, vacuum brakes, or special protection equipment—may require a new strategy implementation. “Data-driven” does not mean every future physical behavior can be encoded without programming.

Future coupling is an atomic consist transaction: stop/align, validate couplers and permission, preserve vehicle IDs, create revised train membership, recalculate properties, and update path/occupancy/services together. Reserve that contract now; do not implement full shunting in the first prototype.

## H. Railway Network Architecture

### Separate the meanings of “route”

Use distinct terms to prevent a frequent architectural error:

- **Track edge:** geometric length of physical track between topology boundaries.
- **Train path:** ordered directed traversal of edges and junction connections.
- **Interlocking route:** an authorized movement through a protected station/junction area, including point positions and conflicting resources.
- **Service itinerary:** ordered calls and timing targets.
- **Playable corridor:** a packaged environment and collection of services.
- **Region:** a larger content/simulation partition with explicit boundary connections.

### Canonical representation

| Element | Required logical representation |
|---|---|
| Track | Physical alignment plus directional traversal permissions; neither a mesh nor a spline actor ID |
| Segment/edge | Stable ID, end connection IDs, arc length, horizontal/vertical geometry, speed/gradient/cant attributes, region ID |
| Junction | Connection topology plus explicit valid traversals; arbitrary node degree must not imply every turn is legal |
| Points | Position, requested position, proved state, locking owner/resources, associated junction traversals |
| Signal | Stable ID, directed edge position, protected movement/block, signal role, selected aspect/rule family |
| Block/detection section | Referenced edge intervals, direction policy, occupancy members, operating role; distinct from scenery chunks |
| Platform | Track interval, usable length, side, permitted direction, clearance, service compatibility and stopping marker set |
| Station | Group of platform/yard/operational IDs plus passenger/activity metadata |
| Stop point | Direction- and formation-aware target with permitted tolerance and berth constraints |
| Speed restriction | Edge interval, direction, speed, reason, validity window, applicable train classes |
| Corridor | Content manifest + graph subset/connection references + itinerary set; not one giant level |
| Train path | Directed edge sequence, selected transitions, route version and recent traversal history |
| Region connection | Shared stable boundary ID, alignment/coordinate transform, permitted movement and handoff contract |

Represent track positions as edge ID, distance along edge, and direction. Save these logical positions. Transform to local world coordinates only for presentation. Define consistent canonical coordinates, region origin transforms, meters versus centimeters, and numerical precision. Do not store national-scale positions in single-precision local vectors.

Graph topology should subdivide at operationally useful boundaries, but geometry samples need not become graph nodes. A track edge can have several speed/gradient intervals. Use precomputed arc-length sampling and validated continuity rather than runtime searching through arbitrary spline actors.

### Authoring and validation

Author rail alignments in editor tools; export/cook immutable graph and geometry data. Maintain one declared source of truth so manual changes cannot leave the visible track and graph mismatched. Validation must catch disconnected nodes, duplicate IDs, invalid radii/grades, abrupt vertical transitions, impossible platform fits, unsafe clearances, incompatible power supply, missing signal bindings, and invalid interlocking routes.

The MVP keeps its operational graph resident because it is small compared with scenery. Long-term, region graphs can load on demand while a coarse corridor/service graph remains available. Only aggregate regions whose services are causally independent of active conflict areas. At a boundary, a train has exactly one owner; transfer state with a handoff ID so it cannot disappear or duplicate.

Expansion should add regions and rule data without changing train-position semantics. This reduces rewrite risk; it does not remove future authoring, migration, or performance work.

## I. Signalling and Railway Operations Architecture

### Authority chain

Use this sequence:

Service intention → feasible path → dispatch request → interlocking/block checks → locked/proved route → movement permission → aspect presentation and driver response → whole-train passage → controlled release.

Occupancy, reservations, locks, and signal aspects are different state:

- **Occupancy** means the physical train footprint intersects protected track.
- **Reservation** expresses an allocation or intention; it is not evidence the track is occupied or safe.
- **Route/point locks** prevent conflicting configuration while a movement is protected.
- **Authority** specifies where and under what constraints movement may proceed.
- **Aspect** communicates part of that permission according to the chosen rule family.

Use movement-authority records internally even when reproducing lineside signalling. They are a software contract, not a claim that the railway uses a particular cab-signalling system.

### MVP operating design

Choose one researched operating context. Recommended initial content: double-track running plus a protected branching junction and loop/platform routes, with a consistent color-light presentation. Use a constrained, automatic-block-inspired running-line policy and explicit interlocked station/junction routes. Treat this as a documented game approximation until the selected rule pack is reviewed.

MVP rules must include whole-train block detection, opposing-movement exclusion, conflicting-route exclusion, point proof/locking, conservative route-end protection, a stop target, and braking-compatible warnings. An occupied block cannot be cleared because its train actor is culled. A train must not traverse a turnout until the visual transition is complete if the player can see it.

A simplified clear/caution/stop family can work only where warning distances support the worst authorized braking performance. Do not use three colors as a universal Indian signalling model. A caution indication is not an arbitrary universal speed limit. Distinguish route/turnout limits, warning of a stop ahead, and the actual authority endpoint.

### Safety invariants

1. In normal operation, at most one exclusive movement owns a conflicting resource set.
2. A route grants only if its required track, point states and protection conditions are satisfied atomically.
3. Train footprint includes its rear and the swept movement between updates; a long step cannot skip a detector or stop boundary.
4. A point cannot move under any part of a train or contrary to an active lock.
5. Release occurs only after the relevant rear-clearance and rule conditions, not front passage or elapsed time alone.
6. Lowering a signal does not instantly release a route already committed to an approaching train. MVP may conservatively hold the full route until passage; cancellations require a verified stopped/uncommitted condition.
7. Missing/invalid state denies new permission. This is a game robustness rule, not a safety certification claim.
8. A stale visual aspect cannot authorize movement. UI and AI inspect the authoritative snapshot.

If the player passes danger, apply the chosen fail/recovery policy while retaining actual occupancy. Preventing entry through assistance is a separate assist setting. Scoring must not silently treat a physically violated limit as though the train stopped.

### Dispatch, priority, and conflicts

Dispatch selects among eligible requests using scenario priority, scheduled departure, accrued waiting time and downstream capacity. Add aging so low-priority trains cannot wait forever. Do not reserve an entire corridor indefinitely. Reserve an appropriate movement horizon and require enough downstream space to avoid trapping a train across a junction.

Start with fixed feasible paths and explicit conflict groups. A wait-for graph or simpler bounded dependency inspector detects no-progress cycles. Resolution first changes uncommitted dispatch decisions; never unlock occupied track to “fix” a deadlock. If an authored scenario is unsalvageable, pause with a clear recovery/retry option and preserve diagnostics.

### Long-term realism

Introduce territory-specific block working, station working rules, multi-aspect families, approach locking, sectional route release, overlaps/flank protection, shunt signals, calling-on/permissive movements, failures and special authorities only as reviewed rule packs and capability changes. Single-line absolute block and automatic block must not be collapsed into one generic “next block clear” algorithm.

Indian Railways' [single-line automatic block rules](https://indianrailways.gov.in/railwayboard/uploads/directorate/safety/NWR_SR/NWR_SR_CH9_Eng.pdf) explicitly include direction establishment and protection beyond certain signals. This illustrates why a single occupancy-to-color rule is inadequate. The broader [General and Subsidiary Rules reference](https://iritmold.indianrailways.gov.in/instt/uploads/files/1681386072477-G%20%26%20SR%20corrected%20with%20CS-88.pdf) is a research starting point, not a substitute for selecting current regional rules. Do not copy a distance or exception from one territory into every route.

Evolution replaces or extends rule evaluators and protection resources while retaining graph IDs, occupancy, route-request/result contracts and train control. More realistic rules can require new track annotations and save migrations; zero content rework is not a credible promise.

## J. AI Train Architecture

Separate three responsibilities: a **service planner** determines itinerary, a **dispatcher** allocates movements, and an **AI driver** operates the train within its permitted path. Unreal navigation meshes are not the routing system for rail vehicles.

The MVP driver can use a compact state machine: preparing, awaiting authority, accelerating, cruising/coasting, approaching restriction/stop, braking, dwelling, ready to depart, completed, and fault-held. Inputs include actual train dynamics, upcoming speed restrictions, authority endpoint, station berth, departure conditions and schedule. Outputs use the same throttle/brake/horn command path as the player.

Compute a conservative speed envelope backward from upcoming restrictions and stops, accounting for gradient, adhesion, brake lag and margin. A simple stopping-distance estimate alone is insufficient on changing gradients or during brake buildup. Schedule pressure never overrides safety. Dwell completion does not imply authority to depart.

### Simulation levels

| Level | Presentation | Logical update policy | Promotion/retention rules |
|---|---|---|---|
| Near/interactive | Full relevant exterior, wheels, lights and prioritized sound; cabs only where needed | Shared fixed-step dynamics, driver planning initially around 5–10 Hz with immediate critical-event response | Player visibility, short time to encounter, shared junction/block, or active mission relevance |
| Medium | Lower LOD, fewer transform/audio updates, no hidden interior | Same position/occupancy contract; cheaper driver planning, proposed 2–5 Hz | Promote before braking/conflict relevance, not only by distance |
| Far but active | Usually no actor | Coarser motion prediction with exact boundary events and conservative occupancy; no skipping protected resources | Retain active operations when it can affect a near train within the planning horizon |
| Background independent | No actors or frame ticks | Timetable/discrete-event records between validated boundaries, bounded event queue | Materialize a continuous compatible state before interaction; exact authority/occupancy ownership at handoff |

Rates are starting hypotheses. The MVP should use one shared logical dynamics implementation for its bounded AI roster, and vary visual/driver update work first. Add aggregate background motion only when measured scale justifies it; test its invariants before release.

Simulation relevance combines path time, resource conflicts, service significance and distance. A train on a parallel but disconnected line can be less relevant than a distant train holding the next junction. Promotion uses hysteresis and prediction to avoid oscillation and visible teleportation. Never reroll a train's delay or position when its actor respawns.

Delays arise from dwell, restrictions, route waits and seeded scenario events. Future dynamic service generation must admit trains only at valid entry/storage locations with capacity checks. Freight/shunting requires task planning, reversal and consist transactions; it is a later extension, not a large Behavior Tree attached to every MVP train.

## K. Living Railway Architecture

Believability depends on consistent causality more than independent activity volume. A signal hold, the train causing it, the station announcement and the eventual departure should describe the same event.

Use a bounded activity coordinator that consumes service events, station demand, time and weather. It requests local crowd/worker animation and ambient audio; it cannot spawn authoritative trains or bypass dispatch merely to create spectacle.

| Layer | MVP behavior | Later expansion |
|---|---|---|
| Operations | Visible passing trains, plausible holds, real route conflicts, understandable departures | Network delay propagation, rerouting, disruptions |
| Passenger demand | Counts by station/service; capacity-limited boarding and bounded dwell influence | Origin/destination flows, transfers, demand variation |
| People | Authored platform walking/waiting zones, small pooled crowds, staff roles and departure cues | More local behaviors, targeted interactions |
| Station life | Timetable-aware announcements, changing platform presence, train arrival/departure audio | Regional activity packs and special events |
| Environment | Local road/background loops, restrained ambient movement, day/night/rain presets | Richer regional and seasonal behavior |
| Events | Seeded dwell extension or traffic conflict within a validated scenario envelope | Procedural disruptions with solvability checks |

Passenger counts are authoritative; individual rendered people are representative samples. Do not imply that each visible person must persist or board one-to-one. Boarding must not extend dwell indefinitely because a decorative NPC animation failed. Empty platforms at quiet times are valid; perpetual maximum crowds are not more realistic.

Every system-driven interaction has a cause ID and log entry. Measure whether players noticed and understood it, not merely whether the scheduler recorded one. A train hidden behind scenery does not automatically satisfy the differentiator.

## L. World and Environment Architecture

Build railway corridors as authored experiences, with sightlines and landmarks organized around the train and permitted cameras. One researched regional kit should establish coherent vegetation, soil, architecture, platform furniture, signage, overhead equipment and ambient sound before adding new regions.

| Spatial layer | Content | Representation and constraints |
|---|---|---|
| Near | Track, signals, platforms, cab-facing structures, poles, staff and adjacent trains | Best useful LODs, readable signage, correct rail/OHE alignment, limited collision and shadows |
| Mid | Villages, roads, fields, station outskirts, yards and woodland | Modular meshes, instancing, simplified materials, reduced animation and lights |
| Far | City silhouette, hills, distant bridges, broad landforms | Coarse terrain, HLOD/proxies and skyline silhouettes; no detailed interiors or individual NPCs |

Distance bands are selected from camera visibility, train speed, sighting requirements and memory measurements, not fixed universal radii. Maintain distinctive terrain silhouettes so HLOD transitions do not change the apparent route geography.

### Predictive streaming

Load ahead using path direction, speed, possible near-term junction choices, and camera offsets. A useful minimum lookahead estimate is maximum travel speed multiplied by measured high-percentile load/activation latency, plus a visibility and safety margin. Also retain enough behind the front for the entire train, chase camera and rear-facing views.

Keep logical route/operations data available beyond braking and dispatch horizons even when scenery is absent. This logical horizon and the visual streaming horizon are different budgets. At a junction, prefetch plausible next cells before the player is committed. For quick-play placement or camera jumps, request content and verify readiness before revealing the view.

If required scenery is late, first suppress optional detail; if essential presentation cannot be made ready, use an explicit loading pause that stops the entire simulation clock. Never create a false red signal to hide a loading failure or let the train drive into missing track.

### Content construction

- Track geometry and clearances precede scenery dressing. Switch geometry and overhead equipment must match the selected track path.
- Stations use modular platform, canopy, signage, footbridge and furniture kits with a limited set of bespoke landmarks.
- Cities/villages are silhouette and frontage compositions; inaccessible building interiors are absent.
- Bridges/tunnels are special visibility/acoustic/lighting zones. Tunnel exits are streaming stress cases, not guaranteed free occlusion.
- Hills/fields/vegetation use regional distribution rules; procedural placement, if used, is primarily an editor production tool with reviewed cooked results.
- Roads provide limited local activity; no complete city traffic simulation is necessary to support the rail experience.
- Yards contain visually plausible storage and bounded active tracks. Every visible wagon need not be a fully active train entity.

Continuous day/night is deferred until lighting and material assumptions support it. Weather is local presentation driven by a shared logical state; particles do not fill the whole route. Large-network expansion uses independently installable region manifests and stable seam definitions, not a mandatory nationwide seamless map.

## M. Gameplay Architecture

The practical loop is: choose a service → understand formation and conditions → drive while observing cues → respond to traffic and restrictions → serve stops → receive an explained result → unlock a meaningful variation → replay or continue.

### Modes and objectives

- **Quick Play:** a valid checkpoint-to-checkpoint service slice, usually one leg, approach, or controlled departure. Show expected duration and the operational challenge. Finish with a complete result, not an abrupt timer expiry.
- **Career:** short lessons and service assignments grouped into a mastery ladder. Teach stopping first, then restrictive signals, traffic waits, heavier formations and adverse conditions. Longer runs connect several legs and support resume.
- **Free Drive:** select installed compatible train/formation, corridor itinerary, day/night, weather and a validated traffic preset. No creator UI is necessary.
- **Scenario replay:** repeat with changed starting delay, traffic ordering, formation load or weather within feasible bounds. Seed variation cannot invalidate required platforms or create impossible stopping distances.

Objectives are typed conditions: serve station, respect speed profile, obey stop authority, depart after conditions, complete itinerary, or recover acceptable lateness. Evaluate these from logical events and intervals, not frame-by-frame widget observations.

### Scoring and progression

Safety is a prerequisite for high grades. Reward accurate station stops, smooth acceleration/braking, appropriate speed and schedule performance. Explain penalties at the event and in results. Separate player-caused lateness from imposed traffic holds; use dispatch-adjusted expectations or visibly reduced punctuality weighting.

Do not reward approaching a danger signal too fast to “beat” a timetable. Cap or normalize repeat rewards to prevent trivial farming if an economy is later added. At MVP use reputation/mastery and unlocks rather than adding a currency with no meaningful spending purpose. Track assisted/manual grades separately without locking casual players out of basic content.

Failure policy: danger-signal violation or collision ends a scored scenario or offers an explicitly marked assisted rewind to a safe checkpoint. Minor overspeed and stopping errors reduce score within defined tolerance. Explain the reason and provide retry; do not erase career progress. Free Drive may allow a less punitive configurable recovery policy.

### Specific touch-control recommendation

Use landscape orientation with a movable left/right dominant-thumb control cluster. The primary lever has distinct brake, coast and traction regions with large detents, clear state labels and optional haptics. It issues traction/brake intent through an assist adapter. Provide a large reachable horn button, separate emergency brake, and tap-to-cycle camera; camera gestures use a non-control zone. Protect emergency input from accidental drags without requiring a lengthy confirmation.

Manual mode exposes separate throttle and automatic-brake controls and the supported cab functions. Cab tapping and HUD controls share the same command semantics. Do not use tilt control as the baseline. Optional speed assistance must drive the actual controls and respect authority; it must not directly set velocity.

Always show current speed, permitted speed, next relevant signal meaning/distance, next stop and stopping target, control state, and an understandable hold reason. Assistance may add a braking guide and route preview. Enthusiasts can reduce overlays. Signal meaning must be available without color alone.

Evaluate one-handed reach, accidental touches, glare, font size and precision on small and large phones. “One-handed” is an assisted layout goal to verify, not a promise that every phone can safely be held and driven with one hand. Pause must be immediate and reliable; resume begins paused with the previous controls and situation visible.

## N. MVP Scope

### Scope derived from experience coverage

The candidate below covers five essential contrasts: local versus express operation; lighter versus heavier handling; running line versus conflicting junction movement; busy station versus quieter landscape; and clear day versus wet/night visibility. Count content only when it provides a distinct experience or a necessary part of an authentic formation.

| Category | Candidate baseline | Why this is the minimum useful candidate; acceptance condition |
|---|---|---|
| Locomotives | Two drivable classes, preferably one electric and one diesel if the chosen regional reference supports both | Exercises different traction response, cab and sound configuration. A second livery is not a second handling experience. Confirm reference/audio/asset feasibility before class selection. |
| Passenger coaches | One coherent coach family with three required roles: general/seating, reserved/sleeper, and appropriate brake/guard/luggage role | Supports recognizable local and express formations without building two entire coach families. Exact roles follow the selected era/stock; validate authentic ordering and required service vehicles. |
| Freight wagons | One wagon family in a through-freight AI formation | Adds visible service/priority and length contrast without requiring player freight loading or shunting. Use a compatible locomotive definition and formation. |
| Formations | Three authored service archetypes: local passenger, express passenger, AI freight | Each creates different dwell, stopping or traffic behavior. Length/load variants count as tests, not advertised new train types. Validate platform fit and credible formation composition. |
| Playable network/routes | Two connected corridors with distinct service patterns sharing one junction | Tests actual branch choice and graph reuse while sharing production kits. Merely reversing one linear route does not meet this coverage. |
| Stations | Six named service locations in the candidate topology | Three endpoints + shared junction + one intermediate stop on each branch create local/express and shared-resource choices. Fewer locations must still demonstrate all those roles; extra stations need experience evidence. |
| Illustrative topology | Trunk endpoint → shared junction → branch A intermediate → endpoint A; junction → branch B intermediate → endpoint B | Provides two playable itineraries through a shared operational area. This is a topology proposal, not real geography or a route-length claim. |
| Environments | One regional kit with three treatments: station/town, rural open corridor, and contrasting terrain/infrastructure stretch | Visual progression without the production cost of several regions. A bridge, cutting or wooded section can supply contrast when regionally credible. |
| Route length | Set from intended service time, spacing and reference speeds after the drive test | Do not buy kilometers at the expense of pacing. Quick Play selects legs; full services retain plausible travel times. State any geographical compression openly. |
| AI traffic | A scenario roster supporting opposing pass, same-direction following/hold, and junction conflict; typical active slices involve two or three relevant AI services | These interactions require different timing/path relationships. Stress-test at least three interacting AI trains plus player, then set roster cap from device evidence; do not promise unlimited density. |
| Signalling | One coherent color-light rule presentation, protected blocks, locked junction/platform routes, whole-train occupancy | Hardware count follows braking and conflict geometry. Never reduce protection because a signal-model quota was reached. |
| Stations/passengers | All stations operationally correct; visible activity concentrated at the junction and selected busy stops | Every stop must work; costly crowds need not appear at equal density everywhere. Staff cues and announcements should agree with service state. |
| Missions | Eight authored foundations: start/controls, accurate stop, multi-stop local, express restrictions, following hold, junction conflict, wet braking, night operation | Each teaches/tests a distinct skill. Parameterized replay and combined career assignments provide longevity. Count only validated playable scenarios, not seeds as new missions. |
| Progression | A compact three-step mastery ladder: basic operation, traffic competence, adverse-condition/longer services | Gives direction and meaningful unlocks without a sprawling economy. Each step needs multiple attainable assignments and clear completion criteria. |
| Weather | Dry and light-rain presets with coherent visibility, audio and adhesion assumptions | Establishes a genuine operational contrast. Fog, storms and dynamic transitions are deferred. |
| Time | Day and night presets, selected at service start | Night is identity-relevant and tests signal/cab readability. No continuous lighting transition is promised for MVP. |
| Audio | Distinct locomotive/horn profiles, wheel/rail and brakes, cab/exterior mix, station/announcement and rain layers | Essential identity rather than optional polish. Shared systems, class-specific reviewed source material. |
| UI | Complete front end, service/formation selection, briefing, HUD, pause/resume, results, progression, settings, loading/recovery | A working driving scene without these is not a complete mobile game. |
| Cameras | Cab, follow/chase, and constrained trackside/lookback view | Covers driving, train appreciation and passing encounters. No unrestricted drone camera that invalidates streaming budgets. |
| Controls | Assisted dominant-thumb and manual layouts, horn/emergency/camera, readable supported cab controls | Serves both audience groups without implementing every physical switch. |
| Save/offline | Profile, one resumable active service, rotating recovery checkpoints, airplane-mode core play | Mobile interruption tolerance is release functionality. Multiple manual slots are unnecessary unless playtests establish need. |
| Performance | Three tested graphics profiles on a defined support matrix, bounded spawning/voices/crowds, predictive streaming and sustained-load evidence | Quality tiers are real validated configurations, not settings labels. |

### Scope freeze and release gates

This is the recommended planning baseline, not a unilateral production commitment. Freeze class selection, region, topology and counts after the representative slice proves their costs. Reduce duplicate content before cutting operating coherence, audio identity, control quality, or save reliability. If the two-class target proves expensive, reuse compatible components and reduce livery variety before removing the handling contrast.

The player-ready MVP requires all of the following:

1. All five experience contrasts are playable, understandable and enjoyable; both casual and enthusiast playtests support that conclusion.
2. Both corridors and all included formations pass topology, platform, signal, timetable and save validation.
3. Core-loop and interaction targets R01–R14 pass. The acceptance seed suite includes delayed AI and restrictive signals, not only uncongested runs.
4. No unresolved critical crash, save-loss, route-conflict, irreversible progression-block or offline-play defects.
5. Worst-case Android frame/memory/loading/thermal targets pass at the supported floor, including night/rain/station/traffic combinations.
6. Content/audio provenance, localization coverage, onboarding, loading/recovery and complete results/progression flows are reviewed.
7. The installed game feels complete without a store, ad, network account, or future-feature promise.

The first prototype can be much smaller than this. Its purpose is to retire risk, not to redefine the MVP downward.

## O. MVP vs Long-Term Architecture

Classification applies to the stated system depth, not whether the game's data is allowed to mention a future feature. **MVP** means release quality within scope; **simplified** means deliberately constrained fidelity with a stable contract. Post-MVP is the next plausible extension; long-term needs renewed feasibility review; do not build yet means no speculative implementation.

| System | Current classification | Extension and reason for boundary |
|---|---|---|
| Player train, input assignment | MVP | Future cab/train switching uses IDs and command-source ownership. |
| Locomotive capabilities | MVP simplified | Two supported traction families; additional physical capabilities require strategies, not class-name switches. |
| Coaches/wagons | MVP simplified | Shared vehicle schema; richer loads, doors and variants later. |
| Formation | MVP simplified | Fixed authored sets now; mutable split/join transactions post-MVP. |
| Longitudinal physics | MVP simplified | Calibrated aggregate forces now; distributed braking/slack later where perceptible. |
| Wheels/axles/bogies | MVP simplified | Correct path placement now; detailed suspension long-term. |
| Throttle/brakes | MVP simplified | Essential modeled controls now; full equipment procedures post-MVP. |
| Cab controls and instruments | MVP simplified | High-quality essential controls; deeper electrical/protection systems later. |
| Cameras | MVP | More cameras only if bounded streaming and usability remain valid. |
| Track/network/path semantics | MVP | Stable geometry and graph contracts now; larger graph partitions long-term. |
| Stations/platforms/stopping | MVP simplified | Correct stop/dwell rules now; complex platform reassignment post-MVP. |
| Signals | MVP simplified | One coherent family now; regional multi-aspect and special authorities later. |
| Junctions/points/interlocking | MVP simplified | Conservative locks now; approach/sectional release and failure modes later. |
| AI driver | MVP simplified | Shared physics and finite-state driver; specialized freight/shunt tasks later. |
| Traffic/dispatch | MVP simplified | Bounded conflict groups and curated paths; broad optimization long-term. |
| Timetable | MVP simplified | Feasible authored schedules; generated services post-MVP. |
| Railway operation rules | MVP simplified | Typed policy boundary now; reviewed regional rule packs later. |
| Passengers | MVP simplified | Counts and representative crowds; network journey demand long-term. |
| Station activity/workers | MVP simplified | Local roles and service cues; richer interactions post-MVP. |
| Freight | MVP simplified for AI; player freight post-MVP | Load/mass exists now; loading/unloading objectives later. |
| Shunting/coupling | Post-MVP | Preserve vehicle identity now; no yard planner before reversal/transactions are tested. |
| Missions/scenarios | MVP | Curated parameterization now; unrestricted generation long-term. |
| Career/progression | MVP simplified | Compact mastery ladder now; broader career branches later. |
| Economy/rewards | MVP simplified | Reputation/unlocks now; currency only after meaningful sinks are designed. |
| Weather | MVP simplified | Dry/rain presets; dynamic transitions and fog post-MVP. |
| Time/day-night | MVP simplified | Shared service clock and static lighting presets; continuous cycle post-MVP. |
| Environment | MVP | Polished one-region kit; multiple regions post-MVP. |
| Streaming/loading | MVP | Explicit corridor candidate; larger World Partition regions if benchmarked. |
| Audio/Indian railway identity | MVP | Reusable system and distinctive banks now; more regional recordings later. |
| UI/HUD and settings | MVP | Complete supported flows now; future features add views, not authority. |
| Touch controls | MVP | Assisted/manual profiles now; external controllers post-MVP unless easy to validate. |
| Graphics/scalability/performance | MVP | Sustainable budgets are launch requirements, not post-launch fixes. |
| Save/load/offline | MVP | Local recovery/migration now; cloud conflicts post-MVP. |
| Monetization integrations | Do not build yet | Set business policy first; introduce optional adapters after core release quality. |
| Analytics/telemetry | MVP simplified | Local useful metrics; remote reporting only with a defined question and consent policy. |
| Tutorial/onboarding | MVP | Train-specific deeper tutorials follow new supported controls. |
| Accessibility/localization | MVP within declared coverage | Broader languages and assistive support expand after actual testing. |
| Debug tools/content validation | MVP | Essential to one-person maintainability; richer authoring tools grow with pain points. |
| Asset management | MVP | Bundle/lifetime discipline now; downloadable region packaging later. |
| Full national network/dispatch | Long-term | Regional boundaries and IDs now; national optimization is not an MVP subsystem. |
| Multiplayer | Do not build yet | Preserve command/state separation; choose cooperative/competitive authority model only if product demand warrants it. |
| User route/scenario editor | Do not build yet | Internal validation first; public tooling adds security, versioning, support and moderation obligations. |
| Runtime generative AI | Do not build yet | No need for network LLMs in driving or signalling; development assistance is a different concern. |

## P. Data-Driven Architecture

### Definitions versus instances

Data-driven means content variation is declared, validated and versioned. Algorithms enforce physical and operational invariants. A timetable may state a departure time; it cannot grant permission to pass a stop signal.

| Data family | Recommended mechanism | Reason and validation |
|---|---|---|
| Locomotive/coach/wagon | Primary Data Assets + typed curves | Structured asset references/capabilities; validate dimensions, masses, speed, brake/traction curves and family compatibility. |
| Formation | Primary Data Asset containing ordered definition references and defaults | Validate length, orientation, couplers, brake family, power and service compatibility. Runtime instance IDs remain separate. |
| Livery/cab/camera | Data Assets and material instances | Reuse capability logic; validate socket/control bindings and camera bounds. |
| Track/network | Editor-authored graph compiled to compact cooked records | Efficient queries; stable entity IDs; topology and geometry consistency checks. |
| Station/platform | Structured station assets/graph annotations | Validate berth lengths, directions, stop markers, service routes and activity zones. |
| Signal/rule family | Typed assets plus tested C++ evaluators | Store role/aspect mappings and permitted policy parameters; avoid arbitrary unvalidated rule scripts. |
| Speed/temporary restrictions | Typed interval records; tables for authoring/import | Validate overlaps, precedence, units, direction, time windows and referenced edges. |
| Timetable/services | Data Tables or imported structured records | Bulk editing and feasibility checks; package per corridor rather than one national table. |
| Mission/scenario | Primary Data Asset with typed objectives, starting snapshot recipe and seed rules | Validate resource references, initial safety, prerequisites and reachable completion. |
| Rewards/progression/difficulty | Small tables/assets | Version thresholds; ensure no circular unlock requirements and distinguish assistance from physical rules. |
| Weather/time/environment | Profile assets + curves/material parameters | Share one state across physics and presentation; validate preset combinations. |
| AI | Driver/dispatch profiles | Bounded decision rates and margins; never let tuning disable interlocking invariants. |
| Sound | Class/region audio profile assets and reusable graphs | Bundle isolation, loop/loudness metadata, voice priorities and source rights. |
| Graphics | Device Profiles/INI + bounded quality preset assets | Keep platform features separate from user preference; validate cooked variants. |
| Localization | FText/localization pipeline and String Tables | Stable keys, script coverage and plural/text-expansion review. |
| Runtime state/save | Versioned typed records | Mutable state belongs here, not in shared authoring assets. |

Components provide per-actor capabilities, interfaces define boundaries, and subsystems own lifetimes. None is a replacement for a data schema. Gameplay Tags classify content; they are neither primary keys nor a physics rules engine.

### Content identity and delivery

Use stable logical IDs that survive asset rename/move, with a manifest mapping to Primary Asset IDs and soft references. Distinguish content version, save schema version and simulation/rules version. Migration tables explicitly map retired IDs where possible.

Every definition declares required capability/schema versions. Every content pack declares dependencies and installed/verified state. Keep lightweight selection metadata separate from large visual/audio references. Cook validation must verify referenced assets are packaged; a soft reference does not guarantee discoverability at runtime.

### Authoring pipeline

Reference/specification → source/provenance record → geometry/audio/data preparation → import → automated validation → packaged Android review → approval into a release pack.

For vehicles, review silhouette, bogies, cab sightlines, control placement, sound and stopping behavior together. For routes, review topology, signals, platforms, clearance and service feasibility before scenery polish. Asset budgets are part of acceptance, not an optimization request sent after production.

Use version control with large-file support or a suitable binary-asset workflow, backed-up source assets, and recoverable release tags. Store canonical source files once; avoid manually mirrored “master” copies. AI-produced changes need small reviewable diffs, reference checks, and tests for behavior that can lose saves or violate railway invariants.

## Q. Android Architecture and Performance Strategy

### Platform baseline and support policy

The current project log identifies UE 5.8.2, but that does not prove Android suitability. First validate a packaged build with the engine's matching SDK/NDK/JDK requirements, selected ABI, graphics API and all included native plugins. Maintain the exact toolchain manifest in project documentation. See Epic's [Android development requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-development-requirements-for-unreal-engine).

Target ARM64 initially. Use Vulkan as the initial test path; add another rendering/API path only if the pinned engine supports it and device coverage justifies a second test matrix. Do not promise every low-end Android phone. Define support by tested SoC/GPU/driver/OS combinations and sustained results, not RAM alone.

Check native-library and packaging compatibility on 16 KB page-size devices, including third-party SDKs. Android documents the relevant [page-size compatibility requirements and verification](https://developer.android.com/guide/practices/page-sizes). Store target API and asset-size policies are release checks against current official requirements, not permanent numbers copied into the game design.

### Candidate device tiers

The RAM bands below are planning candidates, not sufficient minimum specifications. Name and acquire representative devices before committing a support claim.

| Tier | Candidate envelope | Presentation behavior | Invariants |
|---|---|---|---|
| Low supported | Tested 4 GB-class ARM64 device with compatible GPU/driver; 30 fps target | Lower internal resolution, strongest LODs, shorter shadows, sparse representative crowds, reduced foliage/rain/reflections and voice counts | Same train handling, signal visibility/meaning, occupancy, objectives and traffic causality |
| Mid reference | Tested 6–8 GB-class device; 30 fps quality default | Better textures, medium shadows and station activity, longer scenery visibility | Same logical scenario; proposed main art/UX reference tier |
| High | Tested 8 GB+ device and suitable GPU; 30 quality or optional sustained 60 fps | Higher resolution/LOD ranges, more cosmetic activity/lights/voices within memory limits | No extra railway correctness; 60 fps is allowed only after a separate 16.7 ms budget passes |

### Provisional budgets to measure and revise

These are internal working limits chosen to expose feasibility problems early. They are not Epic guarantees or claims about all devices.

| Metric | Initial low/reference target | How to use it |
|---|---|---|
| Frame pacing | Sustained 30 fps; 33.3 ms deadline | Measure presentation intervals and CPU/GPU timelines after thermal stabilization. |
| GPU/game/render-thread headroom | Aim each bottleneck lane at approximately 25 ms or below at steady state | Threads/GPU overlap; do not sum them as independent full-frame allowances. Track end-to-end missed frames. |
| Core simulation | Start with ≤3 ms per rendered frame averaged over fixed-step work in the stress scene | Attribute graph, train, driver, dispatch and boundary processing separately; test worst-step spikes. |
| Stutter | Proposed p95 frame interval ≤34 ms, p99 ≤50 ms; no recurring >100 ms streaming hitches during driving | Measure warmed and cold-shader cases separately; investigate long-tail spikes even if average fps passes. |
| Process memory | Start with ≤1.2 GB steady and ≤1.5 GB peak PSS on the candidate 4 GB floor | Actual low-memory behavior may require lower caps. Track engine/graphics allocations as well as PSS; leave OS headroom. |
| Texture residency | Initial ≤256 MB low-tier texture-pool target | Not the entire graphics-memory budget; meshes, render targets, driver and upload transients are additional. |
| Audio residency | Initial ≤48 MB decoded/buffered active content | Stream/compress appropriate long material; account for decoder/IO CPU cost. |
| Train/scenery load transient | Must remain under the total peak cap while old/new chunks overlap | Test station boundaries and camera switches; avoid assuming unloading is instantaneous. |
| Render submissions | Initial ≤500 draw submissions in worst visible low-tier scene | Provisional diagnostic, not a universal engine limit; materials/shadows can multiply passes. |
| Visible geometry | Initial ≤600k conventional visible triangles low tier | Includes train and scenery; shader/overdraw/bandwidth cost can dominate below this count. |
| Cold menu / initial drive | Candidate ≤15 s cold to usable menu; ≤10 s selected service load on reference storage | Show truthful progress/cancel/recovery; distinguish first install from warmed runs. |
| Checkpoint resume | Candidate ≤5 s after app/menu availability on reference device | Tune only after state and asset bundles are stable. |
| Core delivered/installed size | Planning caps around 1 GB delivered, 2 GB installed | Measure actual textures/audio early; these are product budget proposals, not storefront limits. Split optional packs before bloating the core. |

No fixed memory partition should conceal a growing “other” category. Track engine baseline, textures, meshes, materials, audio, simulation, UI, render targets, streaming transients and driver allocation where tooling allows. Android typically shares physical memory between CPU and GPU; do not treat advertised RAM and “VRAM” as additive independent budgets.

### Cost controls

**CPU:** no tick per sleeper, track section, passenger record, coach subsystem or signal definition. Batch transform work, cache graph queries, bound path searches and event processing, reduce off-screen animation and avoid per-frame allocations. Use spatial/resource indices rather than scanning all trains.

**GPU/materials:** opaque surfaces, limited glass/translucency, a small number of material slots, restrained post-processing, authored mipmaps and LODs, bounded shadow casters, and measured night lights. Texture compression should match the validated device distribution; use an appropriate ASTC target where supported and avoid shipping unused texture variants.

**Physics:** custom constrained longitudinal motion; collision shapes only where interaction requires them. No Chaos constraint chain per train and no world-scale collision for background terrain objects.

**Crowds/animation:** representative local agents, small skeletal budgets, distance/update culling, no persistent global NavMesh crowd. Train bogies/wheels use procedural transforms; hidden cabs/interiors stay unloaded.

**Audio:** voice limits, class priorities, distance virtualization, streaming for suitable long recordings, and pooled emitters. Reduce distant ambience before the player's horn, control feedback or operational alerts.

**Spawning/GC:** pool only measured high-churn visual actors and cap the pool. Pooling everything can increase memory. Release asset handles and hard references deliberately, batch activation, avoid mass UObject creation at station entry, and schedule controllable cleanup at safe transitions. Profile actual garbage collection pauses.

**Shaders/loading:** constrain permutations, exercise expected materials and pipeline states in packaged tests, evaluate supported PSO precaching for the pinned engine/RHI, and test cold caches. A desktop editor warm run does not validate shipping shader behavior.

### Dynamic adaptation

Use stable device defaults plus a slow adaptive controller with hysteresis, minimum dwell times and user quality ceilings. Respond to sustained frame/thermal evidence: lower resolution first when GPU-bound, then shadows/VFX/foliage/cosmetic population; reduce presentation animation/voice work when CPU-bound. Increasing quality requires longer stable headroom than decreasing it. Keep frame pacing and a battery-friendly 30 fps mode.

Use platform thermal signals where available; Android's [Dynamic Performance Framework](https://developer.android.com/games/optimize/adpf) provides relevant facilities. Integration feasibility and fallback behavior must be validated in the chosen Unreal build. Do not rely on peak benchmark performance or thermal status alone.

Never reduce logical occupancy precision, signal sighting below required gameplay readability, the player's physics fidelity, or save reliability to meet graphics budgets. Lower AI presentation/decision frequency only within safety contracts. Avoid fluctuating traffic density mid-scenario; it changes gameplay and can invalidate causality.

### Verification matrix and mobile lifecycle

Use both Adreno and Mali families at minimum if both are supported, plus low/mid/high representatives, different display sizes and slow storage. Add any other GPU family only with explicit coverage. Test a 30–45 minute combined stress run unplugged, noting ambient conditions and battery state; repeat important cases warm, cold, low-storage and after background/resume. Record energy drain per session rather than claiming a universal battery percentage.

Handle calls/audio focus, headphones/Bluetooth changes, interruptions, screen lock, process termination, orientation policy, display cutouts and high-refresh screens. Suspend simulation and audio on background according to platform lifecycle. Save proactively; Android may kill the process before a final callback can finish.

Package only required maps/assets, audit dependency-driven size, and exclude editor/debug/test content from shipping. For future optional route packs, use verified manifests and appropriate delivery modes. Google documents install-time, fast-follow and on-demand asset delivery in [Play Asset Delivery](https://developer.android.com/guide/playcore/asset-delivery). Core offline promises must state which content is installed; a never-downloaded pack cannot be available offline.

## R. Save / Offline / Progression Architecture

### Separate records by purpose

| Record | Contents | Lifetime |
|---|---|---|
| Profile | Reputation/unlocks, mission completion/bests, career node state, stats, achievement progress, reward transaction IDs | Across app sessions and game-world travel |
| Active service | Scenario/content/rule versions, seed and RNG states, simulation clock, train/vehicle IDs, path/history/position, speed, actuator/brake state, load, relevant AI state, timetable/dwell, weather, objective/scoring state, operational locks/requests | One resumable run |
| Preferences | Audio, controls, accessibility, language, user graphics choices | Independent of scenario version; capability-clamped per device |
| Content/entitlement cache | Installed pack versions and local purchase evidence/status as appropriate | Optional service; does not replace store validation for acquisition/restore |

Save logical identities and positions, not actor pointers, UObject addresses, transient components or raw streamed-level state. Store physical state needed for continuity; do not reset brake application or active AI waits on resume.

### Save transaction and recovery

Capture an immutable consistent snapshot at a simulation boundary. Serialize asynchronously where safe. Write a new generation to a temporary/new slot, include schema/content identifiers, size and checksum, verify readability, then promote it through an atomic replacement mechanism validated on Android. Retain the last known-good generation. If the platform path cannot guarantee atomic rename, use alternating slots with generation numbers and validation; recovery selects the newest complete valid record.

Checksums detect corruption, not cheating. A local save cannot be treated as trusted evidence for competitive online rewards later.

Couple results and progression with a unique settlement ID. A crash between “mission complete” and profile update must neither lose nor duplicate the reward. Use an idempotent transaction/recovery record or a single coherent commit envelope; two unrelated save files with no transaction protocol are insufficient.

Save at significant milestones—departure, served station, pause, result settlement—and periodically at a bounded safe cadence. Proposed progress-loss tolerance is at most 30 seconds in normal driving, plus mandatory settlement protection; test write cost before fixing cadence. Background callbacks are best-effort additional protection, not the only save trigger. Permit a single active-run slot with rolling backups for MVP.

### Restore sequence

1. Validate record integrity, schema, required definitions and installed content versions.
2. Migrate supported schemas in ordered, tested steps while retaining the original backup.
3. Load the logical graph and restore train instances/path history and required dynamic state.
4. Rebuild occupancy from physical footprints. Restore and validate route locks/reservations; do not trust a stale derived aspect.
5. Reconcile conflicts conservatively. If consistency cannot be proved, recover a known safe checkpoint with an explanation rather than granting movement.
6. Bind visible actors and load required assets. Resume paused with the current situation and controls visible.

On missing optional content, retain the save and offer installation or a compatible checkpoint. On a changed route graph, use an explicit migration/checkpoint policy; guessing a new edge from world coordinates can place trains unsafely. Unsupported future saves must not be overwritten silently.

Epic's [SaveGame documentation](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine) supplies the serialization framework; corruption recovery, coherent snapshots and transactional rewards are additional project responsibilities.

### Offline boundary

Installed single-player missions, Free Drive, tutorials, local achievements, profile progression and resume operate offline. Initial content downloads, purchase/restore, rewarded ads, cloud sync, remote leaderboards, live events and multiplayer may require connectivity. Queue only explicitly chosen/consented bounded telemetry; expired online work cannot block game startup.

Future cloud saves need conflict handling for divergent profiles and active runs. Do not merge two in-progress worlds field-by-field. Allow explicit selection or a documented profile merge policy; preserve a backup. Future purchases require store-specific entitlement and offline behavior review before implementation.

## S. Audio Architecture

Indian railway identity requires researched source material and behavior, not a generic engine loop with an occasional horn. Each class/region audio profile maps semantic states to reusable graphs and licensed recordings.

| Layer | Drivers | MVP treatment and extension |
|---|---|---|
| Diesel engine | RPM/notch, load, startup/running state | Blended stable loops with response lag; startup/shutdown detail only if supported by gameplay |
| Electric traction | Speed, tractive effort, equipment state | Characteristic motor/fan/compressor layers; do not reuse diesel logic as a pitch-only substitute |
| Brakes | Application/release, speed, brake effort | Air events and appropriate friction/squeal layers; no continuous squeal at every stop |
| Wheel/rail | Speed, track joint/roughness class, curvature, bridge/tunnel | Distance-consistent rhythm and contextual layers; avoid one emitter per axle |
| Horn/whistle | Player/AI command, vehicle profile, distance | Class-appropriate attack/sustain/release and doppler/attenuation; essential voice priority |
| Couplers | Slack/coupling events where simulated | Limited movement sounds; coupling events added with actual shunting capability |
| Doors | Validated service/door state | Local audible cues for represented doors; do not claim fully animated boarding choreography |
| Station/platform | Arrival/departure/hold event, locale, activity level | Event-correct announcements and restrained crowd/worker layers |
| Environment/weather | Region, time, rain intensity, acoustic zone | Regional ambience and camera-local rain; tunnel/bridge/interior routing |
| Cab/interior | Listener position, windows/profile, equipment | Filtered exterior and near-control sounds, clear instruments/alerts |
| Infrastructure | Point motion, crossing equipment where modeled, station equipment | Triggered by real logical/presentation transitions, not independent random loops |

The audio system consumes normalized telemetry, not locomotive-specific Blueprint casts. Separate parameter smoothing from simulation state. Use reusable MetaSound or Sound Cue templates based on measured CPU and authoring needs; do not require a complex procedural graph for every source.

Define sound classes/submixes for train, operations alerts, station speech, ambience, UI and music if used. Keep operational cues intelligible under horns and rain; prevent clipping with headroom and concurrency policies. Offer independent volume groups, captions for announcements/important alerts, and optional haptics.

Voice priority: player controls and operational alerts → player locomotive/rail sound → nearby interacting trains → station speech relevant to the service → general ambience. Virtualize distant sources and stream long ambience/announcements where appropriate. A coach sound aggregate can convey length without a voice per carriage.

Announcement records use localized text keys and matching voice assets, with cancellation/priority rules. A delayed train must not play an on-time departure announcement from a disconnected loop. Regional language expansion should add banks, not rewrite service events.

Audio QA includes loop seams, pitch artifacts, volume transitions, cab/exterior camera changes, distance behavior, simultaneous trains, phone speakers, mono compatibility, headphones, focus loss and asset unloading. Keep a source/permission register and authenticity notes for each horn and locomotive bank. Defer mass audio production until the parameter model and two reference classes prove reusable.

## T. Development Dependency Map

### Critical path

Product/device/rule assumptions → Android packaging and measurement → graph/coordinates/vehicle schemas → train path placement and dynamics → occupancy/interlocking → AI and dispatch → representative streamed railway slice → player loop and save recovery → validated content pipeline → candidate MVP production → release qualification.

This is a dependency graph, not a waterfall. Save contracts, diagnostics and device testing begin with the first stateful prototype. UX, audio references and production research can proceed alongside technical validation, but they consume agreed contracts before producing expensive final assets.

| Consumer | Must already have | Why it cannot be skipped |
|---|---|---|
| Train movement | Track geometry, arc length, units, vehicle length/bogie data | Incorrect path semantics corrupt every later stopping and occupancy feature. |
| Whole-train occupancy | Path history, actual vehicle ordering, footprint rules | A front-position trigger cannot protect junctions or long trains. |
| Interlocking | Occupancy, valid route/conflict definitions, point proof | Signal color alone cannot establish a safe movement. |
| AI driver | Dynamics, braking limits, authority endpoints, station targets | Otherwise AI “works” only by teleporting or ignoring its physics. |
| Dispatch | Feasible routes, shared-resource conflicts, service readiness | Priority rules without downstream capacity create deadlocks. |
| Timetable content | Travel/braking/dwell reference measurements | Scheduled times must be achievable under the real model. |
| Passenger dwell | Station/service/capacity definitions | Crowd animation must not become the authoritative boarding model. |
| Mission results | Typed simulation events and failure/assist policy | Scoring copied from HUD values is fragile and exploitable. |
| Profile rewards | Stable result IDs and save transaction design | Completion must survive interruption without duplicate settlement. |
| Route scenery | Validated alignment, camera envelope, material/LOD and streaming budgets | Otherwise scenery has to be rebuilt around corrected track or device limits. |
| Final locomotive assets/audio | Vehicle schema, cab interaction and sound parameter contracts | Prevent per-locomotive bespoke logic and expensive asset rework. |
| Dynamic time/weather | Stable lighting/material and physical-condition contracts | Visual and braking behavior must agree across transitions. |
| Runtime coupling | Reversal, vehicle identity, transactional occupancy/formation updates | Mutating a train cannot leave orphan occupancy or broken services. |
| DLC/cloud/multiplayer | Versioned content/save identity and a chosen product model | Optional services cannot safely infer these contracts afterward. |

### Safe parallel work streams

After foundational decisions, the human/AI workflow can independently research vehicle references, draft scenario objectives, test UI reachability, record source rights, and prepare regional kit specifications. These are parallel work opportunities, not instructions to create agents or begin implementation now.

Asset production must wait for prototype budgets. Timetable authoring must wait for calibrated travel times. Large-network optimization must wait for measured region-scale bottlenecks. Multiplayer must wait for an explicit product decision.

## U. Technical Risk Assessment

Likelihood and impact are qualitative pre-prototype judgments: H = high, M = medium, L = low. They indicate investigation priority, not statistical probabilities. “MVP?” means whether exposure must be addressed before MVP release.

| Risk | Why risky | Likelihood / impact | Mitigation | First validation point | MVP? |
|---|---|---|---|---|---|
| Train dynamics | Bad braking/grade integration undermines the main activity | H / H | Reference curves, stable rest behavior, fixed-step model, stopping-distance fixtures across loads/grades | First motion prototype | Yes |
| Vehicle path placement | Long formations can cut corners or split at switches | H / H | Arc-length history, bogie sampling, locked selected transitions, clearance validation | First junction test | Yes |
| Signalling/interlocking | Distributed state can grant mutually conflicting movements | H / critical | One authority owner, atomic resource checks, invariant/property tests and readable inspector | Before AI traffic | Yes |
| AI driving | Driver assumptions differ from actual brake/traction response | M / H | Same commands/dynamics, conservative envelopes, adverse-condition tests | First AI service | Yes |
| Dispatch/deadlock | Feasible individual routes can form an impossible traffic pattern | H / H | Small conflict domains, downstream-capacity checks, aging and wait diagnostics | Multi-train junction test | Yes |
| Simulation LOD | Coarse updates skip occupation or create jumps | H / critical | Boundary events, swept footprint, causal promotion, continuity checks | Before any background aggregation | Yes if used; defer complex aggregation |
| Large environments | Scenery production cost and residency grow faster than route mileage | H / H | Corridor kits, sightline planning, package isolation, production-cost samples | Representative environment slice | Yes |
| World streaming | Fast trains/camera changes outrun activation | H / H | Predictive path prefetch, measured latency, full-train retention, explicit loading fallback | Early two-chunk test | Yes |
| Android GPU cost | Long consists plus stations/rain/night multiply materials and overdraw | H / H | Combined worst-case benchmark, conventional rendering, tier budgets | First device visual test | Yes |
| Thermals/battery | Brief high fps conceals sustained throttling | H / H | Unplugged soak, frame pacing, thermal-aware bounded adaptation, 30 fps default | Early representative slice | Yes |
| RAM/shared graphics memory | Engine baseline and overlapping load peaks can trigger termination | H / critical | PSS/allocation traces, asset handle audits, peak budgets, low-memory tests | First asset streaming test | Yes |
| Loading/shader hitching | Cold cache and IO cost differ sharply from editor runs | H / H | Packaged cold-run tests, bounded materials, precache evaluation, staged activation | Packaging/renderer gate | Yes |
| Storage/download size | Cab textures and unique audio inflate installation and updates | H / M–H | Content size reports, shared resources, compression and pack strategy | First two-stock sample | Yes |
| Device fragmentation/toolchain | Driver/native-plugin/package differences cause install or render failures | H / H | Pinned toolchain, real device matrix, 16 KB/package checks, conservative plugin choices | Packaging gate | Yes |
| Asset production | Solo development cannot cheaply sustain many bespoke vehicles/stations | H / H | Modular kits, one-region scope, validated second-asset reuse, provenance pipeline | Before content scope freeze | Yes |
| Animation/crowds | Many skeletons and navigation agents overwhelm CPU | H / M–H | Representative populations, local zones, animation culling, strict visibility caps | Busy station slice | Yes |
| Audio authenticity/memory | Suitable recordings are hard to acquire; layered voices saturate | H / H for identity | Early source audit, two class proofs, parameterized banks, concurrency/memory tests | Before final stock selection | Yes |
| Save corruption/migration | Interrupted writes and changed graph IDs destroy long sessions | M / critical | Alternating validated generations, snapshots, migrations, recovery fixtures | First resumable scenario | Yes |
| Rewards/progression | Repeat settlement or impossible unlock dependencies harm trust | M / H | Idempotent transactions, progression-graph validation, interrupted-result tests | First result/progression loop | Yes |
| Content scalability | Hard references and duplicated classes load or entangle everything | H / H | Soft-reference/bundle audits, stable IDs, add-second-content acceptance test | Second vehicle/route addition | Yes |
| Blueprint complexity | Large graphs and casts hide ownership and slow iteration | H / H | Thin presentation BPs, C++ core, bounded graph responsibility and event tracing | Every system review | Yes |
| C++ complexity | Excess abstractions, threading or engine changes exceed maintenance capacity | M / H | One runtime module initially, no engine fork, explicit ownership, simple algorithms first | Architecture/code review gate | Yes |
| Mobile controls | Small displays and thumb reach undermine precision | H / H | Real-phone usability tests, assisted/manual layouts, touch cancellation/focus tests | First motion prototype | Yes |
| Localization/accessibility | Fonts, color-only cues and narrow widgets exclude players | M / H | Early Hindi/text-expansion pass, shape/text signals, scalable UI, caption/control testing | HUD/menu slice | Yes |
| Authenticity/rule mismatch | Mixing region/era conventions produces internally inconsistent operation | H / H | Selected reference context, rule/stock review, documented approximation boundaries | Before route and signal assets | Yes |
| IP/content provenance | Untracked images/models/audio can block distribution | M / H | Source/permission register and release rights review | Asset intake, repeated at release | Yes |
| Multiplayer | Replication, bandwidth, authority and cheating need a different product design | H / H | Defer; retain commands and stable state; later separate feasibility prototype | Only after explicit scope approval | No |
| Runtime procedural/national scope | Generation can create impossible track, operations and production debt | H / H | Curated regions, validators, bounded seeds; no national simulation commitment | After regional scalability evidence | No |
| AI-assisted development/maintenance | Inconsistent assumptions and plausible-looking untested code accumulate | H / H | Small reviewed changes, ownership/contracts, reference cases, release tags, reproducible bug evidence | Every contribution | Yes |
| Monetization/cloud coupling | Network failures interrupt offline play or duplicate entitlements | M / H | Optional adapters, idempotency, disconnected-state tests; defer SDKs | Before optional integration | Core isolation yes; integration no |

Highest-priority risk cluster: path/footprint correctness → interlocking correctness → AI/dispatch continuity, tested together on Android with streaming. A beautiful empty train scene does not retire this cluster.

## V. Recommended Development Sequence

Each stage is a gate based on evidence, not a calendar deadline. A stage may loop back when its assumptions fail. Optimization and testing are continuous work, not final stages that compensate for an unsuitable architecture.

| Stage | Objective and required inputs | Systems built; dependencies | Validation and exit criteria | Common failure modes |
|---|---|---|---|---|
| 0. Decisions and references | Resolve X's prototype blockers; use brief, device inventory, rule/stock sources | Architecture decision records, supported-envelope draft, content/rights register; no game implementation in this review | Each blocker has a decision or a bounded experiment with success criteria; selected reference devices available | Endless design expansion; specifying exact stock without usable references |
| 1. Android feasibility harness | Prove packaging, tools and rendering on candidate floor; requires pinned engine/toolchain | Tiny packaged test map, profiling markers, representative material/light/mesh load; depends on 0 | Installs/runs offline; logs/traces/crashes can be retrieved; basic memory/frame cost known; renderer experiment selected | Desktop preview treated as device evidence; untested native plugins |
| 2. Rail movement kernel | Prove coordinates, topology, vehicle definitions and handling; requires reference geometry/curves | Graph, path sampler/history, formation model, longitudinal controls, simple visual binding and touch test; depends on 1 | Long train follows curves/junctions in both directions, holds still, brakes plausibly on grades; commands reproducible within tolerance | Only front follows spline; visual shortcut masks bad physics |
| 3. Railway safety kernel | Prove protected movement; requires footprint/path semantics | Blocks, occupancy, point locks, route requests/conflicts, signal views, debug inspector; depends on 2 | Automated conflict/whole-train/reversal/cancel tests pass, including actor unload; no invalid grant in seeded fuzz cases | Signal actors own truth; occupancy releases at the locomotive |
| 4. AI and operational traffic | Demonstrate causal interaction; requires safe authority and measured dynamics | AI driver, curated service schedules, dispatch, station dwell, hold explanations; depends on 3 | Passing/following/junction cases resolve safely; wait causes visible; no starvation in intended scenarios | AI uses different physics; arbitrary waits hide deadlocks |
| 5. Streaming and recovery stress slice | Prove runtime boundaries; requires actual simulation and representative asset load | Two streamable chunks, visual activation, early save/resume, state snapshots and lifecycle handling; depends on 2–4 | No occupancy loss on unload; max-speed/camera streaming within budget; process-kill recovery works; peaks measured | Geometry looks correct but state vanishes; save restores front only |
| 6. Representative experience slice | Prove fun, identity and production feasibility; requires stable train/operations contracts | One finished-quality station segment, two stock configurations, cab/touch/HUD, sound, night/rain sample, mission/result/profile loop; depends on 4–5 | Both audience groups can operate it; second stock is data-led; audio/visual/device targets credible; scope baseline N can be frozen | Polishing before controls work; calling the slice the full MVP |
| 7. Content and career production | Deliver N coverage using proven tools; requires frozen budgets/schemas/scope | Two corridors, six-role station network, formations, eight scenario foundations, career, localization and onboarding; depends on 6 | Every content addition passes validators and packaged checks; no new core branches for ordinary variants; progression completes offline | Duplicated Blueprints, infeasible schedules, unchecked assets |
| 8. MVP qualification | Validate the whole combined product; requires complete candidate content | Device matrix, soak/fault injection, cold-load/PSO checks, save migration fixtures, accessibility and release packaging checks; depends on 7 | R01–R14 and N gates pass; critical defects closed; rights/content audits complete; recovery/user flows verified | Only sunny empty route tested; average fps hides stalls and memory peaks |
| 9. Evidence-led expansion | Extend a released stable foundation; requires observed player demand and new budgets | Player freight, richer weather/rules/regions in separately scoped increments | Each extension states benefit, dependencies, migration impact, budget and acceptance tests before implementation | Building the long-term feature list indiscriminately |

### Test strategy across the stages

- **Pure simulation tests:** forces and units, braking envelopes, path traversal, reversal, footprint intervals, restriction precedence, formation compatibility, occupancy and conflict invariants.
- **Scenario integration tests:** station berth/dwell, delayed departure, different priorities, AI approaching red, emergency stop, downstream blockage, route cancellation, long train crossing a chunk boundary.
- **Property/seed tests:** randomized valid event orderings and resource requests; assert no conflicting permission and eventual progress where the setup guarantees a solution.
- **Persistence fault tests:** termination during writes/settlement, invalid checksum, old schema, missing pack, changed graph, no free disk, duplicate result and restored RNG continuity.
- **Packaged Android tests:** cold startup, shader/cache behavior, thermal soak, frame pacing, low memory/storage, audio focus, screen lock, process death and offline startup.
- **Human reference tests:** handling and sound plausibility, signal/region consistency, formation authenticity, one-handed assisted usability, manual-control clarity and gameplay pacing.

Every bug report should capture build/content versions, scenario/seed, logical time, assigned train, recent input/events, active route/occupancy state and relevant device metrics. Development overlays show logical train footprint, path history, blocks, locks, authority endpoint, AI state/target, streaming cells and memory. Release builds retain bounded diagnostics without shipping unrestricted debug controls or unnecessary personal identifiers.

## W. Recommended First Prototype

**Prototype question:** Can the selected Android floor run a controllable long train and interacting AI through correctly protected, streamed railway infrastructure while preserving state across interruption?

This is a risk prototype, not the MVP. Use simple temporary geometry and reference-quality parameters when implementation is later authorized; no production asset creation is needed at this planning stage.

Recommended test fixture:

- A short track graph containing a straight, curve, gradient, turnout, parallel passing track/loop and platform stop.
- One representative powered vehicle definition, a configurable multi-coach formation, and a second parameter configuration to expose hardcoded assumptions.
- Two AI movements whose paths can conflict with the player, with an optional third for deadlock/capacity testing.
- Whole-train detection sections, locked junction routes, stop/caution/clear presentation, and debug authority/occupancy overlays.
- Two scenery chunks with a boundary near the operational test area; essential graph data remains resident.
- Cab/chase views, assisted touch controls, a minimal speed/signal/stop HUD, one sample horn/traction profile, and a resumable checkpoint.

Do not require a full career, beautiful route or public menu flow to answer the prototype question.

### Experiments and pass evidence

| Experiment | Required evidence |
|---|---|
| Short/long formation through curve and switch | Bogies remain on the chosen path; rear follows historical transition; no point change under the consist; geometry tolerances defined and measured |
| Braking at different loads, grades and adhesion | Monotonic plausible response, no discontinuous stop/rollback artifacts, measured stopping distance compared with declared references/tolerance |
| Conflicting route requests | Exactly one compatible authority set, understandable holds and eventual progress in the feasible schedule |
| Near versus visually culled AI | Same occupancy and route protection; no teleport on reappearance |
| Unload/reload around train and signal | No change in authority caused by actor lifetime; restored presentation matches current snapshot |
| Max-speed chunk crossing and camera reversal | No missing essential view, bounded peak memory, measured load/activation lead time |
| Touch interruption/emergency | Commands cancel/recover correctly; emergency takes precedence; stopping can be performed on a real phone |
| Save/process termination/resume | Physical/operational continuity, no duplicate rewards, valid backup recovery, resumed world starts paused |
| Sustained device run | Measured headroom or a concrete bounded optimization path; no unsupported claims based on PC frame rate |

If the prototype fails, revise the failing model or budget before producing routes. For example, a shader-bound scene calls for rendering/content changes, not weaker interlocking. A switch-placement defect calls for path-history correction, not hiding coaches from view. Only after these tests pass should the prototype grow into the polished representative slice.

## X. Architecture Decisions That Must Be Finalized Before Implementation

Not every production detail must be locked before a technical experiment. Separate **decisions required before the first implementation** from **decisions that the prototype must resolve before production**.

### Before the first implementation

| Decision | Recommended default | Why it blocks coherent work |
|---|---|---|
| Prototype/MVP distinction | Adopt R01–R14 and N as proposed acceptance basis; first-route success is a slice gate | Prevents scope being silently changed by whichever prototype finishes first |
| Platform/toolchain | Pin installed UE 5.8.2 only after its Android package/toolchain check; document exact versions | Build and feature assumptions otherwise float |
| Device floor candidates | Identify named low/reference/high devices and GPU families; provisional 4 GB-class floor subject to evidence | No meaningful performance budget exists without hardware |
| Simulation ownership | Actor-independent rail world owner, thin presentation actors | Persistence, streaming and AI all depend on this boundary |
| Movement/units | Constrained longitudinal model; SI simulation units; edge-distance position and path history | Retrofitting coordinate/physics semantics would affect every system |
| Identity | Stable train/vehicle/edge/platform/service IDs; typed versions | Required for data reuse, saves and diagnostics |
| Safety model | Whole-train occupancy, atomic conflicting-resource grants, point locks, conservative release | Required before believable signals/AI can be designed |
| Command ownership | Shared player/AI semantic commands with explicit precedence | Avoids two incompatible driving implementations |
| Control/product policy | Landscape, assisted dominant-thumb plus manual mode, no walking avatar | Determines cab, HUD, input and camera requirements |
| Session clock/failure | Paused single-player world, simulation-time scoring, defined SPAD/retry behavior | Prevents contradictory mission and save semantics |
| Operating context | Choose region/era/rule references; prototype uses documented simplified subset | Signal roles and content topology require a consistent reference |
| Persistence contract | Separate profile/run/preferences with transactional settlement and versioning | Early state schemas must support interruption and migration |

### Resolve through the prototype before content production

| Decision | Experiment and recommended starting point |
|---|---|
| Renderer | Benchmark Mobile Forward against Mobile Deferred with actual night/rain/station content; choose one supported baseline |
| Streaming | Compare explicit corridor/station sublevels with World Partition for latency, memory, authoring and lighting compatibility; start with explicit chunks |
| Physics calibration | Select documented traction/brake targets and error tolerances; tune fixed-step rate and brake lag from results |
| Region/stock/assets | Select two classes and coach family only after source/audio availability and second-asset reuse are proven |
| Route scope | Freeze N's topology and counts after service duration, content cost and enjoyment tests |
| Lighting/time | Ship day/night presets first; decide whether baking is compatible with selected world workflow before producing lightmaps |
| Signal implementation fidelity | Have the chosen operating subset reviewed; identify what is deliberately approximated in-game |
| Performance caps | Replace provisional Q budgets with measured accepted numbers and named device coverage |
| Save cadence/loading targets | Measure IO and snapshot costs under interruption and low-storage conditions |
| Localization | Confirm launch languages and available translation/voice resources; English/Hindi is the current candidate |
| Monetization | Decide whether MVP is free, premium or otherwise distributed; keep gameplay independent and defer SDK implementation until needed |

Record the selected option, alternatives, evidence, consequences and revisit trigger in short architecture decision records. “Designed for scalability” is not evidence; adding the second stock type/route and preserving saves is.

## Y. Systems That Should NOT Be Built Yet

| Defer | Reason | Trigger to reconsider |
|---|---|---|
| Multiplayer replication/backend | No defined multiplayer experience; major authority, bandwidth and support cost | Proven demand and explicit cooperative/competitive mode proposal |
| Nationwide continuously active simulation | Unbounded complexity before a region is validated | Multiple shipped regions expose a measured cross-region requirement |
| Full pneumatic brake propagation and coupler slack solver | High calibration/debug cost before basic driving is verified | Aggregate model fails a perceptible requirement in player freight/long consists |
| Physics-based derailment/destruction | Large content/physics cost, little support for primary loop | Separate gameplay case with mobile budget and asset plan |
| Full shunting, locomotive swaps and rescue | Requires reversal, mutable formations, local authorities and richer objectives | Stable shared train/network model plus demand for player freight operations |
| Steam locomotive thermodynamics | A different control/physics/audio capability family | Explicit class scope and reference resources |
| Fully simulated passenger lives | Unnecessary for station believability and expensive in memory/CPU | A passenger-management game loop is deliberately added |
| Complex NPC behavior trees/global navigation | Decorative station activity does not justify them | Specific nearby interaction cannot be implemented with bounded local behavior |
| Continuous severe weather/seasons | Requires coordinated lighting, visibility, adhesion and content | Dry/rain/night presets meet device and gameplay gates |
| Runtime-generated routes | Invalid geometry/operations risk; hard to test | Mature authoring validators and a defined generation use case |
| User-generated route marketplace/editor | Tool UX, moderation, storage, versioning and security become products themselves | Internal tools mature and content demand justifies public support |
| Online leaderboards/live events/cloud | Entitlement, anti-cheat and sync design are unresolved | Local progress/save reliability proven and online purpose specified |
| Advertising/purchase SDK suite | Adds startup, privacy, build and offline failure dependencies | Business model selected and release integration explicitly scoped |
| Runtime LLM dispatch/announcements | Nondeterministic/offline/cost issues without a demonstrated gameplay need | A bounded optional feature proves preferable to authored rules/content |
| Generic plugin framework/large ECS rewrite | Abstraction and maintenance cost before scale evidence | Profiling or clear packaging boundaries justify it |
| Engine source fork | Long-term merge/toolchain burden for a solo developer | A demonstrated blocking engine defect has no supported workaround |

Do reserve essential contracts now: stable IDs, capability composition, immutable definitions, versioned saves, optional-service interfaces and independent logical state. Reserving a contract does not mean creating empty managers for every future feature.

## Z. Final Recommended Project Structure

Preserve the existing numbered workspace folders. The paths below describe the recommended organization; this review does not rename the Unreal project, create modules, alter settings, or move assets.

### Workspace organization

| Existing root | Recommended contents |
|---|---|
| `01_GameDesign/` | Vision, revised requirements, player experience, controls/UX, MVP coverage, mission/progression specifications, playtest evidence |
| `02_TechnicalArchitecture/` | Original master brief, this review, architecture decision records, system contracts, data schemas, performance budgets and risk register |
| `03_UnrealProject/MyProject/` | Actual Unreal project; retain current project/module identity until a deliberate rename before production |
| `04_Assets/` | Canonical source art organized by rolling stock, railway infrastructure, stations, regional kits and UI; provenance/import specifications |
| `05_Audio/` | Canonical source recordings, edit sessions, licenses/provenance, class/region bank specifications; runtime imports live in Unreal Content |
| `06_Routes/` | Source route references, track/topology specifications, timetables, operational validation and region manifests |
| `07_Documentation/` | Toolchain/device matrix, setup/build process, asset pipeline, QA/repro reports, release checklists and rights register |
| `08_Builds/` | Versioned packaged artifacts and associated manifests/reports; exclude bulky generated builds from ordinary source history as appropriate |

### Proposed Unreal source boundaries

One runtime module, using the eventual chosen project name, contains these logical directories:

| Directory | Responsibility |
|---|---|
| `Simulation/Network/` | Geometry records, graph, positions, paths and topology queries |
| `Simulation/Trains/` | Vehicle/formation instances, motion, controls, brake and traction models |
| `Simulation/Operations/` | Occupancy, interlocking, point locks, authority/rule policies, dispatch and schedules |
| `Simulation/AI/` | Driver state/planning and simulation relevance |
| `Simulation/Stations/` | Berthing, dwell, passenger counts and service state |
| `Application/Session/` | Scenario lifecycle, mode policy, clocks/pause coordination and application commands |
| `Application/Objectives/` | Mission evaluation, scoring and tutorial conditions |
| `Application/Profile/` | Career, unlocks, stats and reward transactions |
| `Application/Persistence/` | Snapshot records, SaveGame orchestration, migrations and recovery |
| `Application/Content/` | Catalog/pack availability, stable-ID resolution and bundle requests |
| `Presentation/Trains/` | TrainPawn, vehicle visuals, cab instruments and procedural motion components |
| `Presentation/World/` | Signal/point/station presentation, local activity and streaming coordination |
| `Presentation/UI/` | Read-only view models and bounded application command adapters |
| `Presentation/Audio/` | Telemetry/event mapping, priority and acoustic context |
| `Presentation/Camera/` | Camera manager and train-relative views |
| `Platform/` | Input, lifecycle, capability/performance bridge and optional online adapters |
| `Data/` | Typed definition schemas, gameplay classification and validation contracts |
| `Diagnostics/` | Bounded event traces, simulation inspector hooks and performance counters |
| `Tests/` | Non-shipping pure-model and integration test organization |

Use Public headers only for intentionally exposed module contracts; keep implementation details Private. The editor-only module contains graph/asset validation, imports, visualization and authoring helpers. It must not become a runtime dependency. Test/editor resources must not be pulled into shipping packages through hard references.

### Proposed Unreal Content organization

| Under `Content/IndianRailway/` | Contents |
|---|---|
| `Core/` | Small shared interface/presentation assets and common material/audio/widget primitives; not a dumping ground |
| `Data/RollingStock/` | Locomotive, coach, wagon, livery and formation definitions |
| `Data/Operations/` | Rule family, signal, service, timetable and dispatch configuration |
| `Data/Gameplay/` | Scenarios, objectives, career, reward, difficulty and tutorial definitions |
| `Data/Environment/` | Weather, lighting preset, regional and camera/audio profiles |
| `RollingStock/Locomotives/<Class>/` | Exterior/cab assets and thin visual Blueprint assembly; separate preview/cab/exterior bundles |
| `RollingStock/Coaches/<Family>/` | Reusable coach meshes/materials/visual bindings and role variants |
| `RollingStock/Wagons/<Family>/` | Wagon presentation and cargo variants when supported |
| `Railway/` | Track presentation, signals, points, OHE, signs and common station infrastructure |
| `Regions/<RegionID>/` | Route shell, chunk/sublevel or World Partition assets, terrain, station assemblies, scenery and regional data |
| `Environment/Shared/` | Approved reusable building/vegetation/terrain masters and instancing assets |
| `Audio/Shared/`, `Audio/Stock/`, `Audio/Regions/` | Reusable sound graphs and scoped runtime banks |
| `UI/` | Screens, widgets, icons, input layouts and presentation assets |
| `Localization/` | String Tables, fonts and localization assets with declared language coverage |
| `VFX/` | Bounded shared rain/exhaust/brake effects and presets |
| `Maps/Frontend/` | Lightweight menu/loading worlds without full-route dependencies |
| `Maps/Validation/` | Explicit non-shipping test worlds; excluded through cook/package rules |

Use consistent prefixes for discoverability—such as BP, DA, DT, WBP, SM, SK, M, MI and T—without encoding gameplay meaning solely in filenames. Keep Blueprint inheritance shallow and do not create separate gameplay Blueprints for every livery. Manage `Config/` and build settings in source control; generated `Intermediate/`, `Saved/` and local derived caches are not authoritative source assets.

### Architecture acceptance record

Before building the first prototype, review the decisions in X and record the selected defaults or bounded experiments. Before building production content, demand the evidence from W and the representative slice. Before calling the result an MVP, require N's full acceptance gates. This sequence preserves the living-railway ambition while making each expansion depend on a foundation that has actually been demonstrated.
