"""Normalize finalized Unreal CSV and optional device telemetry; never infer absent samples.
Usage: python measure_s8.py capture.csv --apk build.apk --device samples.json --output report.json
Device samples use metric arrays {PSS: [...], loadLatency: [...], residency: [...],
activationCounts: [...], thermalStatus: [...]}; units are documented in the profile method.
"""
import argparse
import csv
import hashlib
import json
import math
import statistics
from pathlib import Path
from validate_s8 import DATA, load, validate_record

LANES=['FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RailS6/SimulationStepMs','RailS7/ServiceUpdateMs']
DEVICE=['PSS','loadLatency','residency','activationCounts','thermalStatus']

def digest(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()

def distribution(values):
    ordered=sorted(values)
    return dict(count=len(values),mean=statistics.fmean(values),p50=ordered[math.ceil(len(values)*.5)-1],
                p95=ordered[math.ceil(len(values)*.95)-1],p99=ordered[math.ceil(len(values)*.99)-1],maximum=max(values))

def summarize(source,profile,device=None,apk=None):
    csv.field_size_limit(16 * 1024 * 1024)
    errors=validate_record('profile',profile)
    if errors: raise ValueError('; '.join(errors))
    if profile['endSeconds']<=profile['warmupSeconds']: raise ValueError('Invalid profile analysis window')
    with Path(source).open(encoding='utf-8-sig',newline='') as handle: rows=list(csv.reader(handle))
    if not rows or not rows[-1] or rows[-1][0]!='[HasHeaderRowAtEnd]': raise ValueError('CSV is not finalized; retain incomplete capture')
    headers=[r for r in rows if r and r[0]=='EVENTS']
    if not headers: raise ValueError('Missing Unreal CSV header')
    header=headers[-1]
    if 'FrameTime' not in header: raise ValueError('Missing FrameTime')
    if any(header[:len(h)]!=h for h in headers): raise ValueError('CSV column order changed')
    for name in LANES:
        if header.count(name)>1: raise ValueError(f'Ambiguous duplicate metric: {name}')
    index=header.index('FrameTime'); elapsed=0.; selected=[]; count=0
    for row in rows:
        if not row or row[0]=='EVENTS' or row[0].startswith('['): continue
        try: frame=float(row[index])
        except (ValueError,IndexError): continue
        if not math.isfinite(frame) or frame<=0: raise ValueError('Invalid frame time')
        elapsed+=frame/1000; count+=1
        if profile['warmupSeconds']<elapsed<=profile['endSeconds']: selected.append(row)
    if not selected: raise ValueError('No frames in declared analysis window')
    if elapsed<profile['endSeconds']: raise ValueError('Capture does not cover declared window')
    result=dict(schemaVersion=1,scope=profile['scope'],profileId=profile['profileId'],source=str(source),sourceSha256=digest(source),
        profile=profile,frames=count,analysisFrames=len(selected),captureSeconds=elapsed,percentileMethod='nearest rank',
        interpretation='Engine intervals, not compositor timings. CPU/GPU lanes overlap; never sum. Simulation step is included in service update.',metrics={})
    for name in LANES:
        values=[]
        if name in header:
            ix=header.index(name)
            for row in selected:
                try: value=float(row[ix])
                except (ValueError,IndexError): continue
                if math.isfinite(value) and value>=0: values.append(value)
        if len(values)!=len(selected) or not values or (name=='GPUTime' and not any(values)):
            result['metrics'][name]=dict(status='UNVALIDATED',reason='absent, partial, invalid or zero-only GPU samples',validSamples=len(values))
        else: result['metrics'][name]=dict(status='CAPTURED',unit='ms',**distribution(values))
    frame_values=[float(row[index]) for row in selected]
    result['metrics']['FPS']=dict(status='CAPTURED',value=1000/statistics.fmean(frame_values))
    for name in DEVICE:
        values=(device or {}).get(name,[])
        if not values: result['metrics'][name]=dict(status='UNVALIDATED',reason='no device samples supplied'); continue
        if any(type(v) not in (float,int) or not math.isfinite(v) or v<0 for v in values): raise ValueError(f'Invalid device metric {name}')
        result['metrics'][name]=dict(status='CAPTURED',unit='KiB' if name=='PSS' else 'ms' if name=='loadLatency' else 'count/status',**distribution(values))
    result['metrics']['packageBytes']=dict(status='CAPTURED',value=Path(apk).stat().st_size,sha256=digest(apk)) if apk else dict(status='UNVALIDATED',reason='APK not supplied')
    result['finalBudgetClaim']=False
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('csv',type=Path);p.add_argument('--profile',type=Path,default=DATA/'profile.json');p.add_argument('--device',type=Path);p.add_argument('--apk',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    try:
        report=summarize(a.csv,load(a.profile),load(a.device) if a.device else None,a.apk)
        if a.device: report['deviceSource']=dict(path=str(a.device),sha256=digest(a.device))
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(a.output)
    except (ValueError,OSError) as error: p.exit(1,f'{error}\n')
