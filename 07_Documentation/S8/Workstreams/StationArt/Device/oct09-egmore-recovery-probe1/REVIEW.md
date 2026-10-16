# Egmore service recovery coverage — 9 October 2026

The failing canonical Automation assertion was missing AI-release coverage, not
a demonstrated snapshot mismatch. The player completed at tick 8948 while the
adjacent AI route was still legitimately active (head 1025.07 m). A standalone
probe confirmed exact service/world JSON recovery at Arrival, Berthing, Dwelling,
ReadyToDepart, Departed and Completed.

The recovery scenario now holds at ReadyToDepart until the AI route has been
observed active and then released. It must not count the initial unreserved route
as a release. The AI releases at tick 9637; the player then departs and completes
at tick 14613. Recovery matches at the release and all station phases.

Only the test scenario timing changed; runtime gameplay and save formats did not.
Editor build succeeds. `UnrealAutomation/index.json` records all 13 suites passing.
Standalone probe source is in the StationArt tools folder. Its compile/result
files are development diagnostics, not phone or performance evidence.

Playable APK packaging/phone validation, signal mesh/aspect binding, persistent
scene readiness, night/rain/audio and full S8 release remain open.

Follow-up presentation fixes: `RailServiceApplication.cpp` resolves S2's actual
boundary through the train's committed path history instead of the old hardcoded
130 m threshold. `RailServiceView.cpp` suppresses proxy platforms and markers
based on Egmore content identity even without the optional S8Station flag.
Independent read-only review found no defects for supported paths. Editor build
passes; separate post-edit Automation evidence in UnrealAutomationPresentation
records13 succeeded, zero warnings/failures/not-run. No editor/build remains active.
These are desktop source changes, not changes to the installed diagnostic APK.
