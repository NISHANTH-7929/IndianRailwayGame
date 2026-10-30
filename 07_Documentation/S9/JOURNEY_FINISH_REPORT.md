# Journey finish verification

## Desktop result

The packaged live Egmore-Tambaram service completed at tick 52138 with grade A
(95.4723), berth error -0.604 m, and a completed 300-tick terminal dwell.
The reputation award was saved, then recovered after closing and restarting:
reputation 10, one award, mastery unlocked, and an identical settled fingerprint.
All 118 baseline checkpoints remained unchanged.

Candidate: `08_Builds/S9_JourneyFinish-Shipping-20261017-02/Windows`.
Use `Start Live Railway Journey.cmd`.

The finish includes shared foliage/ground material palettes, varied rooftop
details, 18 cab/coach material derivatives, five supplemental synthesized sound
clips, and a reset of opposing-train audio state when the nearest train changes.
Six focused Unreal audio/cab/coach tests passed. The simulation previously passed
70 journey checks and 238 regression checks.

Evidence: `Evidence/LiveJourney-Packaged-20261017-01/journey-summary.json`,
`terminal-telemetry.json`, `settled-telemetry.json`, and
`settled-reloaded-telemetry.json`. There were 367 journey observations, no sampled
faults or blocked states, and both points locked in the sampled crossover.
The first opposing pass has a screenshot; the second was observed through its
passing HUD. The third was covered by simulation/telemetry, not a visual capture.

## Limits

This journey was saved and resumed between builds. The 1200-second finish frame
capture includes driving and the result screen: p99 6.949 ms, maximum 2305.933 ms,
109 intervals over 100 ms. This is not a smooth whole-route performance pass.
Five-second telemetry samples cannot rule out stalls. Supplemental sound is
synthetic, not verified WAP-7 field audio. Scenery and dispatch are representative,
not a surveyed route or certified timetable. Further listening review is pending.

## Android extension, 18 October

The user authorized mobile implementation and testing. Connected device I2304,
Android 16. The Android ASTC Development candidate is being built from the same
source. Device installation, touch interaction, full journey, lifecycle recovery,
audio and sustained frame/thermal checks must be recorded separately; desktop
passes do not establish mobile acceptance.

## Completed mobile journey — Android07
Installed Android07 in place, retaining all18 older checkpoint files (final hash
comparison passed). Recovered tick42594 with exact fingerprint22b4169c9d5fbdf8.
Completed at tick52138: GradeA95.4723, berth error-0.603698m, dwell300ticks/10s,
overspeed0, player lateness0, traffic wait4767ticks/158.9s. Saved +10 reputation
and mastery unlock; force-stop/cold restart retained the exact settled fingerprint
and profile with awards=1. No duplicate reward.
Active-driving KEEP_SCREEN_ON observed on Android; released at Result. Final
approach completed without another overload pause. Startup log has no prior
GameFeatureData ensure. Plain journey command line restored; long CSV override off.
Across513 sampled observations:0 logical faults,2 presentation-blocked samples,
3 crossover samples with both points locked. These are sampled observations,
not a continuous smoothness certificate.
Evidence: Evidence/Journey-Mobile-20261018-01/journey-summary.json, terminal.json,
settled.json, settled-reloaded.json, preservation-final.json.
Remaining qualification: uninterrupted frame-time/performance pass; visual capture
of actual opposing train passes; acoustic fidelity and physical touch ergonomics.
Earlier thermal status3 and frame overload remain recorded. Current scene is a
prototype, not a surveyed railway replica; generated sounds are not authenticated
WAP7 field recordings. Full Standard-mode manual journey is not yet certified.
