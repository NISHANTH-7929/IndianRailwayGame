# Refined cab manual touchscreen regression — 8 October 2026

Exact installed/artifact SHA256:
`2b75ffb51eeeaa3ec163ce37b34db3c7852b5b7da1cb9b21cb6a13c02f61d33d`.

15/15 physical-screen input injections registered one interaction each, with
advancing, valid, nonfaulted simulation telemetry. Four throttle taps changed
requested power 0.35 -> 0.60 -> 0.85 -> 1.00 -> 0.00. Brake taps changed 0.00 ->
0.25 -> 0.50 -> 0.75 -> 1.00. Three reverser taps cycled 1 -> -1 -> 0 -> 1.
Emergency was tapped twice; wipers and cab light once. Their interaction counts
passed; switch/electrical semantics are not independently asserted by telemetry.
The native engine log independently records all 15 intended control roles in order
with `accepted=1`; no rejected command is present in that interaction sequence.

Initial and final screenshots inspected: dashboard remains visible, wiper position
changes, and camera look permits right-hand control access. Fine labels remain
hard to read. Exterior debug geometry is the isolated legacy test world, not the
new Egmore station. This is not final art acceptance or a full-S8 gameplay test.

Verified restoration: all six save hashes unchanged, exact commandline bytes
restored, wake returned to 2, APK artifact unchanged. Test leaves the game stopped
after restoration to avoid advancing normal gameplay. No app data was cleared.

Evidence: `touch-results.json`, per-input screenshots, `preflight.json`, `engine.log`,
and `result.json`. The runner's six no-device safety tests pass.
