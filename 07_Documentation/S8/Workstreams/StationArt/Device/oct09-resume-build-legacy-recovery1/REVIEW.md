# Existing-service compatibility on resume-enabled APK

All three paused cold recoveries PASS on APK 20261009-144106:
legacy fingerprint 0efccd7c539ec58a, LHB ff7e0afb56b48956, and station
2c55c3509c5a4284. Full service state and content identity match prior references;
the station reconstructs 4,216 instances / 30 components with no proxy instances.

All eight normal checkpoint paths, original commandline and wake setting remained
unchanged; app stopped and both installed/archive hashes verified. Fresh startup
reports pass 263 cases across ten groups (startup-tests/summary.json).
This is compatibility evidence, not final S8 or interior visual acceptance.
