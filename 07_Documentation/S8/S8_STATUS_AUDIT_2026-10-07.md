# S8 status audit — 7 October 2026

S8 is not complete. Technical integration is ahead of visual acceptance. This audit
supersedes stale current-status wording in older reports without rewriting their
historical evidence. Scope: WAP-7/LHB, representative Chennai Egmore station and
platform/infrastructure/context, reusable second configuration, final combined
Android workload. BLC remains explicitly deferred.

| Area | Evidence-backed status | Outstanding exit condition |
|---|---|---|
| Foundation | Complete for foundation scope, 29/29 gates | Do not equate foundation with full S8 |
| WAP-7 exterior | Imported, integrated, Android exercised | Prototype fidelity and final materials/visual review |
| WAP-7 cab | Functional candidate; 15 touch actions verified | Instrument faces, panel/glass materials, analog night illumination, variant/proportions |
| LHB AC3-tier | Versioned integration technically validated | Final art fidelity; source acquisition date/receipt unavailable despite account confirmation |
| Old save/fixture compatibility | V21: 263 Android cases, 10 Automation suites, three recovery modes pass | Repeat appropriate gates for final release candidate; intermittent earlier startup failure unresolved |
| WAG-9 | Exterior imported and separately measured historically | Integrated second representative configuration; own cab unfinished if retained in scope |
| Generic station | 26 reusable modules, A/B arrangements, instancing and recovery tested | Realistic materials/detail; A/B is not a finished Egmore station |
| Egmore | 12 heritage modules + separate multilingual board; V20/V21 phone renders | Full coherent station layout, proportions, context and visual acceptance |
| Latest V22 | Tower-side geometry and sky-fill candidate; import/LOD/GLB checks and Android packaging pass | No V22 phone capture/recovery/performance evidence located |
| OHE/trackside | Masts, portals, wire, cabinets and boards; V21 grade correction verified | Remaining reference details and corridor integration |
| Track/signals | Rails/sleepers/ballast follow graph; authoritative aspects exist | Switch blades/frogs/mechanisms and realistic signal housings/lamps; debug indicators remain |
| Environment/activity | Sparse ground/boundary context only | Representative quiet adjoining section, drainage, vegetation, urban context and bounded activity |
| PBR/lighting | Maps/LODs exist, multiple diagnostic passes | Convincing player-camera surfaces and phone lighting; not accepted |
| Night/rain/audio | Foundation contracts and limited cab low-light checks | Production scene night/rain/wet response/ambience; all four combinations unexecuted |
| Catalog/release | Separate runtime asset JSONs work | Top-level manifest remains foundation with assets[] empty; complete versioned production manifest/readiness |
| Combined Android workload | V21 generic-station run: 6000 frames,29.9687FPS,p9534.785ms,GPUmean21.1144ms,PSSmean621.60MiB | Actual final combined scene, repeated same-build runs, thermal/streaming/camera/weather coverage and measured tiers |
| Final acceptance/docs | Evidence preserved, architecture logged | Consolidate stale reports/decisions, per-asset sign-off, repeatability and actual cost record |

Why this has taken too long:
- Scope expanded after LHB completion into cab, station, Egmore, props and environment.
- Many small procedural art revisions were packaged and retested before resolving
  the larger scene/silhouette/material acceptance gaps.
- Export handedness, mirrored lettering, fallback shaders, mip streaming, shadow
  differences and interrupted device captures caused genuine rework.
- Technical passes accumulated without satisfying the requested visual standard.
- There was no single maintained completion board; older reports and decision
  records still describe earlier candidates. This obscured what was actually done.

Completion sequence:
1. Consolidate scope/inventory/release gates; retain BLC deferral and old identities.
2. Finish one reference-matched Egmore scene at player-camera distances: layout,
   major architecture, platforms/bridge/canopy, train fit and believable materials.
3. Finish railway infrastructure and limited adjoining context; replace visible
   debug signals through presentation bindings only.
4. Finish train/cab visible fidelity and integrate Egmore as a separate playable
   content/service configuration with its own stable identity, preserving Shantipur.
5. Exercise second reusable configuration without special gameplay/AI code.
6. Add representative night/rain/audio/activity and verify four workload combinations.
7. Complete production manifest/provenance/readiness; run final regressions,
   compatibility/rejection/recreation, repeated Android cost and thermal checks.
8. Accept visual comparisons, resolve remaining reliability issues, archive APK and
   evidence, publish one final S8 report with actual costs and limitations.

Workflow correction: batch related art changes, visually compare before packaging,
use targeted checks per change, and run the broad suite at meaningful integration
milestones. Do not claim percentages, perfect realism or dates without evidence.

Key evidence:
- S8_FOUNDATION_REPORT.md
- LHB_VALIDATION_REPORT.md
- Evidence/Cab/FINAL_DEVICE_REVIEW.md
- Evidence/Station/v21-validation-summary.json
- Evidence/Egmore/package-20261005-181300-result.json
- Evidence/Egmore/content-validation.json
- Content/IndianRailway/Production/manifest.json and workload.json
