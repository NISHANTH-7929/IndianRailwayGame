# Scene02 APK compatibility recovery — 9 October 2026

APK SHA256 `2b52b9f0b321173106e1dfe39db410cf3f2451b9d5debee62ef7e0da676018b7`.
All three existing saved services cold-loaded paused without resuming simulation:

- Legacy S7: exact identity, fingerprint `0efccd7c539ec58a` and service matched.
- LHB: exact identity, fingerprint `ff7e0afb56b48956` and service matched.
- Generic station: fingerprint `2c55c3509c5a4284`; 4216 station instances/30
  components, eight LHB vehicles/14 components and zero proxies matched.

All cases valid, nonfaulted, unblocked after presentation streaming, and recreated
the expected vehicles/loaded levels. Fresh telemetry and unique-run successful
world-load proof exclude earlier-run data. Screenshots and engine logs retained.

Six checkpoint hashes, original launch override, original service telemetry,
wake setting0 and installed/artifact SHA were preserved. Backups retained.
App left stopped, no save writes or data clearing performed by the runner.

Earlier attempt `oct09-scene02-cold-recovery1` is excluded: it incorrectly treated
the initial presentation-streaming state as final. Logical saved state already
matched; the runner now waits up to90seconds for presentation only while requiring
identity/fingerprint/service/paused/valid throughout. Mock tests cover both readiness
success and persistent-block timeout. Total device-tool test suite:27 passing tests.

This verifies backward compatibility of the static-review APK, NOT playable Egmore,
new service recovery on Android, final visuals or performance.
