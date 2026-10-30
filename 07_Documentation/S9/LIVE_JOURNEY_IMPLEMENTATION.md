# Live journey implementation — 17 October 2026

Scope: retained WAP-7/LHB, representative Egmore–Tambaram alignment. New
service S9-MS-TBM-LIVE-01 / IR-S9-MSTBM-live-service-1 uses content
IR-S9-MSTBM-LIVE-1 and S9/EgmoreTambaram/LiveServiceV1 saves. Existing
S8 service and ServiceV6 checkpoints remain supported without migration.

New route: wait for the priority departure to clear the throat, traverse both
existing crossover arcs from track 1 to track 0, then continue to Tambaram.
Both points are reserved for the player movement; route release still requires
whole-formation clearance. The existing 30 km/h throat restriction applies.
Three opposing services start on real reverse-directed graph paths, with
independent route reservations and endpoint braking. No decorative teleports.

Five-train presentation includes every locomotive and existing approved coach
models. Assisted and Standard use the same protected routes; Standard retains
manual controls. Launch requires -S8Tambaram -S8Cab -S9LiveJourney.

Host validation: LiveJourney-Host-20261017-06 passes all 70 cases, including
three moving opposing encounters (closest sampled distances 11.47, 16.68 and
22.97 m, all approximately 9.8 m/s), crossover under both point locks, cold
recovery on crossover, manual controls, separate save identity and full dwell.
LiveJourney-Regression-20261017-02 passes 238 cases with warnings treated as errors.
Earlier encounter failure is retained: T4 reached its holding area too early;
its starting segment changed from 40 to 60. Failed build 01 retained two
shadowed-variable diagnostics, subsequently corrected. Shipping build 03 completed successfully, including nearest-opposing audio
selection across all three opposing services. Package hash manifest is recorded
in Evidence/LiveJourney-Shipping-20261017-03/package-manifest.json.

Packaged UI: both mode briefings verified. Assisted waited for priority traffic
and traversed edges 90000 and 90001. All four sampled crossover observations
showed both points locked for player train 2, route 2. No sampled simulation
faults. Save/exit and cold restart restored tick 12843 and exact fingerprint
96752b731205d63f, paused, retaining five trains. Evidence:
Evidence/LiveJourney-Packaged-20261017-01/verification.json.
Full terminal completion and three moving passes passed host tests; a full
new packaged journey remains pending.

Launcher: 08_Builds/S9_LiveJourney-Shipping-20261017-03/Windows/Start Live Railway Journey.cmd.
Current isolated test window PID40836 is paused after restart. Test profile:
Evidence/LiveJourney-Packaged-20261017-01/UserData. The normal launcher uses
the normal user profile and its separate live-service save.

Limitations: representative authored scenario, not current operational routing
or timetable. Existing generic/synthesized audio retained. Broader scenery,
coach fidelity and uninterrupted frame-pacing qualification remain open.
