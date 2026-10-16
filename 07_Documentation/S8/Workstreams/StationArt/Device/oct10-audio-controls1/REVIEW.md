# Partial audio interaction evidence

Steps0..3 capture running, horn playback, paused silence, then resumed rail/station
playback. Engine log records the horn trigger and horn=1; pause reports all four
playback flags0 and gains0; resume reports running/rail/station1.

The action-file deadline expired after step3, so the complete runner did not pass.
All restoration/preservation checks passed: app stopped, original override,
telemetry/wake restored, normal saves and APK unchanged. action-3.json was written
after the runner had exited and was NOT executed. Do not count brake acceptance.
Repeat as controls2. No audible-quality or complete lifecycle claim.
