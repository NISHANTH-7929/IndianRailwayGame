# Android Egmore assisted itinerary — PASS

APK20261009-121852, SHA256
5126ed14ce2d04cd9349e923aa1e099ee4cd0efab2ac6513b331199f7fd6be84.

Observed all7 phases: Approaching, Arrival, Berthing, Dwelling, ReadyToDepart,
Departed, Completed. Actual Depart screen tap at(1228,1135), service tick4079;
subsequent telemetry confirms departure authorized and Departed. Completed tick9291.
Full300-tick/10-second dwell, berth error−0.189334m, zero overspeed/harsh ticks,
all9 objectives met, gradeA /98.579994. Result screenshot visually inspected.
Reward intentionally not settled; no normal profile advancement claimed.

`itinerary.json` is the bounded service-completion receipt. Generic wrapper
`result.json` retains conservative gameplayValidated/serviceCompletionValidated
false fields: it validates restoration and smoke, not the full S8 release.
All restoration checks pass:8normal save paths unchanged, original commandline,
telemetry and wake0 restored, exact installed/archive hashes unchanged, app stopped.

This proves this isolated assisted itinerary only. Egmore cold recovery/reward
settlement, Standard driving, interior/cab visual acceptance, night/rain/audio,
combined performance and release catalog remain separately open.
