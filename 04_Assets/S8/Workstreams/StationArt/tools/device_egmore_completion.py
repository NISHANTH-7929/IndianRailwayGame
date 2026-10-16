"""Bounded isolated Egmore itinerary with one observed-layout Depart touch.

Uses normal engine controls, never changes normal checkpoints or simulation time.
This proves only the selected assisted itinerary, not full S8 acceptance.
"""
import argparse
import io
import json
import time
from PIL import Image
import device_egmore_service as smoke
device = smoke.device


def itinerary(out, token, duration):
    deadline = time.monotonic() + duration
    proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()
    stages = []
    tapped = False
    samples = 0
    while time.monotonic() < deadline:
        time.sleep(3)
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
        service = state.get('service', {})
        complete = service.get('phase') == 'Result'
        # Completed services intentionally pause. Validate all remaining readiness.
        candidate = dict(state)
        if complete:
            candidate['paused'] = False
        if not smoke.validate(candidate):
            if samples:
                raise RuntimeError('Service lost readiness or paused unexpectedly')
            continue
        samples += 1
        (out / 'telemetry.json').write_bytes(raw)
        (out / 'engine.log').write_bytes(log)
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Game not foreground; no touch or screenshot permitted')
        stage = service.get('station')
        if stage not in stages:
            stages.append(stage)
            (out / f'phase-{stage}.json').write_bytes(raw)
            (out / f'phase-{stage}.png').write_bytes(device.screenshot())
        if stage == 'ReadyToDepart' and not tapped:
            if state['locomotive'].get('cabCamera') is not False:
                raise RuntimeError('Depart layout requires observed exterior controls')
            picture = device.screenshot()
            with Image.open(io.BytesIO(picture)) as png:
                if png.size != (2800, 1260):
                    raise RuntimeError('Unexpected phone resolution; refuse guessed touch')
            (out / 'before-depart.png').write_bytes(picture)
            # Depart button centre in inspected 2800x1260 exterior layout.
            device.adb('shell', 'input', 'tap', '1228', '1135')
            tapped = True
            (out / 'depart-touch.json').write_text(json.dumps(dict(x=1228, y=1135, tick=service['ticks'])))
        if complete:
            passed = (service.get('success') is True and service.get('departureAuthorized') is True
                      and service.get('dwellTicks') == 300 and tapped
                      and all(p in stages for p in ('Approaching', 'Dwelling', 'ReadyToDepart', 'Departed', 'Completed')))
            (out / 'itinerary.json').write_text(json.dumps(dict(passed=passed, stages=stages,
                departTouchSent=tapped, service=service, scope='isolated assisted itinerary; reward not settled'), indent=2))
            if not passed:
                raise RuntimeError('Itinerary result failed required service checks')
            return samples
    raise RuntimeError('Itinerary deadline expired; no completion accepted')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    args.map, args.duration, args.cab = smoke.WORLD, 600, False
    smoke.run(args, exercise=itinerary)
