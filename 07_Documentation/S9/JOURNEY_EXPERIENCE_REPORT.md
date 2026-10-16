# Journey experience changes - 16 October 2026

Scope: make the retained WAP-7/LHB corridor easier to experience as a continuous journey, and qualify actual packaged behavior. This is not a claim of fully realistic art, authenticated recordings or a surveyed operational railway.

Implemented:
- Station-aware route display derived from existing15 station chainages, with passing and terminal roles, elapsed journey time and progress.
- Whole-formation station clearing and current route limits; upcoming restriction advice from the existing driving profile.
- Spatial station-ambience fade at intermediate stations, using the existing licensed recording and existing cab filtering/mix.
- Compact HUD control; final source retains restrictions, emergency and service notices. Controls remain accessible.
- Shipping debug toggle removed/disabled; existing isolated editor diagnostics retained.
- Reproducible process/route/memory sampler and application-present timing summary. No telemetry upload.

Tests: JourneyExperience-20261016-02 passes62 cases, including five new presentation boundary/audio-fade checks. JourneyExperience-Shipping-20261016-01 build/cook passed. Packaged selection/briefing/confirmation/start/release/power/HUD and protected wait exercised through UI. Early120s PresentMon sample records17281 intervals,p957.5347ms,p997.9558ms,max42.9882ms,no>50ms. Scope is departure/junction only on this machine, not hardware-floor or whole-journey acceptance.

Current verification: trip saved at tick9366/head1279.786m to build two UI fixes (origin label and notices retained in compact mode), then resume the same isolated checkpoint. Latest run and pending work: JOURNEY_EXPERIENCE_CONTINUATION.md. Do not mark full packaged trip complete until terminal/result/reward recovery evidence exists.

Compatibility: contentIR-S8-MSTBM-6/service-7/ServiceV6 unchanged. Presentation is derived and cannot grant authority, change occupancy or advance station dwell. Source additions do not alter handling calibration.

Realism still requires: reviewed WAP-7/LHB recordings and mix, stock/glazing/underframe improvements, less repetitive scenery, human cab/driving review, dated operating data and a broader desktop qualification matrix. Current task excludes Android and new stock/corridors.

## Latest packaged result

Shipping02 also built/cooked successfully and includes all UI corrections. Cold recovery from Shipping01 preserved tick9366 and the complete logical fingerprint, restoring generation37 paused. All118 baseline checkpoint hashes remain unchanged. The packaged process is paused because the computer-use helper cannot activate its window (failed after refresh/retry); user asked to restore MyProject manually. Full trip/settlement and sustained performance verification remain incomplete. No invented PASS.

Latest playable archive:08_Builds/S9_JourneyExperience-Shipping-20261016-02/Windows. Start Railway Journey.cmd supplies the corridor/cab flags and uses normal local saves; isolated verification used its separate UserDir. Package manifest retains hashes including launcher and readme.
