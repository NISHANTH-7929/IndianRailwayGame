"""Complete one isolated assisted service and save its earned reward via the UI."""
import argparse
import io
import json
import time
from PIL import Image
import device_egmore_completion as completion

smoke = completion.smoke
device = smoke.device


def settlement_checks(before, after):
    a, b = before['service'], after['service']
    pa, pb = a['profile'], b['profile']
    return dict(success=b.get('success') is True, settled=b.get('settled') is True,
                identity=b.get('settlementId') == a.get('settlementId'),
                phase=b.get('phase') == 'Result',
                awards=pb.get('awards') == pa['awards'] + 1,
                reputation=pb.get('reputation') == pa['reputation'] + 10,
                run=pb.get('lastSettledRun') == a['run'],
                nextRun=pb.get('nextRun') == pa['nextRun'],
                generation=after.get('generation', 0) > before.get('generation', 0))


def exercise(out, token, duration):
    count = completion.itinerary(out, token, duration - 45)
    before = json.loads((out / 'telemetry.json').read_text(encoding='utf-8-sig'))
    return settle(out, before, count)


def settle(out, before, count=0):
    if before['service'].get('settled') is not False or before['service'].get('success') is not True:
        raise RuntimeError('Expected completed, unsettled isolated service')
    focus = device.adb('shell', 'dumpsys', 'window').stdout
    if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
        raise RuntimeError('Game not foreground; no settlement input')
    picture = device.screenshot()
    with Image.open(io.BytesIO(picture)) as png:
        if png.size != (2800, 1260) or before['locomotive'].get('cabCamera') is not False:
            raise RuntimeError('Unexpected settlement layout')
    (out / 'before-settlement.png').write_bytes(picture)
    # Centre of SAVE REPUTATION + UNLOCK in inspected completed-service layout.
    device.adb('shell', 'input', 'tap', '1400', '739')
    deadline = time.monotonic() + 40
    while time.monotonic() < deadline:
        time.sleep(2)
        raw = device.adb('exec-out', 'cat', smoke.TELEMETRY).stdout
        try:
            state = json.loads(raw.decode('utf-8-sig'))
        except (ValueError, UnicodeError):
            continue
        if not state.get('service', {}).get('settled'):
            continue
        checks = settlement_checks(before, state)
        (out / 'settled-telemetry.json').write_bytes(raw)
        (out / 'settlement.json').write_text(json.dumps(dict(passed=all(checks.values()), checks=checks,
            touch=dict(x=1400, y=739), scope='isolated reward save; cold recovery verified separately'), indent=2))
        (out / 'settled.png').write_bytes(device.screenshot())
        if not all(checks.values()):
            raise RuntimeError('Reward settlement checks failed')
        return count + 1
    raise RuntimeError('No verified settlement after UI input')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    args.map, args.duration, args.cab = smoke.WORLD, 600, False
    smoke.run(args, exercise=exercise)
