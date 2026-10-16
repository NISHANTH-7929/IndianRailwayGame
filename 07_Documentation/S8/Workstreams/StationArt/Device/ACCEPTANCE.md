# Connected-phone acceptance queue — 8 October 2026

Exclusive device owner: S8 Chennai Egmore chat. Desktop owner:
Create reference-based railway props. Candidate contract: ../../../../PHONE_HANDOFF.md
(canonical path: 07_Documentation/S8/PHONE_HANDOFF.md).

## Completed this turn

- Wired I2304 preflight, installed APK SHA matched archived refined-cab baseline.
- 15 manual cab touches; explicit throttle/brake/direction progression reviewed.
- Six normal saves, commandline bytes and wake=2 verified restored.
- Twelve no-device capture/install safety tests pass, including foreground loss and changed
  saves. Tests do not substitute for device acceptance.

## Next APK checks

1. Receive immutable archived APK, SHA and cooked map list; reject rolling output.
2. Record six saves/commandline before non-destructive `adb install -r`; never
   uninstall, clear data or overwrite normal saves to make a test pass.
   Owned `tools/device_install.py` enforces the handoff SHA, captures checkpoint
   backups after stopping the game, and verifies unchanged state after installation.
3. Verify installed base APK hash exactly matches handed-off bytes.
4. Run `device_station_tour.py` on isolated Scene02 review, expecting 1,316
   placements and 40 modules, all six cameras, base+interior identities. Inspect
   each screenshot: correct lettering on visible faces, canopy visibility, stairs,
   clear platform path, train fit, camera clipping and missing/default materials.
5. Return concrete defects to desktop owner; do not fix canonical assets concurrently.
6. A static camera tour is only art integration evidence. Full S8 still requires
   separate playable Egmore identity/configuration, authoritative signal binding,
   motion, night/rain/audio, final manifest and appropriate regression/recovery.
7. Only on the final combined playable APK: repeated same-build workload captures,
   thermal/memory/streaming checks, and legacy/LHB/station recovery. Existing generic
   station timings cannot stand in for final Egmore performance.

Preserve failed evidence, label incomplete runs, and recheck restoration before
any further phone mutation. Phone is left stopped after restored test launch settings
so normal gameplay cannot advance unexpectedly.
