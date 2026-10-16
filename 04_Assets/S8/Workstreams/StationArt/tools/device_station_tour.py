"""Capture all six Scene02 interior/train cameras; never call static art gameplay."""
import argparse
import json
import time
import device_review as device


def exercise(out):
    seen = set()
    samples = []
    startup_view_zero = False
    startup_advanced = False
    began = time.monotonic()
    while time.monotonic() - began < 100:
        time.sleep(3)
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Game left foreground; screenshot refused')
        raw = device.adb('exec-out', 'cat', device.SAVED + '/S8/EgmoreScene02/Review/telemetry.json', check=False)
        if raw.returncode:
            continue
        try:
            state = json.loads(raw.stdout)
        except ValueError:
            continue
        if (state.get('family') != 'Station.Egmore.Scene.02' or state.get('interior') != '2.1.0'
                or state.get('placements') != 1316 or state.get('modules') != 40
                or state.get('staticArtReview') is not True or state.get('gameplayBound') is not False):
            raise RuntimeError(f'Unexpected Scene02 identity/counts: {state}')
        view = state.get('view')
        if type(view) is not int or not 0 <= view < 6:
            raise RuntimeError('Invalid review camera')
        if not startup_advanced:
            if view == 0:
                startup_view_zero = True
            elif startup_view_zero and view == 1:
                startup_advanced = True
            if not startup_advanced:
                continue
        samples.append(dict(seconds=time.monotonic() - began, state=state))
        (out / 'tour-samples.json').write_text(json.dumps(samples, indent=2), encoding='utf-8')
        if view not in seen:
            screenshot = device.screenshot()
            # The runtime tour advances independently of ADB. Never label a
            # screenshot with a camera that changed while it was being copied.
            try:
                after = json.loads(device.adb('exec-out', 'cat', device.SAVED + '/S8/EgmoreScene02/Review/telemetry.json').stdout)
            except ValueError:
                continue
            if after != state:
                continue
            (out / f'view-{view}.png').write_bytes(screenshot)
            (out / f'view-{view}-memory.txt').write_bytes(device.adb('shell', 'dumpsys', 'meminfo', device.PACKAGE).stdout)
            seen.add(view)
            print(f'Captured Scene02 camera {view}: {len(seen)}/6', flush=True)
        if len(seen) == 6:
            log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
            if log.count(b'S8_EGMORE_SCENE02_REVIEW') < 6:
                continue
            (out / 'tour-result.json').write_text(json.dumps(dict(views=sorted(seen),
                complete=True, visualAccepted=False, gameplayValidated=False), indent=2))
            return
    raise RuntimeError(f'Incomplete fresh camera tour: {sorted(seen)}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    if not device.re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        parser.error('Invalid label')
    args.map = '/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study'
    args.flags = '-S8EgmoreCameraTour -S8EgmoreView=0'
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    before = device.preflight()
    (out / 'preflight.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    device.capture(args, out, before, exercise=exercise)
    print(out)
