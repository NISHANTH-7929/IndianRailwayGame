# WAP-7 and WAG-9 production candidates

User selected WAP-7 passenger and WAG-9 freight electric locomotives, with exterior,
moving parts, driver cab, instruments, existing driving-command bindings and Android
cost measurements. Parallel model work is authorized. Target is convincing class
identity from exterior and driver viewpoints, with moderate game geometry.

## State on resuming 2026-10-01

Both Blender source scenes, generators, FBX exports and first renders exist under
`04_Assets/S8/Locomotives`. First-pass visual review identified excessively boxy
fronts, simplified bogies and generic cab panels. Both candidates are being revised
against class references. First-pass renders are not accepted representative quality.

Runtime presentation code was partially written before interruption. Import,
compilation, packaged controls and Android performance remain unverified until
their evidence is recorded. Existing calibrated handling has not been replaced by
class-specific WAG-9 physics. Full S8 acceptance requires more than these two assets.

The connected iQOO I2304 S7 save was backed up under
`Evidence/Locomotives/pre-integration/S7`. Its saved logical fingerprint is
`0c71336bb96fdebf`, with a paused running service. Asset profiling must preserve it.

Historical S8 foundation evidence remains evidence for that original build, not
for these new assets. New checks and captures go under `Evidence/Locomotives`.

## User-selected replacement sources, 2026-10-01

Use only Acquired/WAP7_Selected and Acquired/WAG9_Selected for continued work.
Both user ZIPs copied with SHA-256 receipts, extracted and rendered in Blender.
Full-resolution original geometry/UVs/material textures preserved. WAP7 has
388,854 triangles; WAG9 has 289,524. Prepared sources partition bogies; wheelsets
and pantographs are not yet rigged. Furnished cabs are not established.

User requested old locomotive items deleted. The recursive deletion command
was rejected by automatic approval review; superseded folders still exist.
Do not use old generators or import_acquired_wap7.py. Use
prepare_selected_locomotive.py and import_selected_locomotives.py.
Source listing links/credits requested; do not copy the prior WAP7 license
onto these different assets without verification.

Selected Unreal import saved all six meshes with three LODs each and 13 texture
imports per family. Full editor process crashed during shutdown after its completed
import marker; saved assets are undergoing independent reload/review. Editor C++
build passed after correcting which player train proxy is hidden. No Android
device connected during this run, so no new on-device measurement is claimed.

Independent editor reload verified all six selected mesh assets and three LODs
per mesh. Review assembly bounds are approximately 21.33 m long and 3.11 m wide,
matching Blender source dimensions. Latest C++ build also passes with cab entry
disabled for these currently exterior-only assemblies.

Both exact new Sketchfab listing IDs were verified through the official API:
WAP7 6d533d73faf34a8a811fee1fcb9c8878; WAG9 665150d63c6146e0b361c1020a87074e.
Both credit knitro_vedant under CC BY 4.0; credits saved beside sources and in staged
Production/locomotives. Correctly lit Unreal review PNG confirms source liveries
and detailed exterior appearance. Android review build is separate from foundation
at 08_Builds/S8_SelectedLocomotives.

Android selected build succeeded (246,901,590-byte APK) and installed with adb -r
on USB-connected I2304. Two attempted WAP7 captures were excluded: Android
keyguard/NotificationShade had focus. Both cleanup reports confirm commandline
restored and normal checkpoints unchanged. User asked to unlock phone.
No valid Android frame-time or memory result exists yet.

## Completed initial device captures

Phone reconnected/unlocked. One finalized 6,000-frame capture each for WAP7, WAG9 and baseline completed. See Device/SELECTED_EXTERIOR_REPORT.md and per-run source CSV/telemetry. Normal saves unchanged. Exterior-only scope; dark test lighting, missing cabs and moving equipment remain unfinished. Baseline camera differs, so results are not isolated per-asset cost deltas.

## 3 October 2026 — WAP-7 cab Android verification

Reconstructed cab has 45,565 source triangles, 12 cab groups, and 15 runtime components including exterior. Corrected contact atlas UV corruption and Unreal Python sun rotation. Physical touch checks passed all 15 actions; final daylight run captured 6,000 frames at 29.95 FPS mean, 34.444 ms p95 frame time, 638.1 MiB peak sampled PSS. Low-light retry verified advancing gameplay, lamp response and exterior/cab transitions. Normal saves and launch arguments restored. First low-light attempt had stale telemetry and is excluded; intermittent startup failure remains undiagnosed.

Current APK SHA-256: 40730b9d25b0c7a2eb9c8c8b48e21ea1c2f3f6713de573bc4f7e285ae7d48f92.
Detailed assessment: Evidence/Cab/FINAL_DEVICE_REVIEW.md. Photographic realism is NOT accepted: panel material response, fine instrument labels, analog night illumination, exact dimensions and fleet-specific equipment identification remain unresolved. WAG-9 cab is not duplicated from WAP-7. Shared-project LHB changes preserved.
