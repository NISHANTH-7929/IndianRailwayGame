"""Render current evidence status without manufacturing pending acceptance."""
import json
from pathlib import Path
from validate_s8 import DOC,load
E=DOC/'Evidence'
acceptance=load(E/'acceptance.json')
artifact=load(E/'artifact.json')
android=load(E/'android-verification.json') if (E/'android-verification.json').exists() else {'passed':False,'summary':'Device verification pending.'}
status='S8 FOUNDATION COMPLETE' if acceptance['passed']==29 else 'S8 FOUNDATION IMPLEMENTED — VERIFICATION PENDING'
measurement='No accepted device capture yet.'
if android['passed']:
    metrics=android['details']['metrics'];frame=metrics['FrameTime']
    measurement=(f'Paused-fixture analysis (10-170 s): {metrics["FPS"]["value"]:.3f} FPS; '
        f'frame p50/p95/p99 {frame["p50"]:.3f}/{frame["p95"]:.3f}/{frame["p99"]:.3f} ms; '
        f'maximum {frame["maximum"]:.3f} ms. Mean GameThread/RenderThread/RHI/GPU '
        f'{metrics["GameThreadTime"]["mean"]:.3f}/{metrics["RenderThreadTime"]["mean"]:.3f}/'
        f'{metrics["RHIThreadTime"]["mean"]:.3f}/{metrics["GPUTime"]["mean"]:.3f} ms. '
        f'PSS maximum {metrics["PSS"]["maximum"]:.0f} KiB; thermal status 1 in all 15 samples; '
        'two resident chunks. Simulation/service timings and asset activation counts are '
        'UNVALIDATED in this paused run. The external floating overlay was present. '
        'These numbers verify reporting, not production budgets or an active-service benchmark.')
report=f'''# S8 foundation report

**{status}**  
**Acceptance {acceptance['passed']}/29**  
Evidence finalized 2026-09-30. Foundation scope only; full S8 is not accepted.

## 1. Purpose
Prepare the production method and cost-proof infrastructure without producing the
representative art. Production assets intentionally not created. No content-generation
credits, marketplace acquisition, production audio, weather/night art or S9 systems.

## 2. S7 baseline status
S2-S6 reports were read before edits; S7 reports 34/36 gates, with gate 7 (real-player
restriction comprehension) and gate 33 (six-person usability) pending. This task does
not complete them. Baseline hashes match the S7 final source-change manifest; see
Evidence/s7-baseline-verification.json. Existing S7 save state is preserved on device.
The historical S7 report is not rewritten to imply S8 acceptance.

## 3. Foundation architecture
Production JSON lives under Content/IndianRailway/Production. A new read-only
URailProductionSubsystem loads five UFS-packaged JSON documents and writes a runtime
validation report to Saved/S8. It exposes no simulation mutation API. JSON is an added
module dependency, and DefaultGame.ini stages the catalog. Existing simulation source,
maps, data assets and sounds are byte-identical (Evidence/architecture-audit.json).
Host schema validation runs before Build-S8 packaging; runtime checks verify packaged
identity, S6 chunk bindings, configurations and the actual S7 save codec.

## 4. Content standard
CONTENT_STANDARDS.md specifies common identity, unit/orientation, pivot, collision,
material/texture/LOD, streaming, mobile and provenance requirements plus a fourteen-
category matrix: locomotive, coach, wagon, formation, station, trackside, environment,
building, vegetation, character, animation, audio, VFX and UI. Conceptual category
folders are not populated before content arrives.

## 5. Provenance system
Schemas/provenance.schema.json defines source/archive/date/creator/license/rights,
restrictions, attribution, modified reference, verification status/evidence and notes.
Unknown, pending review, restricted and rejected cannot release. Approval requires
verified commercial/modification rights and source evidence. Templates are pending
records, not acquisitions. PROVENANCE_STANDARD.md explains human verification limits.

## 6. Asset ID system
Stable case-sensitive Category.Family.Variant IDs survive path and visual replacement.
Simulation DefinitionId, VehicleId and TrainId are independent. Duplicate/colliding IDs
and unresolved references fail validation. No path is used as logical simulation identity.

## 7. Content manifest
manifest.json is Content.Foundation.01 version 0.1.0, schema 1. It has zero production
assets, explicit previous-version compatibility, dependencies, essential flags,
platform/tier variants, localization/audio references and vehicle/formation/route/station
bindings. It mirrors IR-S6-fixture-1 and chunks 1/west and 2/east. It cannot grant a route,
set a signal, change topology or replace the existing S6 demand/streaming system.

## 8. Import workflow
IMPORT_STANDARD.md documents source -> validation -> cleanup -> scale/orientation ->
naming -> import -> materials -> LOD -> proxy -> manifest -> validation -> Android.
Tools/instantiate_content.py creates pending cost/readiness/provenance and optional
repeatability records, refusing to overwrite existing evidence. No DCC import is run.

## 9. Vehicle binding
Definition IDs 1/2 version 1 map to Vehicle.Proxy.01/.02 presentation IDs. Slots support
exterior, cab, bogie, wheel, lights, animation, audio and material/LOD profile. Visual
slots are null until content integration; current S7 proxies stay unchanged. Ordered
formation presentation reuse is recorded separately from physical formation identity.

## 10. Station binding
Station.Shantipur.01 references S7-Shantipur-01, logical platform edge 2, extent 95-145 m
and chunk 2. Slots cover platforms, signs, canopy, footbridge, furniture, lighting,
people/activity, ambience, regional kit and landmarks. The existing S7 stop/dwell/berth
contract remains authoritative. Route 2 has an explicit chunk/station presentation map.

## 11. Audio binding
Audio.Foundation.01 defines traction, rolling, brakes, horn, doors, compressor/fans,
cab controls, station ambience, announcements, environment ambience and rain. Each
layer consumes semantic state and has a null sound binding. No sounds are created;
S7's three synthetic cues remain byte-identical. Future auxiliary/weather observations
are contract names only, not newly implemented behaviour.

## 12. LOD/material/texture policy
Shared mobile material instances, named texture channels/groups, ASTC, world mipmaps,
LOD0/reduction declarations and simple/query proxies are documented. Numerical limits
remain null/PROVISIONAL pending measurement, review, acceptance and freezing. Optional
single-LOD/scale review warnings are distinguished from invalid structural metadata.

## 13. Streaming policy
Reuse S6 level ownership, demand, readiness gating, unloading and weak-reference audits.
Catalog bundles describe route, station, vehicle, audio and optional scenery lifetimes.
They are not Android download partitions or a nationwide streaming implementation.
Presentation handles must release when their owner unloads. Logical state never unloads
with a mesh or level. Runtime/host checks reject disagreement with the two compiled chunks.

## 14. Graphics-tier framework
Low/Medium/High records expose only presentation cost controls: textures, LOD distance,
vegetation, people, shadows, VFX and optional detail. Null inherits the existing fixture
configuration. No new CVars or logical-state changes occur. Unknown fields fail schema
validation, including attempted physics settings. Final tier budgets remain pending.

## 15. Localization/accessibility foundation
Four UTF-8 English/Hindi sample keys cover brake, pause, result and stop warning.
Configuration defines font scaling, standard/large touch layouts and text+symbol+color
signal meaning. LOCALIZATION_ACCESSIBILITY.md documents future FText/String Table
integration and font/shaping review. Existing S7 widgets are not claimed fully localized;
Hindi glyph coverage and human accessibility validation remain pending.

## 16. Android measurement harness
profile.json declares iQOO/API36/ARM64/Vulkan/ASTC, 6000 frames and a 10-170-second analysis
window. measure_s8.py validates finalized captures, computes nearest-rank frame/lane
statistics and FPS, and records available PSS/thermal/load/residency/package data with
input hashes. Missing/partial/zero-only GPU data stay UNVALIDATED. CPU lanes are never
summed. Profile-S8.ps1 tests capture on a paused S7 fixture with foreground/state guards
and restores the commandline override. It is a harness smoke, not production workload
or active service performance. Three paired runs are prescribed for later comparisons.

## 17. Representative workload definition
workload.json requires a long player formation, second distinct locomotive, AI traffic,
busy station, quiet adjoining route, streaming boundary, camera changes, day/night,
dry/rain and a demanding night/rain combination. Four environmental combinations are
enumerated. Status is defined-not-executed, missingProductionContent is true and execution
evidence is empty. Stock, region and final workload sizes await actual content selection.

## 18. Cost measurement system
Templates and schemas record actual preparation, manual/correction/integration work,
Android optimization, iterations, rework, AI assistance and verification. No cost estimate
is invented. CONTENT_COST_TEMPLATE.md explains effort versus elapsed time and the
second-content audit. Pending checklists require evidence for pass and reasons for exemptions.

## 19. Save compatibility foundation
Sidecar preflight checks catalog version, snapshot schema, service identity/version/policy,
vehicle definitions and route contract. Compatible previous catalog versions are explicit.
S6's binary identity/CRC/invariant checks and S7's exact serialized service header remain
mandatory; no checkpoint format or slots are modified. Unreal tests capture/restore a real
in-memory S7 record across a presentation-version change. Unsupported metadata rejects
without migrating or overwriting a save. This is a small preflight, not a migration system.

## 20. Validation tools
validate_s8.py uses a fail-closed evaluator for the checked-in JSON Schema keyword subset,
with no third-party dependency; it is not a general JSON Schema implementation. It checks
structure, naming, provenance, IDs, graph dependencies, bindings, chunks, versions,
localization and graphics fields. Release mode rejects the empty foundation and requires
per-asset Reviews/<AssetId>/readiness.json with final approval. Runtime checks are narrower
packaged-contract checks, so build-time validation must not be skipped.

## 21. Decision register
decision-register.json contains owner, status, evidence needed, options, reason, date and
affected systems for stock, region, rendering, device matrix, LOD/material/texture budgets,
streaming thresholds, people/activity, audio residency, graphics tiers and localization.
All twelve decisions remain pending. No arbitrary final performance limit is claimed.

## 22. Tests
23 host foundation tests pass, including S8-F01-F18 plus duplicate IDs/JSON keys,
nonfinite JSON, false readiness and empty-release rejection. S2 39, S3 28, S4 39,
S5 36, S6 50 and S7 46 regression cases pass (238 total). Seven Unreal Automation
suites pass with zero failures/warnings. S8-F19-F27 map source preservation, regressions,
Automation and device evidence in Evidence/test-mapping.json. Synthetic measurement
tests are explicitly separated from device measurements.

## 23. Android verification
{android.get('summary','Device verification pending.')}

{measurement}

APK: `{artifact['path']}`; {artifact['bytes']:,} bytes. SHA256:
`{artifact['sha256']}`. BuildCookRun completed successfully. The inherited S7 app display
name/version stays unchanged to preserve baseline configuration; artifact identity is
the SHA256 and the packaged S8 runtime report, not the display label.

## 24. Problems encountered
Initial ADB inventory was empty; the connected device became available on continuation.
The first build required installed-engine/cache access outside workspace sandboxing.
An authoring-script parenthesis error was corrected before any generated catalog existed.
The inherited device pull helper assumed a running process; it failed after successfully
copying saves while the app was stopped. That failure is retained and was not counted as
successful installation or runtime verification. Standard-library schemas avoid an absent
optional jsonschema dependency. Existing stale S5 project-description text is recorded,
not silently repaired. Build logs include inherited Gradle deprecation warnings.

## 25. Fixes
Corrected the authoring syntax, granted build-tool access through normal reviewed execution,
and made the S8-only helper tolerate an absent PID. Used hash-qualified resumable APK
transfer and retained old evidence separately. Packaging now runs catalog validation first.
No earlier-stage simulation or UI fix was introduced under this task.

## 26. Known limitations
No production assets, measured asset budgets, final tier behaviour, extensive translation,
font validation, full representative scenario or content #2 integration exists. The
catalog has foundation bindings and null visuals. Metadata cannot prove real geometry,
license authenticity, visual quality or user comprehension. Asset-level activation counters
are unavailable until future owners instrument them; chunk loads must not impersonate them.
The profile smoke deliberately keeps the saved service paused; it does not establish
active driving CPU cost or final Android capacity. Full S8 and S7 human gates remain open.

## 27. Scope statement
**Production assets intentionally not created.** No assets acquired/downloaded/generated,
no production locomotives/coaches/stations/people/vegetation/buildings/audio/weather/night
lighting/cinematics, no nationwide streaming and no S9 systems. Existing proxy/synthetic
content is untouched. Source-change manifest and SHA256 evidence index document the work.

## 28. Next S8 content task
**Next task: S8 representative content implementation.** Select representative stock and
region, acquire verified rights, integrate content #1 and then #2 through this pipeline,
measure the combined workload and review costs/budgets. This report does not authorize or
perform that content work. S7's two outstanding human gates still need their own evidence.

## Foundation acceptance

| Gate | Requirement | Result |
|---|---|---|
'''
for row in acceptance['gates']: report+=f'| {row["gate"]} | {row["requirement"]} | {row["status"]} |\n'
report+='''
Reproduction: REPRODUCTION.md. Raw results: Evidence/foundation-tests.json,
Evidence/UnrealAutomation/index.json, Evidence/android-verification.json. Source identity:
Evidence/source-change-manifest.json. Evidence hashes: Evidence/evidence-hashes.json.
'''
(DOC/'S8_FOUNDATION_REPORT.md').write_text(report,encoding='utf-8')
print(status)
