# Playable Egmore readiness smoke — 9 October 2026

PASS: current-run world-load proof, IR-S8-EGMORE-1 / S8-Egmore-01, valid advancing
simulation at tick150, not paused or blocked. Scene rebuilt with39 modules and2594
static placements; two separately ID-bound signal housings replace the old static
unbound instances. Both WAP7 locomotive presentations report ready. Signals1/2
report proceed at world positions(-350,2.3,0) and(0,-2.7,.84). Track extension enabled.
All eight normal checkpoint paths unchanged; test process stopped.

This is a NullRHI smoke, not visual acceptance, complete service, or Android proof.
Prior desktop1 failed due to a load-transition interval reaching the fixed-step
overload guard. It is retained. The fix discards the single following presentation
load interval on a blocked/transition frame; normal active simulation still uses
the unchanged overload limit. Desktop2 then advances with a valid fingerprint.

Signal lens material created by create_egmore_signal_material.py, included in the
Egmore always-cook directory. WAG9 selector suppressed for the WAP7/LHB scenario.
Interrupted telemetry serialization and pointer-type errors fixed; editor builds.
