"""Finish the human report and hash index after finalize_s6_evidence succeeds."""
from pathlib import Path
import hashlib
import json

doc = Path(__file__).resolve().parents[1]
ev = doc/'Evidence'
def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path, value): path.write_text(json.dumps(value,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
acceptance=read(ev/'acceptance.json')
assert acceptance['status']=='S6 PASS' and acceptance['accepted']==30
profile=read(ev/'profile-final-summary.json')
audit=read(ev/'Device/sustained-final/residency-summary.json')
assert audit['passed']
frame=profile['counters']['FrameTime']
counter_rows=[]
for name in ['GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RailS6/AIUpdateMs','RailS6/SimulationStepMs']:
    c=profile['counters'][name]
    counter_rows.append(f"| {name} | {c['mean']:.4f} | {c['p95']:.4f} | {c['p99']:.4f} |" if c['status']=='CAPTURED' else f"| {name} | UNVALIDATED | — | — |")
counter_table='| Counter (ms) | Mean | p95 | p99 |\n|---|---|---|---|\n'+'\n'.join(counter_rows)
report=doc/'S6_IMPLEMENTATION_REPORT.md'
text=report.read_text(encoding='utf-8')
text=text.replace('**IN PROGRESS. Final packaged tests and acceptance audit are pending.**','**S6 PASS**  \n**Acceptance: 30/30**  \nValidated 2026-09-27 on the iQOO Neo 9 Pro. Technical prototype scope only.')
start=text.index('Pending final profile and 450-second audit.')
end=text.index('\n## 16.',start)
performance=f'''Final capture: 6,000 engine frames. Analysis window 10–170 seconds contains {frame['count']:,} frames, averaging {profile['average_fps']:.3f} FPS. Frame intervals: p50 {frame['p50']:.3f} ms, p95 {frame['p95']:.3f} ms, p99 {frame['p99']:.3f} ms, maximum {frame['max']:.3f} ms; {profile['frames_over_50ms']} frames above 50 ms and {profile['frames_over_100ms']} above 100 ms. The scenario includes controlled pauses and real level cycles; these are engine intervals, not compositor presentation timing. See `Evidence/profile-final-summary.json` and its raw CSV source.

The final sustained run completed {audit['completion']['seconds']:.2f} seconds with {audit['sampleCount']} logical samples, {audit['explicitCycles']} forced paired unload/reload cycles, {audit['loads']} load requests and {audit['unloads']} completed unloads. Actual residency stayed within {audit['peakLoadedLevels']} levels / {audit['peakChunkActors']} chunk Actors / 1,024 proxies. Demand-driven single-level operation was observed while simulation advanced. Retired Actor and unique-mesh references returned to zero at the end; every sampled logical state was valid and no railway overload/fatal/assertion match occurred in the fresh engine log.

Sampled PSS range: {audit['pssMinKiB']/1024:.2f}–{audit['pssMaxKiB']/1024:.2f} MiB. First-three median {audit['firstThreeMedianPssKiB']/1024:.2f} MiB; last-three median {audit['lastThreeMedianPssKiB']/1024:.2f} MiB; change {audit['medianPssGrowthKiB']/1024:+.2f} MiB, within the predeclared +32 MiB bound. Android thermal statuses observed: {audit['thermalStatuses']}. Maximum sampled chunk request-to-ready latency {audit['loadMsMaximum']:.3f} ms; maximum sampled save transaction {audit['saveMsMaximum']:.3f} ms. These samples are not exhaustive per-operation percentiles. See `Evidence/Device/sustained-final/residency-summary.json` and `Evidence/residency-audit-policy.json`.

{counter_table}

CPU lanes can overlap and include waits; do not sum these columns into frame time. AI/simulation averages include intentional paused frames.

Raw CPU/GPU/AI/simulation counters and their validation status are in the profile summary. Zero-only or absent GPU/temperature counters are explicitly UNVALIDATED and must not be interpreted as zero cost. PSS includes allocator/driver caches; this finite proxy-fixture run does not establish asymptotic leak freedom or final-art/world-scale capacity.

Earlier profile attempts, including USB-disconnected captures and the pre-camera-fix run, remain separately labelled and are excluded from final acceptance. The profiling launch override was restored after all attempts, including cleanup after reconnecting.
'''
text=text[:start]+performance+text[end:]
text=text.replace('Final sustained/performance validation remains pending; earlier attempts are labelled separately.','Final sustained/performance validation passed; earlier attempts remain labelled separately. Packaged startup reran all 192 pure cases successfully; see `Evidence/test-summary.json`.')
text=text.replace('Pending final 30-gate audit and S6-T01–T39 evidence mapping. S6 is not yet accepted.', 'All 30 gates pass for the stated two-chunk fixture. The exact 39 requested test IDs map to raw evidence in [test-mapping.json](Evidence/test-mapping.json); these mappings are not 39 independent executions. Machine-readable gates: [acceptance.json](Evidence/acceptance.json).\n\n'+(ev/'acceptance-table.md').read_text(encoding='utf-8')+'\nReproduction steps: [REPRODUCTION.md](REPRODUCTION.md). Source changes and baseline comparisons: [source-change-manifest.json](Evidence/source-change-manifest.json), [source-review.json](Evidence/source-review.json). Artifact identity: [artifact.json](Evidence/artifact.json). Hash index: [evidence-hashes.json](Evidence/evidence-hashes.json).')
assert text.count('\n## ')==21
assert 'pending' not in text.lower(), 'Resolve all remaining pending report statements'
report.write_text(text,encoding='utf-8')
manifest=read(doc/'content-manifest.json')
manifest['status']='Validated on final APK: control/recovery 21/21, compatibility 10/10, completed frame profile and 450-second finite residency audit. Two-chunk technical prototype only.'
write(doc/'content-manifest.json',manifest)
with (doc/'PROGRESS.md').open('a',encoding='utf-8') as f:
    f.write('\n\n## S6 completed — 2026-09-27\nS6 PASS. Acceptance: 30/30. Final controls 21/21, compatibility 10/10, host and packaged 192 cases each, Unreal Automation 5/5. Final frame capture and 450-second residency audit completed; all interrupted profiling overrides restored. See S6_IMPLEMENTATION_REPORT.md and Evidence/acceptance.json. Stop here. Exact next dependency: S7 — Vertical slice: complete player loop.\n')
hashes=[]
for path in sorted(doc.rglob('*')):
    if not path.is_file() or path.name=='evidence-hashes.json' or '__pycache__' in path.parts: continue
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
    hashes.append({'path':path.relative_to(doc).as_posix(),'bytes':path.stat().st_size,'sha256':h.hexdigest().upper()})
write(ev/'evidence-hashes.json',{'algorithm':'SHA256','scope':'All S6 documentation, tools and retained evidence including historical failed attempts; excludes this self-referential index and Python caches. APK identity is in artifact.json.','files':hashes})
print(f'S6 PASS; Acceptance: 30/30; 21 report sections; {len(hashes)} hashed files.')
