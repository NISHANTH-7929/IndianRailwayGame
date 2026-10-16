"""Cold-launch the three existing saves without resuming or replacing them."""
import json, subprocess, sys, time,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
E=ROOT/'07_Documentation/S8/Evidence'
helper=Path(__file__).with_name('lhb_device_evidence.py')
parser=argparse.ArgumentParser();parser.add_argument('--prefix',default='station-v6')
options=parser.parse_args();prefix=options.prefix
if not prefix.replace('-','').isalnum():raise RuntimeError('Invalid evidence prefix')
results={}
for family,label,flag,before in [
    ('LHB','station-v6-legacy','--legacy','station-v5-legacy-result'),
    ('LHB','station-v6-lhb',None,'station-v5-lhb-result'),
    ('Station','station-v6-final','--station','station-v6-cold-result')]:
    label=label.replace('station-v6',prefix)
    args=[flag] if flag else []
    subprocess.run([sys.executable,str(helper),'launch',label,*args],check=True)
    time.sleep(18)
    subprocess.run([sys.executable,str(helper),'capture',label+'-result',*args],check=True)
    old=json.loads((E/family/'Device'/before/'telemetry.json').read_text(encoding='utf-8-sig'))
    new=json.loads((E/family/'Device'/(label+'-result')/'telemetry.json').read_text(encoding='utf-8-sig'))
    checks={
        'identity':new['content']==old['content'],
        'fingerprint':new['logicalFingerprint']==old['logicalFingerprint'],
        'service':new['service']==old['service'],
        'paused':new['paused'],
        'unblocked':not new['blocked'],
        'valid':new['logical']['valid'] and not new['logical']['faulted'],
        'recreated':new['visualVehicles']==old['visualVehicles'] and new['loadedLevels']>0,
    }
    if family=='Station':
        checks['station_recreated']=new['stationInstances']==4216 and new['stationComponents']==30
        checks['lhb_recreated']=new['lhbVehicles']==8 and new['lhbComponents']==14
        checks['proxy_count_correct']=new['proxyInstances']==0
    results[label]={'passed':all(checks.values()),'checks':checks,'generation':new['generation'],
                    'fingerprint':new['logicalFingerprint'],'tick':new['service']['ticks'],
                    'before':before,'after':label+'-result'}
    matrix_name='recovery-matrix-v6.json' if prefix=='station-v6' else prefix+'-recovery-matrix.json'
    (E/'Station'/matrix_name).write_text(json.dumps(results,indent=2))
    if not all(checks.values()):raise RuntimeError(f'{label}: {checks}')
station=results[prefix+'-final']
cold_name='cold-recovery-v6.json' if prefix=='station-v6' else prefix+'-cold-recovery.json'
(E/'Station'/cold_name).write_text(json.dumps(station,indent=2))
print(json.dumps(results,indent=2))
