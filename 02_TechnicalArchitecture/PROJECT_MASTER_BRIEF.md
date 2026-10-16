# INDIAN RAILWAY — PROJECT MASTER BRIEF (v2)

## 1. PROJECT

Working title: Indian Railway

Platform: Android

Engine: Unreal Engine 5

Developer: Solo developer with extensive AI assistance

Primary AI development assistant: Astra / GPT

There is no fixed development deadline. Development speed is secondary to quality, maintainability, realism and scalability.

---

## 2. CORE VISION

The goal is to create a realistic, living Indian railway experience for Android.

The player should not feel like they are simply controlling a 3D train moving along an empty track.

The player should feel like they are operating a train inside a functioning Indian railway system.

The railway around the player should behave dynamically:

* Other trains operate
* Signals respond to traffic
* Junctions affect train movement
* Stations have activity
* Passengers and railway staff create life
* Weather changes the environment
* Day/night changes lighting and atmosphere
* Trains have authentic sounds and behavior
* Missions create different operational situations

The long-term goal is a large interconnected Indian railway simulation.

---

## 3. TARGET PLAYERS

Primary audiences:

* Indian railway enthusiasts
* Indian railfans
* Train simulation enthusiasts
* Casual mobile players interested in trains
* Players who enjoy realistic simulation
* Players who enjoy short mobile gameplay sessions

The game should support both casual and enthusiast-oriented play without forcing one group into the other's preferred complexity.

---

## 4. CORE GAMEPLAY LOOP

Core loop:

Drive
→ Observe railway conditions
→ Follow signals
→ Manage speed
→ React to traffic
→ Arrive at stations
→ Stop accurately
→ Complete objectives
→ Earn rewards
→ Progress/unlock content
→ Start another journey

The railway itself should generate interesting situations rather than every journey feeling identical.

---

## 5. KEY DIFFERENTIATOR

The primary differentiator is:

### A LIVING INDIAN RAILWAY

The game should prioritize the interaction between:

Train + Track + Signals + AI traffic + Stations + Passengers + Environment + Weather + Timetable/operations + Audio

The goal is not simply to advertise the largest number of trains, stations or kilometers.

The goal is to make the railway feel alive and believable.

**Success criterion (how this is measured, not just described):**
A player should encounter at least one unscripted or system-driven railway interaction — a crossing train, a signal hold, a junction conflict, a delayed departure — roughly once per 5–10 minute session. If a build cannot produce this, the "living railway" differentiator is not yet met, regardless of how much content exists elsewhere.

---

## 6. CUSTOMER EXPECTATIONS FROM RESEARCH

Public player feedback across Indian train simulation games repeatedly indicates demand for:

* Better AI railway traffic
* More trains crossing and interacting
* Better signalling
* More realistic junction behavior
* Longer and denser routes
* More locomotives
* More coach types
* Authentic train formations
* Freight trains
* Shunting and coupling
* Better train physics
* Authentic horns and sounds
* Detailed locomotive cabs
* Working cab controls
* Detailed stations
* Passenger activity
* Railway staff and environmental activity
* Day/night
* Proper night lighting
* Functional rain/fog/weather
* Better cameras
* Better UI
* More varied missions
* Timetable/punctuality gameplay
* Regional Indian railway authenticity
* Custom consists
* Eventually custom scenarios/routes
* Offline gameplay
* Stable performance
* Fast loading
* Fewer crashes
* Player-respecting monetization

A critical observation:

Players often care less about isolated graphical fidelity than about whether the entire railway behaves consistently.

Example:

A beautiful train with unrealistic signalling and an empty railway does not create convincing simulation.

---

## 7. MVP PHILOSOPHY

The MVP is NOT a toy prototype.

It must feel like a real version of the intended game.

It should contain enough content and gameplay variety that a new player can enjoy it and understand the long-term vision.

However, the MVP may simplify advanced systems and limit scale.

The MVP should prioritize:

* Quality over quantity
* Strong core gameplay
* Authenticity
* Replayability
* Performance
* Maintainability

Possible MVP content can include multiple locomotives, formations, routes, stations, AI traffic, signalling, weather, day/night, missions, progression, audio and passenger activity.

Exact numbers should NOT be arbitrarily fixed before the architecture and prototype are evaluated.

The MVP scope should emerge from development and testing.

**Definition of done:**
Because there is no external deadline, scope creep is the real risk, not lack of time. The MVP is considered done when the core loop — drive → signal → station → score — is fully stable and enjoyable across one complete route, regardless of how much content remains in the backlog. Additional locomotives, stations, or systems beyond that point belong to post-MVP iteration, not MVP completion.

---

## 8. LONG-TERM VISION

Eventually the game may contain:

* Large interconnected Indian railway network
* Many authentic locomotives
* Extensive coach/consist library
* Passenger and freight operations
* Advanced signalling
* Railway block systems
* Sophisticated AI traffic
* Junction management
* Timetables
* Delays
* Dynamic railway events
* Shunting
* Coupling/decoupling
* Loco changes
* Rescue operations
* Detailed stations
* Regional environments
* Dynamic weather
* Dynamic time
* Advanced career
* Scenario generation
* Sandbox
* User-created scenarios/routes
* Potential multiplayer

These are long-term possibilities, not MVP requirements.

---

## 9. GAMEPLAY MODES

Potential modes:

### Quick Play
Approximately 5–10 minute experiences. Designed for mobile users who want a complete short session.

### Career
Longer structured missions involving train operation, punctuality, safety, station stopping, traffic, railway events, progression.

### Free Drive
Allows players to choose locomotive, formation, route, time, weather, potentially traffic density.

### Future Sandbox/Creator Mode
Potentially allows custom scenarios, consists and eventually routes.

---

## 10. REALISM PHILOSOPHY

Realism should be prioritized where players can perceive and appreciate it.

High priority:

* Train acceleration
* Braking
* Train weight
* Sound
* Horns
* Cab appearance
* Signals
* Junction behavior
* Track interaction
* Station atmosphere
* AI traffic
* Railway operations

Not every subsystem needs mathematically perfect real-world simulation in the MVP.

The objective is convincing and consistent behavior.

---

## 11. MOBILE-FIRST REQUIREMENT

Although development happens on a PC, Android is the primary platform.

The architecture must account for:

* memory limitations
* GPU limitations
* CPU limitations
* thermal throttling
* storage size
* loading times
* scalable graphics
* LODs
* streaming
* efficient AI
* efficient NPC simulation
* asset compression
* battery usage

Do not design a PC game and attempt to port it to Android at the end.

Android performance must be considered throughout development.

---

## 12. MOBILE CONTROLS

Input scheme is a design decision, not a technical afterthought, and should be settled during architecture rather than bolted on at the end.

Candidate approaches to evaluate:

* On-screen throttle/brake sliders (most direct, closest to a real cab)
* Tap-to-set-speed (simpler, more casual-friendly)
* Tilt-based control (novelty, likely poor for precision stopping)
* Separate gesture zones for horn, camera, and signals acknowledgement

The chosen scheme must remain usable one-handed for short Quick Play sessions, while still allowing enough precision for accurate station stopping (a core scored mechanic). Astra should propose and justify a specific scheme as part of the architecture, not leave it undecided.

---

## 13. OFFLINE-FIRST

Core single-player gameplay should work offline wherever technically practical.

Online functionality can later be used for:

* cloud saves
* leaderboards
* events
* analytics
* optional content
* multiplayer
* downloadable content

The core game should not depend unnecessarily on an internet connection.

---

## 14. MONETIZATION PHILOSOPHY

The game may eventually use:

* rewarded advertisements
* optional advertisements
* cosmetic purchases
* premium content
* DLC
* other player-respecting monetization

Avoid designing monetization that repeatedly interrupts gameplay.

Player trust and retention are more important than maximizing short-term ad impressions.

---

## 15. INTELLECTUAL PROPERTY & TRADEMARK CONSIDERATIONS

The game is thematically built around Indian Railways, and may reference real locomotive classes, zones, liveries, or station names for authenticity.

To avoid legal or Play Store risk at launch:

* Do not use official Indian Railways (IR) logos, the IRCTC name/mark, or other official trademarks without permission.
* Prefer generic or lightly fictionalized branding for anything legally ambiguous (e.g., a "based on" livery rather than an exact official reproduction, fictional or composite station names where uncertain).
* Real, historically documented facts about locomotive classes, routes, and railway operations are generally safe to reference; official logos and registered marks are not.
* Revisit this section specifically before any public release or store listing, not just at the start.

---

## 16. DEVELOPMENT PHILOSOPHY

Development has no artificial deadline.

Do not optimize the architecture solely for speed of implementation.

Prefer:

* clean architecture
* modular systems
* maintainability
* scalability
* debugging simplicity
* performance
* data-driven design
* reusable systems

Build incrementally.

Do not generate the entire game at once.

Do not create huge interconnected Blueprints without clear responsibilities.

Do not implement a complex system before explaining its architecture and dependencies.

---

## 17. AI ASSISTANT ROLE

Astra should act as:

* Senior Unreal Engine developer
* Technical architect
* Gameplay programmer
* AI programmer
* Debugging assistant
* Optimization assistant
* Game design consultant
* Documentation assistant

However, the human developer remains the final decision maker.

Before implementing major systems, Astra should:

1. Understand the requirement.
2. Identify dependencies.
3. Propose architecture.
4. Explain trade-offs.
5. Identify risks.
6. Separate MVP requirements from future systems.
7. Then implement incrementally.

When code or Blueprint logic is generated, it should be understandable and maintainable by the solo developer.

---

## 18. DEVELOPMENT PRINCIPLE

Build in this general direction:

Architecture → Prototype → Core train → Track → Signals → Stations → AI trains → Railway interaction → First real route → Indian environment → Audio → Weather/time → Missions → Career → Progression → Optimization → Testing → MVP

This is a flexible order, not a rigid deadline.

The order may change when technical discoveries or testing justify it.

---

## 19. IMPORTANT PRODUCT PRINCIPLE

Do not compete purely on "more trains," "more kilometers," "more graphics," or "more features."

Instead aim for:

### MORE BELIEVABLE RAILWAY

The player should feel that the train, railway infrastructure, AI traffic, signalling, stations, environment and audio are all parts of the same functioning system.

---

## 20. CURRENT OBJECTIVE

Before implementing the game:

1. Review this brief.
2. Identify contradictions or unrealistic assumptions.
3. Identify missing major systems.
4. Design the complete technical architecture.
5. Explain system dependencies.
6. Separate MVP architecture from long-term architecture.
7. Recommend Unreal Engine patterns suitable for a solo developer.
8. Identify Android-specific risks.
9. Propose a development sequence based on technical dependencies rather than an arbitrary calendar.
10. Do NOT begin writing implementation code yet.

The immediate goal is to establish a technically sound foundation before building the first prototype.