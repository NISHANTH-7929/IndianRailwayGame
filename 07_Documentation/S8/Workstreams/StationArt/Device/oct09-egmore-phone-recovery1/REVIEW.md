# Egmore Android cold recovery — PASS

APK 20261009-144106, SHA256
92c96bdd077445973a7d05fcfefb5a82444f8924a11a8b52dd2fdba89fd2f5f9.
Recovered the pre-existing isolated checkpoint with exact fingerprint
87d49fe0fb69b87e, generation and full service state. Two matching paused samples
were required after current-run log proof. Scene02, two ID-bound signals and both
locomotive presentations rebuilt successfully. The captured screen shows the
paused service UI; an unrelated Android clipboard toast obscures part of it, so
this capture is not clean visual acceptance evidence.

All ten checkpoint paths (eight normal, two isolated) remained unchanged.
Original launch override, telemetry and wake setting were restored, app stopped,
and APK/archive hashes verified. No restoration errors. This verifies cold
recovery, not explicit resume progression, reward settlement or final S8.
