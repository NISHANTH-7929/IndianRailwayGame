# Journey realism update â€” 16 October 2026

Implemented in source and cooked candidate:

- More varied street spacing, district height/density, open plots and palettes.
- One distant scenery cell prefetched per frame; essential near-train cells remain mandatory.
- Eleven derivative cab/glass/coach materials preserving original meshes and baked detail.
- Closer corridor exterior camera, plus cleared recovery notice on resume.

Validation: 62/62 journey host tests; editor and Shipping compilation passed.
Asset import verified eleven derivatives, with original S8 parents untouched.
All118 baseline checkpoint files unchanged.

Packaged journey on the prior Shipping02 candidate crossed the junction and
progressed through intermediate stations, but suffered frame overloads at
35852 and38590 ticks. Both failure records and saves retained. Recovery via UI
succeeded. Saved exit at tick38466/generation139/fingerprint8a8ab5689ca82a20.
Terminal dwell and reward were subsequently verified below.

Moving300s PresentMon sample: p95 9.4485ms, p99 10.6564ms, max191.3973ms,
47 intervals over100ms. These are application present intervals on this PC.
Later comparison baseline contains overload and a window-size change: it is
not a clean matched performance baseline. No smooth-full-route claim.

Authentic WAP-7/LHB recordings remain a sourcing gap. Existing approved generic
recordings and explicitly synthesized layers retained. No paid or unlicensed
recordings imported. This remains a representative corridor and stock finish,
not a surveyed scene, exact timetable, or certified driving simulator.

Candidate continuation (Shipping20261016-01): no further frame-overload pause
observed between restored tick38466 and47305. User controls later left the train
stopped near23212.52m, reverse selected, brake25%. This is not terminal success.
120s candidate PresentMon: p956.5915ms,p997.3572ms,max152.8356ms,20>100ms,
25dropped events. Route/window differed from baseline; no causal improvement or
whole-route smoothness claim. Cab material visual review completed; exact art
and audio acceptance remains open.


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
