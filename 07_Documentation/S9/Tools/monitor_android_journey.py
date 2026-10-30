"""Read-only device observations; never advances or edits the simulation."""
import argparse, json, subprocess, time
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--device', required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--seconds', type=int, default=2100)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
out = a.output / 'observations.jsonl'
if out.exists():
    raise SystemExit('Use a fresh observation directory')
package = 'com.indianrailway.s1harness'
saved = f'/sdcard/Android/data/{package}/files/UnrealGame/MyProject/MyProject/Saved'
remote = saved + '/S9/EgmoreTambaram/LiveServiceV1/telemetry.json'

def adb(*args):
    return subprocess.run(['adb', '-s', a.device, *args], capture_output=True,
                          timeout=25, check=True).stdout

start = time.monotonic()
last_health = -60
while time.monotonic() - start < a.seconds:
    elapsed = time.monotonic() - start
    try:
        raw = adb('shell', 'cat', remote)
        state = json.loads(raw)
        row = {'elapsedSeconds': elapsed, 'utc': time.time(), 'state': state}
        with out.open('a', encoding='utf-8') as f:
            f.write(json.dumps(row, separators=(',', ':')) + '\n')
        (a.output / 'latest.json').write_bytes(raw)
        if elapsed - last_health >= 60:
            stamp = f'{int(elapsed):04}'
            for name, args in [('memory', ('shell','dumpsys','meminfo',package)),
                               ('battery', ('shell','dumpsys','battery')),
                               ('thermal', ('shell','dumpsys','thermalservice'))]:
                (a.output / f'{stamp}-{name}.txt').write_bytes(adb(*args))
            print(json.dumps({'elapsed': elapsed, 'tick': state['service']['ticks'],
                              'paused': state['paused'], 'blocked': state['blocked'],
                              'phase': state['service']['phase']}), flush=True)
            last_health = elapsed
        if state['service']['phase'] == 'Result' or state['logical']['faulted']:
            (a.output / 'terminal.json').write_bytes(raw)
            break
    except (subprocess.SubprocessError, ValueError, KeyError) as e:
        with (a.output / 'read-errors.txt').open('a') as f:
            f.write(f'{elapsed:.1f}: {e}\n')
        print(f'Device read failed: {e}', flush=True)
        break
    time.sleep(5)
