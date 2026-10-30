"""Summarize recorded mobile evidence without treating missing checks as passes."""
import argparse, json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('evidence',type=Path)
a=p.parse_args(); root=a.evidence
rows=[]
for file in root.glob('drive*/observations.jsonl'):
    for line in file.read_text().splitlines():
        rows.append(json.loads(line)['state'])
cross=[]
for s in rows:
    player=next((t for t in s['logical']['trains'] if t['id']==2),None)
    if player and player['handling']['edge'] in (90000,90001):
        cross.append(s)
result={'samples':len(rows),'maxTick':max((s['service']['ticks'] for s in rows),default=0),
        'faultSamples':sum(bool(s['logical']['faulted']) for s in rows),
        'blockedSamples':sum(bool(s['blocked']) for s in rows),
        'crossoverSamples':len(cross),
        'pointsLockedDuringSampledCrossover':bool(cross) and all(
            len(s['logical']['points'])==2 and all(p['owner']==2 for p in s['logical']['points']) for s in cross),
        'terminalVerified':False,'rewardRecoveryVerified':False,
        'limits':['Telemetry is sampled, not continuous frame timing.',
                  'Touch inputs injected through Android ADB; physical ergonomics not certified.',
                  'Sound slider evidence does not establish acoustic fidelity.']}
for name in ['terminal','settled','settled-reloaded']:
    file=root/(name+'.json')
    if file.exists():result[name]=json.loads(file.read_text())
if 'terminal' in result:
    s=result['terminal']['service']
    result['terminalVerified']=s['phase']=='Result' and s['success'] and s['dwellTicks']>=300
if 'settled' in result and 'settled-reloaded' in result:
    x,y=result['settled'],result['settled-reloaded']
    result['rewardRecoveryVerified']=(x['logicalFingerprint']==y['logicalFingerprint'] and
        x['service']['settled'] and y['service']['settled'] and
        x['service']['profile']==y['service']['profile'] and y['service']['profile']['awards']==1)
for name in ['terminal','settled','settled-reloaded']:
    if name in result:result[name]=result[name]['service']
(root/'journey-summary.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k not in ['terminal','settled','settled-reloaded']},indent=2))
