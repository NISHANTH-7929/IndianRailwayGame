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

Latest verification: recovered terminal completion and reward recovery verified below. See JOURNEY_EXPERIENCE_CONTINUATION.md for remaining gameplay work.

Compatibility: contentIR-S8-MSTBM-6/service-7/ServiceV6 unchanged. Presentation is derived and cannot grant authority, change occupancy or advance station dwell. Source additions do not alter handling calibration.

Realism still requires: reviewed WAP-7/LHB recordings and mix, stock/glazing/underframe improvements, less repetitive scenery, human cab/driving review, dated operating data and a broader desktop qualification matrix. Current task excludes Android and new stock/corridors.



## Verified completion — 17 October 2026

Shipping JourneyRealism-Shipping-20261017-02 completed the recovered Assisted
service at tick 54882. All 15 station chainages were observed in combined
samples; terminal dwell was 300 ticks (10 seconds), berth error -0.5955 m,
grade A / 95.5337 points. Protected traffic delay was 158.93 seconds, with zero
recorded overspeed or player lateness.

Saved reward through the UI, closed and restarted with the same isolated
UserDir. Result, one award, reputation 10 and mastery unlock survived. Settled
fingerprint aad576dcbfeebd4b matched exactly after cold recovery. Pre-settlement
fingerprint differs because the fingerprint includes profile state. All 118
baseline checkpoint files remain unchanged.

Evidence: Evidence/JourneyCompletion-20261017-01/journey-summary-reviewed.json
and terminal-result.png, settled-result.png, reloaded-result.png. Combined
samples retain both earlier overload incidents: this is recovered completion,
not an uninterrupted performance pass. Maximum sampled cells per view: 92;
maximum sampled working set: 1,615,634,432 bytes. Startup includes blocked/loading,
so all-samples-ready is false. Five-second sampling cannot exclude transient stalls.

Latest package: 08_Builds/S9_JourneyRealism-Shipping-20261017-02/Windows.
Both Assisted and Standard remain selectable. Player turnout movements,
additional protected traffic encounters and authentic stock recordings remain
unfinished; see GAMEPLAY_GAPS.md. No full-realism acceptance is claimed.
