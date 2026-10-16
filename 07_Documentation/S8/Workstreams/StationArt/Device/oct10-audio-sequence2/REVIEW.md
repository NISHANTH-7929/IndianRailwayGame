# No ready foreground service: excluded

90-second initial observation did not obtain a running service. Current-run log
records Android SuspendApp, paused tick0, presentation briefly ready, then world
exit with loadedLevels0. No sequence touches occurred. Do not classify this as a
proven audio failure or assume why Android suspended the app.

All restoration/preservation checks pass: game stopped, original commandline,
AudioReview telemetry and wake restored, normal checkpoint paths unchanged,
installed and archived APK hashes unchanged. No active phone test remains.
Needs phone kept unlocked and game foreground for the next bounded attempt.
67 local device-tool tests pass, including sequence guards. No phone pass implied.
