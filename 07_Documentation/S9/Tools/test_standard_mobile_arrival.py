"""Injected-input Standard arrival test, using observed on-screen controls.

Requires a running Standard service, matching exported route profile and the
inspected 2800x1260 landscape layout. Never resumes, restores, settles, edits
saves, or advances simulation time. Always requests background pause on exit.
This is a functional automation run, not human manual-driving qualification.
"""
import argparse, json, math, subprocess, time
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--device',required=True)
p.add_argument('--profile',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--seconds',type=int,default=1800)
a=p.parse_args()
if not 1<=a.seconds<=1800:p.error('seconds must be 1..1800')
a.output.mkdir(parents=True,exist_ok=False)
profile=json.loads(a.profile.read_text())
pkg='com.indianrailway.s1harness'
remote=f'/sdcard/Android/data/{pkg}/files/UnrealGame/MyProject/MyProject/Saved/S9/EgmoreTambaram/LiveServiceV1/telemetry.json'

def adb(*args):
 return subprocess.run(['adb','-s',a.device,*args],capture_output=True,check=True,timeout=8).stdout
def focus():
 return any(b'mCurrentFocus' in l and pkg.encode() in l for l in adb('shell','dumpsys','window').splitlines())
def read():
 raw=adb('shell','cat',remote);s=json.loads(raw)
 (a.output/'latest.json').write_bytes(raw)
 with (a.output/'observations.jsonl').open('a') as f:f.write(json.dumps({'utc':time.time(),'state':s})+'\n')
 return s
def tap(x,y):
 if not focus():raise RuntimeError('Foreground lost before input')
 adb('shell','input','tap',str(x),str(y))

start=time.monotonic();last_change=start;last_tick=-1;last_report=-60;last_capture=-60
last_controls=None;reason='time limit';success=False
try:
 size=adb('shell','wm','size').decode().strip().splitlines()[-1]
 if '1260x2800' not in size and '2800x1260' not in size:raise RuntimeError('Unexpected resolution')
 while time.monotonic()-start<a.seconds:
  if not focus():raise RuntimeError('Game foreground lost')
  s=read();state=s['service'];tick=state['ticks']
  if state['mode']!=1:raise RuntimeError('Expected Standard mode')
  if state['phase']=='Result':
   success=state['success'];reason='terminal result'
   (a.output/'terminal.json').write_text(json.dumps(s))
   (a.output/'terminal.png').write_bytes(adb('exec-out','screencap','-p'))
   break
  if s['paused'] or s['blocked'] or s['logical']['faulted'] or not s['logical']['valid']:
   raise RuntimeError('Service paused, blocked or faulted; no automatic recovery')
  if tick<last_tick:raise RuntimeError('Tick regressed')
  if tick!=last_tick:last_tick=tick;last_change=time.monotonic()
  if time.monotonic()-last_change>8:raise RuntimeError('Stale telemetry')
  h=next(t['handling'] for t in s['logical']['trains'] if t['id']==2)
  head=h['headMetres'];v=abs(h['speedMps']);distance=profile['berth']-head
  index=max(0,min(len(profile['limits'])-1,int(head/profile['step'])))
  # Conservative look-ahead margin and cruise below the scenario ceiling.
  limit=min(profile['limits'][index:min(index+12,len(profile['limits']))])
  approach=-1.08+math.sqrt(1.08**2+2*.18*max(0,distance-1.5))
  target=min(85/3.6,max(0,limit-.9),max(.12,approach))
  if distance<=1.5 or state['station']=='Dwelling':power,brake=0,100
  elif v>target+.15:power,brake=0,50
  elif v>target-.7:power,brake=0,0
  else:power,brake=(100 if target-v>4 and distance>1000 else 25 if target-v>1.5 else 10),0
  if (power,brake)!=last_controls:
   tap(90,1130)
   tap({0:90,50:560,100:720}[brake],1210)
   if power:tap({10:250,25:405,100:720}[power],1130)
   last_controls=(power,brake)
  elapsed=time.monotonic()-start
  if elapsed-last_report>=30:
   print(json.dumps({'seconds':round(elapsed),'tick':tick,'head':round(head,1),'kmh':round(v*3.6,1),'targetKmh':round(target*3.6,1),'remaining':round(distance,1),'overspeedTicks':state['overspeedTicks']}),flush=True)
   last_report=elapsed
  if elapsed-last_capture>=60 and distance>1000 and focus():
   (a.output/f'driving-{tick}.png').write_bytes(adb('exec-out','screencap','-p'));last_capture=elapsed
  time.sleep(1)
except Exception as exc:reason=str(exc)
finally:
 try:
  if focus():adb('shell','input','keyevent','KEYCODE_HOME')
  time.sleep(1);s=read()
  (a.output/'final.json').write_text(json.dumps(s))
  if s['service']['phase']=='Running' and not s['paused']:reason+='; PAUSE NOT CONFIRMED'
 except Exception as exc:reason+=f'; pause verification failed: {exc}'
 (a.output/'result.json').write_text(json.dumps({'success':success,'reason':reason,'testType':'injected UI inputs in Standard mode, not human qualification'}))
 print(reason,flush=True)
raise SystemExit(0 if success else 1)
