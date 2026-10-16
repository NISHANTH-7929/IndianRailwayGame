# S7 implementation report

**S7 FAIL — Acceptance 34/36.** Implementation and technical verification are complete: 238 host cases,
238 final Android cases and six Unreal Automation suites pass. Gates 7 (real-player comprehension of the restriction)
and 33 (six-person usability) remain pending; no participant feedback is fabricated.
The stage is not accepted and S8 has not begun.

## 1. S7 objective
Prove one selectable, playable, repeatable railway service through driving, causal
AI interaction, station work, result explanation and durable progression.

## 2. S6 baseline
The source/configuration baseline was copied before implementation to
`Evidence/Baseline`. Its verification includes matching S6 source manifest entries
and the archived Android APK. S6 APK SHA256:
`BF5173EBD75E270C42BFDF6FF043D30548F385D33BB4A67A55B815E43C319619`.
The completed S6 report records 30/30 gates. Its evidence is retained separately.

## 3. Architecture changes
`Simulation/RailService` owns application state and consumes the existing railway
world. Unreal owns widgets, platform presentation, sound and filesystem adapters.
The existing snapshot archive was extracted into a shared header; S7 reuses the
S6 alternating-slot, flush/readback transaction through a generic capture callback.
No second physics, interlocking, AI driver or save transaction was introduced.

## 4. Service architecture
`DA_Shantipur` authors one service: player Train 2 (43 m, two vehicles), AI Train 1
(181 m, eight vehicles), existing routes R2 and R1. Selection leads to briefing,
formation confirmation, start, running and result. Assisted and Standard share
S3 dynamics and S4 authority. Expert is reserved and rejected by current selection.
All simulation and objective evaluation use fixed 30 Hz steps.

## 5. Station contract
The lower E2 platform occupies 95–145 m. Berth head is 140 m with 2 m tolerance;
the whole formation must fit and speed must be at most 0.05 m/s. Station phases
are Approaching, Arrival, Berthing, Dwelling, ReadyToDepart, Departed and Completed.
Dwell requires 300 continuous stopped ticks. Explicit Depart requires route
permission. The service ends stopped within tolerance of the 152 m finish marker.

## 6. Timetable/dispatch scope
One authored priority interaction uses the existing fixed fixture. Player route
requests retry every 30 ticks through normal interlocking. No general dispatcher,
network timetable or production corridor system has been added. Arrival/departure
windows and expected duration are authored data.

## 7. AI traffic integration
The existing S5 driver operates Train 1 on R1. Its real route ownership conflicts
with player R2. The player receives the existing restrictive signal/authority
until the express rear clears and S4 releases the resources. There is no timed
signal animation, teleport or visual trigger granting movement permission.

## 8. Objective system
Nine typed objectives evaluate start departure, authority, speed, station, stop
accuracy, dwell, departure permission, itinerary and traffic clearance. State is
persisted with the railway. Visual actors cannot complete objectives.

## 9. Failure/retry
Station overshoot or departure without permission fails the service. Reverse
movement violates this forward-only itinerary; this application failure leaves
the railway invariant-valid. Failed runs earn no reward and can retry with a new
run ID. Existing reputation survives retry. Frame overload pauses without
advancing the railway and offers explicit restoration of the last checkpoint.

## 10. Scoring/result
Weights total 100: completion 50, safety 20, berth accuracy 15, speed 10, punctuality
5. The result shows grade, stop error, traffic delay, player lateness and overspeed
duration. Grade A starts at 90, B at 75. A failed run has zero score and Retry grade.

## 11. Progression/unlock
One successful settlement grants 10 reputation and a Shantipur mastery badge.
The settlement ID includes service ID and monotonic run number. Candidate profile
state becomes visible only after a verified save; repeated settlement does not
write or award again. Replay requires successful results to settle first.

## 12. Save/recovery integration
The S7 application envelope includes exact service configuration identity, full
S6 world snapshot, objectives, station, control intent, result and profile.
S7 uses separate `Saved/S7` slots to preserve legacy `Saved/S6` checkpoints while
sharing the transaction implementation. Invalid configuration/version records
are not overwritten. Host tests cover cold continuation, interrupted settlement,
fallback, repeated settlement and paid-result cold restore.

## 13. Offline behaviour
The packaged APK has no INTERNET permission (`Evidence/apk-permissions.txt`).
The complete phone loop, save/recovery, settlement and replay ran with this
restriction. The phone hotspot remained on; airplane mode was not required.
There is no account, online entitlement or network-dependent objective.

## 14. UI/tutorial
Selection, formation confirmation, mode choice, pause, result, retry and replay
use touch widgets. Throttle/brake presets, emergency/reset, horn and look-back
send ordinary commands. HUD labels explain speed, authority, traffic, stop and
dwell. Final corrections remove clipped button labels and menu debug overlap,
and show “no valid station stop recorded” after a missed berth. Results include
the domain-calculated score components. Screenshots are retained under Evidence/Device.
Unaided comprehension still needs real-player observations.

## 15. Audio
Three deterministic synthetic WAVs provide a horn, looping rolling/joint sound
and station cue. The generation script is retained; no external audio is claimed.
These are provisional functional cues. In-device audibility remains unverified.

## 16. Visual asset strategy
Reuse S2/S6 proxy rolling stock, track and chunks. Add only a primitive platform,
canopy/posts, cab distinction and stop labels. There are no detailed interiors,
photorealistic assets or production-scale route assets.

## 17. Android verification
Target: iQOO Neo9 Pro / I2304, Android 16, ARM64 Vulkan/ASTC Development build.
Device evidence includes real AI restriction/release, two successful Assisted
loops, a Standard overshoot failure, retry/profile preservation, complete-state
pause/warm/cold recovery and pending/paid result recovery. The fresh recorded
run finished at grade A, score 98.534885, berth error -0.195349 m, 300 dwell ticks,
all nine objectives, zero player lateness and 175.833333 s traffic hold.
Host tests also complete a successful Standard loop through ordinary commands.
`required-tests.json` maps all 42 requested tests. A truncated-write settlement
is fault-injected in the shared transaction host test; phone tests verify its
pending and paid recovery endpoints, not a precisely timed physical write tear.
Final APK identity is in Evidence/apk-hash-release-candidate.json. Installation
and final cold launch are verified. All nine regenerated Android reports parse
strictly and pass 238 cases (Evidence/android-tests-summary.json). The corrected
report writer also passes strict parsing across all nine host reports. Final
selection screenshot and telemetry preserve reputation 20, two awards and the
unlocked badge. The temporary profiling override is absent. Evidence is under
Device/final-selection and final-profile-override.txt.

## 18. Usability findings
Pending. No six-person observations have been received. The anonymous CSV is
an empty template, not evidence. USABILITY_PROTOCOL.md requires three casual
players and three enthusiasts, excluding the developer; it records hints,
speed/traffic/station/result understanding, resume and voluntary replay. Audio
audibility/comprehension must be recorded by those listeners. Automated ADB
runs and synthetic-sound generation do not establish these human findings.

## 19. Performance results
A finalized 12,000-frame capture contains 407.75 s. The selected 100–320 s
window is within the fresh running service (after initial menus and before its
result). It averages approximately 29.965 FPS at the 30 FPS cap, with five frames
over 50 ms and none over 100 ms. See the CSV summary for exact CPU/GPU and service
update distributions. Device PSS samples were approximately 446.47–452.93 MiB;
Android thermal status was consistently 2 while the hotspot remained active.
These are engine frame intervals, not compositor presentation times or proof of
production-content performance. The capture APK is recorded in apk-hash-delivery.json;
the later final APK changes only paused-menu text/label presentation in
RailServiceApplication.cpp, plus JSON escaping in the test-report writer. Active driving, simulation, content and rendering
workload are unchanged. This provenance is explicit rather than attributing the
capture to a different binary.

| Counter (ms) | Mean | p95 | p99 |
|---|---:|---:|---:|
| FrameTime | 33.372 | 34.618 | 35.449 |
| GameThreadTime | 5.788 | 7.555 | 8.990 |
| RenderThreadTime | 4.633 | 5.950 | 6.635 |
| RHIThreadTime | 3.166 | 4.185 | 4.926 |
| GPUTime | 1.866 | 2.819 | 3.741 |
| RailS7/ServiceUpdateMs | 0.414 | 0.579 | 0.703 |

CPU lanes overlap and may include waits; do not sum them.

## 20. Problems encountered
The resumed source lacked its widget. Initial reverse-breach testing showed
S4 correctly prevented the breach. Overload debt had no player recovery action.
Phone inspection exposed auto-wrapped/clipped button text, menu/debug overlap,
a technical fresh-save diagnostic in normal UI and a misleading zero berth error
after an invalid stop. Foreground guards stopped touches when Android Settings
was open. The evidence audit also found unescaped JSON in embedded test details; the shared writer was corrected and all reports strictly parsed. Sparse phone monitoring missed the brief Arrival state; fixed-step host
tests verify all seven transitions. Build caches required access outside the workspace.

## 21. Fixes
Implemented the missing widget and Automation wrapper; preserved S4 protection
and added a forward-only service rule. Added explicit checkpoint restoration
after clock overload, mode and score-weight validation, Standard completion
coverage, emergency reset and background movement-audio stop. UI fixes use
unwrapped button labels, a menu background, hidden debug labels and clear invalid
berth wording. Scoring components come from the same domain calculation as the grade.

## 22. Known limitations
One short fixture-based service, one station, provisional train/platform meshes
and three synthetic cues; no claim of production visuals, broad hardware support
or future-route scale. Expert remains a reserved mode boundary. Full usability
and subjective audio evaluation are outstanding. Development startup executes
regression tests. Sparse device samples do not replace fixed-step transition tests.
The save compatibility key includes the exact authored service configuration;
future content migrations are not implemented.

## 23. Acceptance checklist
46/46 S7 host cases, 192/192 earlier regression cases, and 6/6 Unreal
Automation suites pass. These counts are separate from the 36 acceptance gates.
All 42 named test mappings and evidence references are in required-tests.json.
Human understanding is not inferred from visible labels or automated success.

| Gate | Requirement | Status |
|---|---|---|
| 1 | A player can select a service. | PASS |
| 2 | A player can start a valid service. | PASS |
| 3 | A complete service itinerary exists. | PASS |
| 4 | The player can drive the train. | PASS |
| 5 | AI traffic genuinely interacts with the player. | PASS |
| 6 | The interaction uses S4/S5 railway truth. | PASS |
| 7 | The player sees/understands the resulting restriction. | PENDING |
| 8 | The player can approach a functional station. | PASS |
| 9 | Platform/formation compatibility is validated. | PASS |
| 10 | The train can berth correctly. | PASS |
| 11 | Dwell is logically completed. | PASS |
| 12 | Departure is logically completed. | PASS |
| 13 | Objectives evaluate from authoritative state. | PASS |
| 14 | Safety failures are consequential. | PASS |
| 15 | Retry works. | PASS |
| 16 | Result/grade is produced. | PASS |
| 17 | Result explains important outcomes. | PASS |
| 18 | Traffic-caused delay is not treated as player-caused delay. | PASS |
| 19 | One reputation/unlock transaction works. | PASS |
| 20 | Progression settlement is duplicate-safe. | PASS |
| 21 | Pause works. | PASS |
| 22 | Background/foreground works. | PASS |
| 23 | Cold resume works. | PASS |
| 24 | Offline complete-loop operation works. | PASS |
| 25 | Replay/continue works. | PASS |
| 26 | S2 regression passes. | PASS |
| 27 | S3 regression passes. | PASS |
| 28 | S4 regression passes. | PASS |
| 29 | S5 regression passes. | PASS |
| 30 | S6 regression passes. | PASS |
| 31 | Unreal Automation passes. | PASS |
| 32 | Android full-loop verification passes. | PASS |
| 33 | Usability check is completed. | PENDING |
| 34 | No production-scale visual content was unnecessarily developed. | PASS |
| 35 | No S8/S9 systems were implemented prematurely. | PASS |
| 36 | The complete vertical slice is repeatable. | PASS |

**S7 FAIL — Acceptance 34/36**, pending real-player gates 7 and 33.

## 24. S8/S9 exclusion
No S8/S9 content systems, production asset pipeline, full two-corridor/six-station
MVP, weather, day/night, crowds, multiplayer or monetization were implemented.

## 25. Next dependency
Exact next dependency: S8 — Representative slice: production method and cost proof

S7 must first complete its remaining verification and acceptance work.

