# S8 two-chat work allocation — 2026-10-08

## Current ownership update (supersedes phone restrictions below)

Station chat now owns connected-phone/ADB installation and device validation.
Integration chat owns desktop Unreal, canonical imports/source, Automation and APK
builds. Neither chat operates the other's execution surface. Exchange immutable APK
paths, hashes, validation and process status through PHONE_HANDOFF.md. Do not redo
passed cab baseline tests. Prioritize platform/interior and train-camera results;
hold exterior polish. Source-art staging remains owned by the station chat.
V22 and the subsequent cab candidate have passed phone checks; older statements
about V21 below are historical. Scene02 has not yet passed phone validation.

User decision: active rolling stock is WAP-7 and LHB AC 3-tier only. WAG-9 is
deferred and is not an S8 completion gate. Preserve its existing files and evidence.
BLC remains deferred. A second reusable scenario can use WAP-7/LHB again; it does
not require another locomotive class.

## Integration chat (original chat)

Owns canonical Unreal project, rolling-stock source/cab assets, runtime configuration,
imports, shared scripts and release documentation, packaging, phone/ADB, save and
regression checks, weather/audio runtime integration and final workload measurements.
Only this chat runs Unreal Editor, UAT, Automation, installs APKs or operates the phone.
Do not claim the V22 package has passed phone validation: V21 remains the last
fully verified candidate. The rolling APK may already contain V22.

## Station art chat (new chat)

Owns only these new staging paths:
- 04_Assets/S8/Workstreams/StationArt/
- 07_Documentation/S8/Workstreams/StationArt/

Deliver Chennai Egmore architecture/layout, platforms/canopies/footbridge, original
materials, infrastructure presentation meshes and bounded adjoining urban/vegetation
context. Prioritize a coherent realistic scene from player cameras over tiny isolated
revisions. Reuse existing source assets as read-only inputs. Copy generators into
the owned staging area and explicitly redirect all outputs before running them.
Never modify canonical assets, scripts, runtime JSON, global Python packages,
Unreal content, existing evidence, other chat reports, builds or device state.
Use Blender/source validation and local renders; no Unreal or Android processes.
References may be added only inside staging. Preserve attribution; no stock-photo
watermark removal or use of preview photos as production textures.

Write STATUS.md and HANDOFF.md within the owned documentation directory. Deliver
versioned source meshes, FBX/GLB, textures, metric bounds, LOD counts, stable-ID
mapping, provenance, preview renders, validation results and a machine-readable
placement/asset manifest. Clearly mark estimates and remaining issues. Staged asset
versions become immutable when marked ready; continue later revisions separately.

## Integration boundary and remaining gates

Original chat reads completed staged handoffs, checks hashes and copies accepted
versions into canonical content. Art chat does not import them itself. No cross-chat
messages are necessary: use the shared status/handoff files. Both chats can continue
independently until a handoff is ready. Two chats improve concurrency, not guarantee
half the elapsed time; builds, device checks and final scene acceptance are serial.

Preserve S2-S7 fixtures, content IDs, saves, algorithms and all historical evidence.
Complete realistic WAP-7/LHB and Egmore presentation, a separate playable Egmore
configuration, infrastructure/context, representative day/night/rain/audio, production
catalog and final combined Android/recovery/regression evidence. No WAG-9/BLC gates.
The status audit dated 2026-10-07 remains historical evidence; this file supersedes
its optional WAG-9/second-locomotive scope.
