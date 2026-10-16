# Android measurement and provisional budgets

Profile contract: `Production/profile.json`. Device: iQOO Neo 9 Pro / I2304, Android
16/API36, ARM64, Vulkan, ASTC. S8 foundation profiles the current S7 fixture only.
The future representative workload is `Production/workload.json`, explicitly unexecuted.

Use Build-S8.ps1 after host validation and Automation. Record exact APK SHA256, package
bytes, engine/source hashes, device properties, build ID, graphics tier, workload,
thermal/charging/hotspot conditions and repetition index. Install without clearing saves.
Do not change phone connectivity; the inherited APK excludes INTERNET permission.
Confirm this in the final APK/device dump and verify offline operation at app level.

Capture 6000 engine frames at the existing 30 FPS cap, analyze seconds 10-170, and pause
the simulation before CSV export using the existing S7 pause workflow. Retain failed
captures separately. Use at least three paired repetitions for content comparisons,
with matching service/camera sequence and thermal conditions. Numbers define a repeatable
measurement procedure, not final asset budgets. Record menus/pauses and camera changes.

`measure_s8.py capture.csv --apk build.apk --device samples.json --output result.json`
requires finalized CSV and full analysis-window coverage. Report mean FPS, frame p50,
p95, p99, maximum; GameThread, RenderThread, RHI, GPU, simulation step and service update
separately. Lanes overlap/include waits; simulation is included in service update. Never
sum them. Missing/partial/nonfinite lanes and zero-only GPU are UNVALIDATED, not free work.
Synthetic harness tests are clearly separate from real device output.

The device sample adapter stores PSS in KiB, request-to-ready latency in ms, resident
chunk/asset counts, activation counts and Android thermal status. Preserve raw samples
with timestamps and collection source; the summary hashes its input. PSS includes driver
and allocator caching. Existing S6 telemetry reports chunk load and residency; future
asset activation counters must come from actual owners, never be invented from chunk
counts. Unsupported counters remain absent/unvalidated.

Budget approval: PROVISIONAL (null limits) -> MEASURE (raw before/after evidence) -> ACCEPT
(quality and device review) -> FREEZE (versioned approved limits). Review per-category
LOD transitions, slots/draw calls, textures/PSS, overdraw/GPU, audio residency and load
latency. No final budget is asserted by this foundation, and no representative content
performance is inferred from proxy performance.

Foundation device checklist: package/install, launch S7, load five JSON configurations,
runtime S8 report passes, current save resumes, tier records available, compatibility
negative checks pass, no INTERNET permission, collect a real finalized profile. Pending
device work must stay pending until evidence is collected.
