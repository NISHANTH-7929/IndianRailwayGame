# Track and environment continuation — 27 October 2026

Current continuation: see [Track realism rework](D:/Project/IndianRailwayGame/07_Documentation/S9/TRACK_REALISM_REWORK.md). The candidate below is retained as the previous comparison build.

The final editor build, visual review, Win64 Shipping package and two packaged
checkpoint recoveries pass. Scope remains WAP-7/LHB and Egmore–Tambaram.

Run [Start Live Railway Journey.cmd](<D:/Project/IndianRailwayGame/08_Builds/S9_TrackEnvironment-Desktop-20261027-01/Windows/Start Live Railway Journey.cmd>).
Keep the entire Windows archive folder together. This is the desktop candidate
containing the track and terrain improvements below.

## What changed

- New gravel and dry-earth surface textures give the track bed and terrain
  visible grain. World-scale mapping keeps grain size consistent across cells.
  Restrained surface bump follows the texture in the correct lighting frame.
- The six-metre ballast mesh has irregular shoulders with matching seams at all
  three detail levels. Its buried toe meets the terrain without an exposed cut
  edge. Explicit XY UVs remove grey patches on steep shoulders. Small angular
  stones replace cubes and sit partly embedded in the bed.
- Polished rail heads, rust-coloured rail sides and bevelled concrete sleepers
  replace the previous flat appearance. Complete rail sections remain in all
  three distance levels, fixing disappearing distant rails. Vertical sleeper
  faces no longer have stretched grain.
- Ordinary track and turnouts share the same finishes. A broad ballast fill
  supports the long crossover bearers. Route sampling, gauge, rail height,
  sleeper spacing and point logic are unchanged by this art pass.
- Vegetation placement uses deterministic groves, jitter and gaps; shrubs use
  actual mesh bounds for ground contact. Instance limits are lower, with no new
  component family or per-frame scenery update.
- Runtime texture derivatives are limited to 1024 pixels and have generated
  detail levels for distance filtering. Source PNGs are preserved unmodified.
  The cab hardware mesh and its 15 finish materials are retained.

## Verification

| Check | Result | Evidence under `07_Documentation/S9/Evidence` |
| --- | --- | --- |
| Final editor compilation | Passed | `TrackEnvironment-20261025-01/editor-build-20261027.log` |
| Journey simulation | 77/77 passed | `TrackJourney-20261025-01/journey-results.json` |
| Final static scene review | 9/9 passed, 36 screenshots | `TrackVisual-20261027-01/render-results.json` |
| Actual component materials | Passed in all nine scenes | Each scene's `track-assets.json` |
| Compiled texture references | Exact final TexturesV2 bindings passed in all nine scenes | Each scene's `track-assets.json` |
| Shipping package | Passed, exit 0 | `TrackEnvironment-Desktop-20261027-01/result.json` |
| Cooked shader coverage / archive hashes | All seven materials present in SM5 and SM6; executable and pak/ucas/utoc hashes saved | `TrackEnvironment-Desktop-20261027-01/content-verification.json` |
| Two fresh-process checkpoint recoveries | 2/2 passed | `TrackShipping-20261027-01/result.json` |

Scenes cover urban, crossover, suburban, riverside, industrial, airport and
three opposing-train encounter positions. Source fixture hashes and logical
state remain unchanged. These screenshots use actual game components and
cameras. Another locomotive is visible in the opposing views; the crossover
view shows the diagonal rails, although the player train obscures some detail.
The component audit checks populated, registered, visible components and the
textures their compiled materials actually use, rather than only cached paths.

The journey checks cover protected traffic, crossover traversal, braking,
station/dwell progression and recovery in simulation. They ran before the last
art/telemetry corrections; those corrections did not change simulation logic.
Static screenshots do not establish moving traffic or point operation, a new
manually driven journey, frame pacing or sustained performance. Texture detail
levels address minification, but no measured performance improvement is claimed.
There is no mobile install or device qualification for this pass.

Packaged recovery checks use a copied urban checkpoint in an isolated UserDir.
They require the same logical state on two fresh processes, all scene/station
views ready, unblocked paused state, final track/turnout assets, retained cab,
and both exact cooked textures at 1024 × 1024 with 11 mips and streaming enabled.
This checks recovery of the same checkpoint; it does not test new reward writes
or cold-cache performance. Normal player saves are not used by these checks.
Both launches recovered tick 9035 with fingerprint `75ae0214f9bb165f`. Both
textures reported 1024 × 1024, 11 mips and streaming enabled. Per-launch runtime
telemetry is retained in `TrackShipping-20261027-01/verified-telemetry-1.json`
and `verified-telemetry-2.json`. The package verifier's cached asset checks are
paired with the editor's actual component-assignment checks above.

## Assets and reproduction

UE asset root: `/Game/IndianRailway/S9/TrackEnvironmentV1`.

| Mesh | LOD triangle counts | Notes |
| --- | --- | --- |
| `Meshes/SM_Track6m_RealismV1` | 992 / 192 / 132 | Three sections, both rails retained at every level |
| `Meshes/SM_Ballast6m_RealismV2` | 446 / 230 / 122 | One section, seamless ends, no collision |
| `Meshes/SM_AngularBallastStoneV1` | 20 / 12 / 8 | One section, no collision |

Final materials are in `MaterialsV2`; runtime texture derivatives are in
`TexturesV2`. The final import receipt is
`Evidence/TrackEnvironment-20261025-01/uv-mips-v2.json`. Rail/material receipt:
`visual-refinement.json`; persisted finish receipt: `finish-persistence.json`.
Older variants and failed/intermediate receipts remain for diagnosis; use the
27 October visual review as the final screenshot set.

Authoring tools are under `07_Documentation/S9/Tools`: `build_track_bed_v2.py`,
`build_track_rails_v1.py`, `import_track_environment_materials.py`,
`import_track_environment_meshes.py`, `refine_track_visuals_v1.py`, and
`import_track_uv_mips_v2.py`. Fresh-target guards protect earlier evidence.

Verification tools: `Review-Track.ps1`, `Package-Desktop.ps1`,
`Add-LiveJourneyLauncher.ps1`, `Verify-TrackPackageContents.ps1`, and
`Verify-PackagedTrack.ps1`. Use a fresh run label for each execution. Run only
one cook/import at a time; final archive shader metadata is copied immediately
after its successful cook to associate it with the hashed packaged files.

## Source and provenance

The built-in imagegen tool produced these two unmodified 1254 × 1254 source PNGs:

- [Ballast source](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/Textures/T_TrackBallast_BC.png)
- [Dry-ground source](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/Textures/T_DryGround_BC.png)

The exact prompt set is saved in
[generation-prompts.json](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/generation-prompts.json);
see also [ART_PROVENANCE.md](D:/Project/IndianRailwayGame/04_Assets/S9/TrackEnvironmentV1/ART_PROVENANCE.md).
The runtime derivatives resize without padding to avoid tile seams. These are
generated material images, not surveyed photographs or measured PBR scans.

The route remains representative procedural artwork. Broad flat ground,
sparse/repeated buildings, road repetition and some vegetation silhouettes
remain visible. This milestone improves track and terrain presentation; it
does not complete photoreal scenery or certify prototype accuracy.

