"""Manual-touch regression for the exact installed refined cab, isolated saves."""
import argparse
import json
from pathlib import Path
import time
from PIL import Image
import device_review as device


def state():
    for _ in range(5):
        data = device.adb('exec-out', 'cat', device.SAVED + '/S8/AssetReview/telemetry.json', check=False)
        try:
            return json.loads(data.stdout)
        except (ValueError, UnicodeError):
            time.sleep(.3)
    raise RuntimeError('No readable cab telemetry')


def foreground():
    focus = device.adb('shell', 'dumpsys', 'window').stdout
    if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
        raise RuntimeError('Game left foreground; input/capture refused')


def exercise(out):
    for _ in range(45):
        time.sleep(2)
        try:
            current = state()
        except RuntimeError:
            continue
        loco = current.get('locomotive', {})
        if loco.get('cabTransitions') == 1 and loco.get('interactions') == 0 and current.get('logical', {}).get('tick', 999999) < 300:
            break
    else:
        raise RuntimeError('No fresh isolated manual cab session')
    foreground()
    (out / 'initial.png').write_bytes(device.screenshot())
    width, height = Image.open(out / 'initial.png').size
    # Existing build uses mobile content scale 1: 720-pixel short viewport axis.
    scale = height / 720
    results = []
    roles = ['train_brake', 'throttle', 'throttle', 'throttle', 'throttle',
             'train_brake', 'train_brake', 'train_brake', 'reverser', 'reverser',
             'reverser', 'switch_emergency', 'switch_emergency', 'switch_wipers', 'switch_cablight']
    for index, role in enumerate(roles):
        foreground()
        before = state()
        pose = next(p for p in before['locomotive']['controlPoses'] if p['role'] == role)
        x, y = [round(p * scale) for p in pose['screen']]
        if x > width * .95:
            device.adb('shell', 'input', 'swipe', str(width // 2), str(height // 3),
                       str(width // 2 + 350), str(height // 3), '500')
            time.sleep(2)
            before = state()
            pose = next(p for p in before['locomotive']['controlPoses'] if p['role'] == role)
            x, y = [round(p * scale) for p in pose['screen']]
        if not (0 < x < width and 0 < y < height * .86):
            raise RuntimeError(f'{role} touch target outside safe viewport: {x},{y}')
        device.adb('shell', 'input', 'swipe', str(x), str(y), str(x), str(y), '160')
        time.sleep(2)
        after = state()
        foreground()
        (out / f'touch-{index:02d}-{role}.png').write_bytes(device.screenshot())
        valid = after['logical']['valid'] and not after['logical']['faulted']
        after_pose = next(p for p in after['locomotive']['controlPoses'] if p['role'] == role)
        changed = after_pose['rotation'] != pose['rotation']
        field = {'throttle': 'commandThrottle', 'train_brake': 'commandBrake', 'reverser': 'direction'}.get(role)
        if field:
            changed = changed and after['locomotive'][field] != before['locomotive'][field]
        passed = (after['locomotive']['interactions'] == before['locomotive']['interactions'] + 1
                  and after['logical']['tick'] > before['logical']['tick'] and valid and changed)
        results.append(dict(role=role, pixel=[x, y], passed=passed, before=before, after=after))
        (out / 'touch-results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
        print(role, 'PASS' if passed else 'FAIL', flush=True)
        if not passed:
            raise RuntimeError(f'Touch interaction failed for {role}; evidence retained')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    if not device.re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        parser.error('Invalid evidence label')
    args.map = '/Game/IndianRailway/S7/L_RailService'
    args.flags = '-S8AssetReview -S8Cab -S8CabManual'
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    before = device.preflight()
    (out / 'preflight.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    device.capture(args, out, before, exercise=exercise)
    print(out)
