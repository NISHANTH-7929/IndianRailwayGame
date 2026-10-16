# StationArt status — 9 October 2026

## Latest: audio interaction gate passed

Device/oct10-audio-sequence3 passes horn, pause silence, loop restart and emergency
brake playback checks on APK20261010-082112. All settings restored, saves and APK
unchanged; phone idle. Not listening/full-lifecycle/performance acceptance.
Follow-up: emergency cab display B100/EMG conflicts with top HUD Brake0%; investigate
presentation. Fine labels and runtime station night lighting remain open.

## Current phone state

Phone tests stopped and original settings restored. Latest audio sequence2 sent
no touches: Android suspension/world exit at tick0 prevented readiness. Normal
saves/APK preserved. Needs unlocked foreground game for retry. 67 local tool tests
pass; not a phone audio-acceptance claim. See sequence2/REVIEW.md and PHONE_HANDOFF.

## Latest verified checkpoint — 10 October

Combined APK20261010-082112 passes clean legacy/LHB/station cold recovery and
263 fresh startup checks (`oct10-combined-legacy-recovery2`), plus exact completed
Standard Egmore recovery. All normal saves/settings preserved. 63 tool tests pass.
Audio smoke1 passes five asset loads and reported rail/station loops.
Audio-controls1 observes horn, paused silence and resumed loops, but action timeout
means only partial evidence; restoration passes. Brake/listening still open.
Retry2 has not passed preflight because the Wi-Fi phone disconnected again.
Unobscured screenshots still show indistinct small cab labels; not accepted.
Next bounded interior implementation: runtime night lighting, as recorded in
NEXT_INTERIOR_GATE.md. S8 remains incomplete; do not spend time on exterior polish.

## Current combined Android candidate — 10 October

Supersedes historical build/ownership statements below. Parent owns packaging
and ADB; APK20261010-082112 is installed, SHA256
`8889bf472b52ed3a8bf675d2c7e706e18b92a41dcdc1aa40a2eee171e79bd4cb`.
Cab contrast/newer audio included; 16 desktop suites pass. `oct10-cab-audio-smoke1`
passes asset loading and engine-reported rail/station loops, not audible quality.
Screens obscured by notification excluded from visual acceptance; recapture active.
`oct10-combined-egmore-recovery1` passes exact generation150 completed Standard
recovery with all ten checkpoint paths preserved. Legacy regression follows.
No exterior expansion. Final interior/cab visuals, audio listening/lifecycle,
night/rain and sustained performance acceptance remain open; S8 is not complete.

## Phone ownership update

10 October: Standard resumed itinerary PASSES on APK20261009-144106 with actual
scripted UI inputs, all seven phases/nine objectives, dwell10s, berth-1.404824m,
zero overspeed/harsh ticks, GradeB/86.4194. All preservation/restoration passes.
See Device/oct10-egmore-standard-itinerary2/REVIEW.md for resumed-run limitations.
Standard controls and cold recovery also pass. 63 device-tool tests pass.

Desktop chat completed a bounded lettering contrast revision with geometry and
controls unchanged, preserved newer Pixabay audio, and passed16 suites, then hit
its quota. Parent now owns the combined immutable Android packaging, build ID
20261010-082112, and ADB. No simultaneous desktop owner. Old APK remains installed
until a new verified install. Cab visuals/audio listening/performance still open.

CURRENT: settlement1 restoration PASSES. Recovered its completed isolated
checkpoint, saved one earned award/+10 reputation, then cold-restored the exact
settled state (generation98, fingerprint dcdde9a4ed3db7b7). Evidence:
`oct09-egmore-finish-reward1` and `oct09-egmore-settled-recovery1`.
Normal saves unchanged, all settings restored, wake0, game stopped. The phone
recovery/settlement/persistence gate is verified on 20261009-144106. No active
phone/build process. Historical blocking restoration note below is resolved.
Standard driving, final interior/cab visuals, weather/audio, combined performance
and release-catalog acceptance remain open; no full S8 completion claim.

BLOCKING DEVICE RESTORATION: settlement1 disconnected at tick7947/Departed before
any reward-save touch. Automatic restoration failed because ADB disappeared;
advertised Wi-Fi endpoint refused reconnect. Restore this run FIRST using
restore_egmore_interrupted.py after the authorized phone reconnects. Temporary
wake3/isolated commandline may remain; normal checkpoint backups are retained.
This interrupted run is not completion/settlement evidence. See PHONE_HANDOFF.md.

Latest reconnect: 20261009-144106 installed over Wi-Fi; install3 passes all
preservation checks. Egmore phone cold recovery passes exact fingerprint
87d49fe0fb69b87e, generation/service equality and paused presentation recreation.
All ten normal+isolated checkpoint paths remained unchanged and settings restored.
The new build also passes legacy/LHB/station recovery and 263 fresh startup cases.
Evidence: `oct09-egmore-phone-recovery1`, `oct09-resume-build-legacy-recovery1`.
54 mocked device-tool tests pass. Isolated reward settlement test is in progress
under parent ownership (`oct09-egmore-settlement1`). Full S8 remains open.

Continuation: isolated Egmore resume now has a development-only opt-in
`-S8AssetReviewResume` (requires `-S8AssetReview -S8Egmore`). Normal save paths and
gameplay formats are unchanged. Fourteen desktop suites and 50 device-tool tests
pass. Runtime cold resume preserves fingerprint/service/generation and stays
paused; missing/corrupt isolated slots fail closed without checkpoint writes.
All normal paths and the original isolated directory are restored/hash-verified.
Evidence: `Device/oct09-egmore-isolated-runtime2`,
`Device/oct09-egmore-isolated-resume1/UnrealAutomation`.
New immutable Android candidate 20261009-144106 built successfully (206.07s),
SHA256 92c96bdd077445973a7d05fcfefb5a82444f8924a11a8b52dd2fdba89fd2f5f9,
302123854 bytes. Install preflight found the phone disconnected; no mutation.
Last verified installed APK remains 20261009-121852. Device list and mDNS were
empty. The isolated phone recovery runner is ready; see PHONE_HANDOFF.md.

Latest verified follow-up: the playable APK passes all three legacy/LHB/station
cold recoveries with exact logical fingerprints and all eight normal checkpoint
paths unchanged (`Device/oct09-playable-legacy-recovery1`). Its fresh startup
reports pass 263 cases across ten groups. The isolated 60-second moving cab smoke
passes with 13 ready samples (`Device/oct09-egmore-cab-smoke1`). Main gauges and
indicators are readable; fine dark-panel labels still require visual work.
Both runs restored settings and stopped the app. Egmore-specific cold recovery,
settlement, Standard driving and final S8 acceptance remain open.

The dated progress entries below are historical; they do not supersede the
installed playable APK and successful checks recorded above.

RESOLVED: phone dropped during `oct09-egmore-assisted-itinerary1`.
No Depart input was sent before disconnect (last observed tick682/Approaching).
Normal save backups remain in that directory. Temporary wake3 and isolated launch
override may still be active; restoration could not reach device. On reconnect,
run tools/restore_egmore_interrupted.py with that evidence directory FIRST, verify
all8 normal checkpoint paths unchanged, wake0 and original override restored.
Restoration receipt now passes: all8 save paths unchanged, wake0, original override
and telemetry restored, game stopped. `oct09-egmore-assisted-itinerary2` is the
new isolated completion attempt; it now PASSES all7 service phases with actual
Depart touch, full10-second dwell, berth error−0.189334m and gradeA/98.58.
Reward not settled. All normal saves/settings restored. See itinerary2/REVIEW.md.

Playable APK20261009-121852 built successfully and installed, SHA256
5126ed14ce2d04cd9349e923aa1e099ee4cd0efab2ac6513b331199f7fd6be84.
Installation preserved saves/override/wake.90-second isolated Android smoke passed
with20 ready samples; both locomotives/Scene02/signals ready and simulation moving.
All restoration checks passed for smoke1. Screenshot inspection: no IsSky warning,
full-width view, readable HUD; final interior/cab visual acceptance still open.
43 mocked device tests and34 standalone identity/remainder checks pass.

Latest October9 continuation: integration/subagents hit usage limits, so this chat
completed interrupted signal/readiness integration. Editor compiles; all13 suites
pass in `Device/oct09-egmore-recovery-probe1/UnrealAutomationSignals`. Runtime
readiness smoke passes in `Device/oct09-egmore-playable-desktop2`:39 modules,
2594 static placements, two ID-bound signals, both WAP7 presentations ready,
valid advancing simulation, normal checkpoints unchanged. Prior failed smoke is
retained.40 device-tool safety tests pass. New EgmoreService Android package
20261009-121628 is building, not yet accepted/installed. Source builds include
separate Egmore cook directory and isolated signal material. No final S8 claim.

October9: installed Scene02 diagnostic APK passes all three existing-service cold
recoveries over Wi-Fi. Exact identities/fingerprints/service values and recreated
vehicles/station counts match; all six saves/commandline/wake preserved. See
`Device/oct09-scene02-cold-recovery2/REVIEW.md`. This is old-service compatibility,
not a claim that playable Egmore is already integrated.30 device-tool unit tests pass.
Save preservation now covers optional Egmore Service checkpoint slots as well as
the six existing legacy/LHB/station slots. Missing legacy recovery fixtures are
refused before launch; absent Egmore slots are recorded and checked for changes.

The integration chat stopped on a usage limit before packaging its playable
candidate. StationArt diagnosed its missing route-release test coverage and
changed the recovery scenario to hold at the berth until the AI route clears.
Gameplay/save formats are unchanged. Editor build and all13 Automation suites
now pass; see `Device/oct09-egmore-recovery-probe1/REVIEW.md`.
No new playable APK is accepted or installed.
The HUD signal-boundary and Egmore proxy-platform suppression fixes also compile;
all13 suites pass again after these edits. Remaining desktop handoff:
`Device/EGMORE_PLAYABLE_HANDOFF_20261009.md`.

Latest: Scene02 diagnostic APK installed over Wi-Fi; all six fresh Android camera
views captured and inspected. See `Device/oct08-scene02-tour-wifi1/REVIEW.md`.
Six saves, commandline and wake restored. SkyAtmosphere warning, dark fine cab
details and distant overhead-geometry artifacts remain. No final S8 acceptance.
USB attempts failed during transfer and are excluded. Wi-Fi endpoint selection
verifies the same authorized hardware serial before mutation.

User assigned this chat the connected phone on 8 October. The existing
`Create reference-based railway props` chat has acknowledged desktop-only
Unreal integration, validation and packaging; it no longer runs ADB.
Canonical candidate exchange is documented in `07_Documentation/S8/PHONE_HANDOFF.md`.
Source packages remain immutable. Focus remains interiors/platforms/player cameras.

Read-only phone preflight matched installed refined-cab APK SHA256
`2b75ffb51eeeaa3ec163ce37b34db3c7852b5b7da1cb9b21cb6a13c02f61d33d`.
Six existing saves recorded; wake setting 2. This is the old station baseline,
not Scene02. Evidence: `Device/oct08-phone-handoff-preflight/`.

New owned `device_review.py` requires exact installed/artifact SHA agreement and
verifies original commandline bytes, all six save hashes, wake restoration and
artifact immutability. Six mocked safety tests pass, including capture exceptions
and early setting failures. Static review never claims gameplay acceptance.
Scene02 Android capture remains pending an immutable desktop candidate.

## Latest: interior supplement v2.1.0

Sealed additive package `04_Assets/S8/Workstreams/StationArt/v2.1.0-interior/`
adds double-sided platform/exit signs, canopy lights and explicit light/material
bindings. Five new modules, 57 placements, 24,092 expanded triangles, no new delta
textures. 91 source checks and independent GLB policy pass. Four interior previews
were inspected. Base v2 hash verification passes unchanged. Read the supplement's
README before import: its complete Blender scene is preview context; InteriorDelta.glb
contains only the additions. Exterior work was not expanded.

Read-only integration observation: EgmoreScene02/unreal-import-progress.json listed
35 base modules with maximum bounds error 0.000005455m during this turn. This is
import progress, not final runtime visual, workload or Android acceptance.

User priority update: focus further work on the inside of the station and actual
player/train views: platforms, canopy undersides, signage, trackside infrastructure,
train fit and lighting. Hold exterior facade/street/context at the current level;
do not spend further time polishing them unless a player-camera defect requires it.

**v2.0.0 is sealed and ready for source integration review.** The assigned source-art
package has been built, rendered, inspected, exported and validated. Runtime and user
visual acceptance remain pending. No full S8 completion, survey accuracy, perfect
realism or Android result is claimed.

v1.0.0 remains withdrawn and must not be integrated. Its ready label was incorrect;
the prior status/handoff and evidence are preserved in the withdrawn-v1 documents.

## Deliverable

Source directory: `04_Assets/S8/Workstreams/StationArt/v2.0.0/`

- Blender source and assembled GLB; 35 reusable modules, 1,259 placements.
- 105 module GLBs and 105 FBXs across three actual geometry LODs.
- Unique LOD0 geometry: 65,618 triangles. Expanded scene LOD0/1/2:
  1,095,960 / 557,437 / 213,101 triangles.
- 25 bound materials, 40 bound texture resources; 45 standalone texture PNGs.
- Nine previews: five source views, imported-GLB view, day/wet, night/dry, night/wet.
- Bounds: X -144…144m, Y -48…72m, Z -0.82…15.35m.
- Placement manifest, stable-ID mapping, provenance, reproduction scripts and hashes.

## Verification

253 binary checks pass: complete UVs/normals/tangents, zero degenerate triangles,
actual gauge checked. 70 GLB/FBX module reimports match source bounds within 1mm.
Independent game-dev scene policy and canonical package hash verification pass.
The stated 3.25m wide stationary vehicle envelope clears the four authored tracks.
The sealed source has 288 hashed files; tools/verify_v2.py checks it without writes.

## Scene scope

Reference-informed heritage arcade, entrance, towers/domes and roof; three platforms
with detailed canopies, benches, bins, water points, fans and multilingual boards;
connected footbridge/stairs; rails, sleepers/clips, ballast, OHE, cabinets, ducts,
unbound signal housings; forecourt, road, boundary and adjoining trees/buildings.

Dimensions and layout are representative estimates. Background foliage is deliberately
low-detail. Night/wet images demonstrate source materials/lights, not runtime weather.

## Next owner

The integration chat must verify/copy accepted assets, resolve engine basis, instance
and cull modules, choose LOD distances, bind signal aspects, place actual WAP-7/LHB,
and validate player cameras, motion, lighting, saves, performance and Android.
User visual sign-off is pending. No Unreal/UAT/ADB/phone work ran in this workstream.

The game asset production skill supplied independent policy and package verification.
All production outputs stay in StationArt staging; WAG-9 and BLC remain deferred.
