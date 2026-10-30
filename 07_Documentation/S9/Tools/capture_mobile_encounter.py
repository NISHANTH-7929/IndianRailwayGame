"""Capture the game only during a specified observed simulation interval."""
import argparse,json,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('evidence',type=Path);p.add_argument('--from-tick',type=int,required=True);p.add_argument('--to-tick',type=int,required=True)
a=p.parse_args();out=a.evidence/'encounter-captures';out.mkdir(exist_ok=True)
deadline=time.monotonic()+600
while time.monotonic()<deadline:
    latest=max(a.evidence.glob('drive*/latest.json'),key=lambda f:f.stat().st_mtime)
    s=json.loads(latest.read_text())
    tick=s['service']['ticks']
    if tick>a.to_tick:break
    if a.from_tick<=tick and not s['paused']:
        focus=subprocess.run(['adb','shell','dumpsys','window'],capture_output=True,timeout=20,check=True).stdout
        if not any(b'mCurrentFocus' in l and b'com.indianrailway.s1harness' in l for l in focus.splitlines()):
            break
        raw=subprocess.run(['adb','exec-out','screencap','-p'],capture_output=True,timeout=20,check=True).stdout
        (out/f'tick-{tick}.png').write_bytes(raw)
    time.sleep(3)
