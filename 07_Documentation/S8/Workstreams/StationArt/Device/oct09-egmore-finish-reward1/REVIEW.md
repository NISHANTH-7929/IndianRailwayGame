# Recovered completed service: reward settlement PASS

On APK 20261009-144106, restored the existing isolated completed service at
generation 97 with exact service/fingerprint match and two paused ready samples.
No fresh journey was started. The preceding disconnected test is still excluded
as an uninterrupted itinerary, but its preserved completed checkpoint was valid.

One UI touch on SAVE REPUTATION + UNLOCK produced generation 98, fingerprint
dcdde9a4ed3db7b7: settled=true, awards=1, reputation=10, lastSettledRun=1,
nextRun=2. The screenshot shows Grade A / 98.6, reward saved and mastery unlocked.
The authoritative settlement.json passes every reward check. The generic wrapper
keeps gameplayValidated/serviceCompletionValidated=false by design; this run
specifically validates recovery and settlement, not a newly observed itinerary.

All eight normal checkpoint paths unchanged; original commandline/telemetry and
wake0 restored, app stopped, installed/archive hashes unchanged. Separate
oct09-egmore-settled-recovery1 verifies persistence after process restart, including
exact profile/service/generation/fingerprint and all ten checkpoint paths unchanged.
No extra award occurred on recovery. This does not test repeated settlement input
or claim final S8 acceptance.
