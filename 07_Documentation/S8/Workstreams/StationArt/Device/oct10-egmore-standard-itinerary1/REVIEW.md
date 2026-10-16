# Excluded: intentionally stopped to correct test driver

This test used Standard mode and actual UI controls. Source review during approach
found its test-driver dwell predicate incorrectly included Berthing, which begins
six metres before the marker. This could stop the train short. The app was sent
Home to trigger normal suspend/save, causing the runner to reject paused state
and execute cleanup. The test-driver predicate was corrected to hold only Dwelling
(and unauthorized ReadyToDepart); gameplay/source/APK were unchanged.

No completion acceptance. All eight normal checkpoint paths stayed unchanged;
original launch override/telemetry/wake restored; game stopped; APK hashes match.
The resulting isolated background checkpoint is retained for resumed test2.
