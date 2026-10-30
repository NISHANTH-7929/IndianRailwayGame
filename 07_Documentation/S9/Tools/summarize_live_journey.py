"""Aggregate live-service evidence; never infer completion from host results."""
import json,hashlib,sys
from pathlib import Path
root=Path(__file__).resolve().parents[3]
e=Path(sys.argv[1]);out=Path(sys.argv[2]);assert not out.exists()
rows=[]
for p in sorted(e.glob('*observations*.jsonl')):
 rows.extend(json.loads(line) for line in p.read_text(encoding='utf-8-sig').splitlines() if line)
def read(name):return json.loads((e/name).read_text(encoding='utf-8-sig'))
terminal=read('terminal-telemetry.json');settled=read('settled-telemetry.json');restored=read('settled-reloaded-telemetry.json')
cross=[r for r in rows if any(t['id']==2 and t['edge'] in (90000,90001) for t in r.get('trains',[]))]
baseline=json.loads((root/'07_Documentation/S9/Evidence/PackagedUI-20261016-01/normal-save-before.json').read_text(encoding='utf-8-sig'))
changed=[p for p,h in baseline.items() if not Path(p).exists() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
report=dict(service=terminal['service'],samples=len(rows),minTick=min(r['tick'] for r in rows),maxTick=max(r['tick'] for r in rows),
 sampledFaults=sum(bool(r['faulted']) for r in rows),sampledInvalidFingerprints=sum(r['fingerprint']=='INVALID' for r in rows),
 sampledBlocked=sum(bool(r['blocked']) for r in rows),maxWorkingSetBytes=max(r['workingSetBytes'] for r in rows),
 maxCellsPerView=max(v['cells'] for r in rows for v in r.get('views',[])),
 crossoverSamples=len(cross),bothPointsLockedDuringSampledCrossing=bool(cross) and all(len([p for p in r['points'] if p['position']==1 and p['route']==2 and p['owner']==2])==2 for r in cross),
 settledProfile=settled['service']['profile'],restoredProfile=restored['service']['profile'],
 rewardRecoveredOnce=settled['service']['settled'] and restored['service']['settled'] and settled['service']['profile']==restored['service']['profile'] and restored['service']['profile']['awards']==1,
 settledFingerprintPreserved=settled['logicalFingerprint']==restored['logicalFingerprint'],
 normalCheckpointsChecked=len(baseline),normalCheckpointsChanged=changed,
 limits=['Journey deliberately saved and resumed between presentation builds.','Five-second sampling cannot exclude sub-sample stalls.','Frame timing and screenshots are separate evidence.','Representative scenario, synthetic supplemental sound, no prototype or timetable certification.'])
out.write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
assert terminal['service']['success'] and report['rewardRecoveredOnce'] and report['settledFingerprintPreserved'] and not changed
