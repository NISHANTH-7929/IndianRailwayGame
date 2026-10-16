"""Bounded, observed-layout controls test in isolated recovered Egmore service.

Each numbered action file is supplied only after inspecting its paired screenshot.
Normal saves/settings are preserved by the existing isolated runner.
"""
import argparse
import io
import json
import time
from PIL import Image
import device_egmore_service as smoke

device = smoke.device


def exercise(out, token, duration):
    deadline = time.monotonic() + duration
    proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()
    for step in range(12):
        while time.monotonic() < deadline:
            time.sleep(2)
            if not device.exists(smoke.TELEMETRY):
                continue
            raw = device.adb('exec-out', 'cat', smoke.TELEMETRY).stdout
            try:
                state = json.loads(raw.decode('utf-8-sig'))
            except (ValueError, UnicodeError):
                continue
            log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
            start = log.rfind(token.encode())
            if start < 0 or proof not in log[start:]:
                continue
            if state.get('content') != 'IR-S8-EGMORE-1' or not state.get('logical', {}).get('valid') or state['logical'].get('faulted'):
                raise RuntimeError('Invalid Egmore state')
            if state.get('blocked') or not state.get('egmoreScene', {}).get('ready'):
                continue
            break
        else:
            raise RuntimeError('No ready state before deadline')
        (out / f'step-{step}.json').write_bytes(raw)
        (out / 'engine.log').write_bytes(log)
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Game not foreground')
        picture = device.screenshot()
        with Image.open(io.BytesIO(picture)) as png:
            if png.size != (2800, 1260):
                raise RuntimeError('Unexpected screen size')
        (out / f'step-{step}.png').write_bytes(picture)
        action_path = out / f'action-{step}.json'
        action_deadline = min(deadline, time.monotonic() + 90)
        while not action_path.exists() and time.monotonic() < action_deadline:
            time.sleep(1)
        if not action_path.exists():
            raise RuntimeError('Observed-layout action deadline expired')
        action = json.loads(action_path.read_text())
        if action.get('finish'):
            (out / 'controls-final.json').write_bytes(raw)
            return step + 1
        if action.get('phase') != state['service']['phase']:
            raise RuntimeError('Action phase differs from inspected screenshot')
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Game lost focus; no touch')
        x, y = action['x'], action['y']
        if not isinstance(x, int) or not isinstance(y, int) or not 0 <= x < 2800 or not 0 <= y < 1260:
            raise RuntimeError('Invalid touch coordinates')
        device.adb('shell', 'input', 'tap', str(x), str(y))
        time.sleep(min(10, max(1, action.get('wait', 2))))
    raise RuntimeError('Step limit reached')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    smoke.FLAGS += ' -S8AssetReviewResume'
    args.map, args.duration, args.cab = smoke.WORLD, 600, False
    smoke.run(args, exercise=exercise)
