# Clean serial combined-build regression

PASS: legacy S7, LHB and station recover paused with exact expected logical
fingerprints and service state on APK20261010-082112. The station reconstructs
4216 instances/30 components and eight LHB vehicles/14 components with no proxies.
All normal checkpoint paths, original commandline/wake and immutable/installed
APK hashes are preserved; the game is stopped at completion.

Fresh startup reports: 263 passing cases across ten groups, modification times
verified against this run's engine start. Read-only collection leaves device
preflight state unchanged. See startup-tests/summary.json.

This run was started only after the interrupted smoke's original idle baseline
had been restored. It supersedes the excluded overlapping recovery1 run.
Not fresh service-completion, artistic, audio listening or performance acceptance.
