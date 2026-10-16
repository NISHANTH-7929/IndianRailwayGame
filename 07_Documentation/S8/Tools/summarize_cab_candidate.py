"""Summarize complete device captures; never infer a pass from partial folders."""
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('label');args=p.parse_args()
if not args.label.replace('-','').isalnum():p.error('Invalid label')
base=ROOT/'07_Documentation/S8/Evidence/Cab'
wrapper=json.loads((base/(args.label+'-device')/'result.json').read_text(encoding='utf-8-sig'))
results=[]
for mode in ['day','night']:
    folder=base/(args.label+'-'+mode)
    files=sorted(folder.glob('state-*.json'),key=lambda f:int(f.stem.split('-')[-1]))
    states=[json.loads(f.read_text()) for f in files]
    restoration=json.loads((folder/'restoration.json').read_text())
    installed=json.loads((folder/'installed-apk.json').read_text())
    ticks=[s['logical']['tick'] for s in states]
    errors=[]
    for state in states:
        loco=state['locomotive'];poses={p['role']:p for p in loco['controlPoses']}
        expected=-130+260*min(1,abs(loco['speedKmh'])/180)
        errors.append(abs(poses['gauge_speed']['rotation'][2]-expected))
    checks={
        'all_30_samples':len(states)==30 and [f.name for f in files]==[f'state-{s}.json' for s in range(3,91,3)],
        'all_screenshots':all((folder/f'screen-{s}.png').is_file() and (folder/f'screen-{s}.png').stat().st_size>1000 for s in range(3,91,3)),
        'advancing_ticks':len(ticks)>1 and all(b>a for a,b in zip(ticks,ticks[1:])),
        'logical_valid':all(s['logical']['valid'] and not s['logical']['faulted'] for s in states),
        'correct_presentation':all(s['locomotive']['ready'] and s['locomotive']['class']=='WAP7' and s['locomotive']['parts']==15 for s in states),
        'cab_and_exterior':{s['locomotive']['cabCamera'] for s in states}=={True,False} and states[-1]['locomotive']['cabTransitions']>=5,
        'needle_tracks_reported_speed':bool(errors) and max(errors)<.02,
        'six_saves_and_commandline_restored':restoration.get('completed',False),
        'matching_installed_apk':installed['matches'] and installed['sha256'].lower()==wrapper['sha256'].lower(),
        'low_light_flag':('-S8LowLight' in (folder/'commandline.txt').read_text())==(mode=='night'),
    }
    results.append(dict(mode=mode,passed=all(checks.values()),checks=checks,max_sampled_needle_error_degrees=max(errors,default=None),first_tick=ticks[0] if ticks else None,last_tick=ticks[-1] if ticks else None,material_slots=states[-1]['locomotive']['materialSlots'] if states else None))
result=dict(passed=wrapper['passed'] and wrapper['wakeRestored'] and all(x['passed'] for x in results),apk_sha256=wrapper['sha256'],runs=results,wake_restored=wrapper['wakeRestored'],visual_accepted=False,note='Automated commands and camera transitions, not fresh physical touch verification, full-speed calibration or final visual acceptance.')
(base/(args.label+'-device')/'summary.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
