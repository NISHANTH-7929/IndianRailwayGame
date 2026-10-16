"""Audit runtime pass-by telemetry plus actual stereo PCM, not listener preference."""
import json, re, sys, wave
from pathlib import Path
import numpy as np
run=Path(sys.argv[1]);log=(run/'engine.log').read_text(errors='replace')
pattern=r'RAIL_AUDIO_SYNTHETIC_PASS t=([\d.]+) head=([-\d.]+) hornDistance=([\d.]+) rollingDistance=([\d.]+) hornGain=([\d.]+) rollingGain=([\d.]+) hornPitch=([\d.]+) rollingPitch=([\d.]+)'
rows=[dict(zip(('time','head','hornDistance','rollingDistance','hornGain','rollingGain','hornPitch','rollingPitch'),map(float,m))) for m in re.findall(pattern,log)]
assert len(rows)>35, 'Missing complete pass telemetry'
near=max(rows,key=lambda r:r['rollingGain']);approach=[r for r in rows if 200<r['head']<500];depart=[r for r in rows if -600<r['head']< -300]
with wave.open(str(run/'native/motion-audio.wav')) as w:
 rate=w.getframerate();assert w.getnchannels()==2 and w.getsampwidth()==2
 x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float).reshape(-1,2)/32768
pan=[]
for start in range(0,len(x)-rate,rate):
 energy=np.mean(x[start:start+rate]**2,axis=0)
 if energy.sum()>1e-7:pan.append(float((energy[1]-energy[0])/energy.sum()))
checks=dict(growsAndFades=near['rollingGain']>rows[0]['rollingGain']*5 and rows[-1]['rollingGain']<near['rollingGain']*.1,
 dopplerDirection=bool(approach and depart and np.mean([r['hornPitch'] for r in approach])>1 and np.mean([r['hornPitch'] for r in depart])<1),
 oneHorn=log.count('RAIL_AUDIO traffic approach horn')==1,
 stereoPass=bool(pan and min(pan)<-.2 and max(pan)>.2),
 unclipped=bool(np.max(np.abs(x))<.999))
report=dict(passed=all(checks.values()),checks=checks,peak=float(np.max(np.abs(x))),panRange=[min(pan),max(pan)] if pan else [],trajectory=rows,scope='Synthetic pass through runtime audio mixer; actual AI operating route and subjective listening not validated.')
(run/'passing-audio-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='trajectory'}));sys.exit(0 if report['passed'] else 1)
