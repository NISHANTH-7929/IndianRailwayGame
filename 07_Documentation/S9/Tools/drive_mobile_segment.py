"""Bounded Standard-mode UI test segment; never edits saves or simulation state.

Resume the game and inspect its controls before running. Every segment ends by
backgrounding the game, which invokes its normal pause/save lifecycle. This is
injected-input functional testing, not a human driving qualification.
"""
import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--device', required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--seconds', type=int, choices=range(1, 46), required=True)
p.add_argument('--power', type=int, choices=[0, 10, 25, 50, 100], required=True)
p.add_argument('--brake', type=int, choices=[0, 15, 30, 50, 100], required=True)
p.add_argument('--max-kmh', type=float, required=True)
a = p.parse_args()
if not 0 < a.max_kmh <= 100:
    p.error('max-kmh must be between 0 and 100')
a.output.mkdir(parents=True, exist_ok=False)
package = 'com.indianrailway.s1harness'
remote = f'/sdcard/Android/data/{package}/files/UnrealGame/MyProject/MyProject/Saved/S9/EgmoreTambaram/LiveServiceV1/telemetry.json'


def adb(*args):
    return subprocess.run(['adb', '-s', a.device, *args], check=True,
                          capture_output=True, timeout=8).stdout


def focused():
    return any(b'mCurrentFocus' in line and package.encode() in line
               for line in adb('shell', 'dumpsys', 'window').splitlines())


def read():
    state = json.loads(adb('shell', 'cat', remote))
    with (a.output / 'observations.jsonl').open('a', encoding='utf-8') as f:
        f.write(json.dumps({'utc': time.time(), 'state': state}) + '\n')
    return state


def valid(s):
    return (s['service']['mode'] == 1 and s['service']['phase'] == 'Running'
            and not s['paused'] and not s['blocked']
            and s['logical']['valid'] and not s['logical']['faulted'])


reason = 'duration completed'
started = False
failed = False
try:
    if not focused():
        raise RuntimeError('Game is not foreground; no controls sent')
    state = read()
    if not valid(state):
        raise RuntimeError('Expected active, valid Standard service; no controls sent')
    started = True
    if abs(state['locomotive']['speedKmh']) >= a.max_kmh and a.power > 0:
        raise RuntimeError('Already at test speed guard; refusing additional power')
    # Layout verified at 2800 x 1260. Refuse other resolutions.
    size = adb('shell', 'wm', 'size').decode()
    effective_size = size.strip().splitlines()[-1]
    if '1260x2800' not in effective_size and '2800x1260' not in effective_size:
        raise RuntimeError('Device resolution differs from inspected control layout')
    x = {0: 90, 10: 250, 15: 250, 25: 405, 30: 405, 50: 560, 100: 720}
    adb('shell', 'input', 'tap', '90', '1130')  # Cut power before changing brakes.
    adb('shell', 'input', 'tap', str(x[a.brake]), '1210')
    adb('shell', 'input', 'tap', str(x[a.power]), '1130')
    end = time.monotonic() + a.seconds
    last_tick = state['service']['ticks']
    last_change = time.monotonic()
    while time.monotonic() < end:
        time.sleep(1)
        if not focused():
            reason = 'foreground lost; game lifecycle owns pause'
            break
        state = read()
        if not valid(state):
            reason = 'service paused, blocked, ended, or faulted'
            break
        tick = state['service']['ticks']
        if tick < last_tick:
            raise RuntimeError('Telemetry tick regressed')
        if tick != last_tick:
            last_tick, last_change = tick, time.monotonic()
        if time.monotonic() - last_change > 8:
            raise RuntimeError('Telemetry is stale')
        if abs(state['locomotive']['speedKmh']) >= a.max_kmh:
            reason = 'test speed guard reached'
            break
except Exception as exc:
    reason = str(exc)
    failed = True
finally:
    if started:
        # Home is independent of the current in-game panel. No coordinate tap
        # may accidentally resume an overload pause or dismiss a result.
        try:
            if focused():
                adb('shell', 'input', 'keyevent', 'KEYCODE_HOME')
            time.sleep(1)
            state = read()
            (a.output / 'final.json').write_text(json.dumps(state), encoding='utf-8')
            if not state['paused'] and state['service']['phase'] == 'Running':
                reason += '; WARNING: pause not confirmed'
                failed = True
        except Exception as exc:
            reason += f'; pause verification failed: {exc}'
            failed = True
    (a.output / 'result.json').write_text(json.dumps({'reason': reason, 'controlsSent': started}), encoding='utf-8')
    print(reason, flush=True)
sys.exit(1 if failed else 0)
