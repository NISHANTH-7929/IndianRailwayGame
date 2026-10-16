# Egmore snapshot identity checks — 9 October 2026

Standalone current-source C++ probe:34 checks, zero failures.

- All six off-diagonal legacy/LHB/Egmore record→definition combinations rejected,
  retaining both destination world and destination service.
- Valid changed Egmore ID/version/dwell configuration rejected transactionally.
- Mixed Egmore/legacy and Egmore/LHB slots, both orders: recover compatible slot;
  subsequent Save refuses overwrite with zero writer calls and unchanged bytes.
- Standard-mode half-step plus queued throttle/brake captures/restores byte-exact;
  first continuation advances exactly one tick;120 half-steps remain byte-identical.

No engine/source mutation or phone execution by this probe. These host checks
do not substitute for Android cold recovery or full release acceptance.
