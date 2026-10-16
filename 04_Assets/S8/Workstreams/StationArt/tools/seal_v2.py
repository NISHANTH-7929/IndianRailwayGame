from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1]/'v2.0.0'
assert not (R/'READY.lock').exists()
for p in ('binary-audit.json','roundtrip.json','clearance.json'):
    assert json.loads((R/'Evidence'/p).read_text())['status']=='PASS',p
assert json.loads((R/'Evidence/game-dev-validation.json').read_text(encoding='utf-8-sig'))['data']['passed']
assert json.loads((R/'Evidence/package-verify-02.json').read_text(encoding='utf-8-sig'))['data']['hashesVerified']
m=json.loads((R/'manifest.json').read_text());m['status']='ready-for-source-integration-review';m['runtimeAccepted']=False;m['humanVisualSignoff']=False
m['signalBinding']='SignalUnbound is an unlit scenery housing. Integration must supply authoritative aspects; never infer signal state from static materials.'
(R/'manifest.json').write_text(json.dumps(m,indent=2))
(R/'READY.lock').write_text('Station.Egmore.Scene.02 v2.0.0 — immutable source integration handoff, 2026-10-07.\nPassed static/source tests; Unreal, Android and user visual acceptance pending.\nRevisions require a new version directory.\n')
records=[]
for p in sorted(R.rglob('*')):
    if p.is_file() and p.name!='SHA256SUMS.json':records.append({'path':p.relative_to(R).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(R/'SHA256SUMS.json').write_text(json.dumps(records,indent=2))
for r in records:assert hashlib.sha256((R/r['path']).read_bytes()).hexdigest()==r['sha256']
print(json.dumps({'filesHashed':len(records),'bytes':sum(r['bytes'] for r in records),'sceneSha256':next(r['sha256'] for r in records if r['path']=='EgmoreScene.glb'),'manifestSha256':next(r['sha256'] for r in records if r['path']=='manifest.json')}))
