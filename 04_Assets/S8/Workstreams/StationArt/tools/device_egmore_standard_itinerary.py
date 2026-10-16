"""Exercise Standard mode using only observed phone UI power/brake buttons.

This is a scripted-input gameplay test, not a human usability or performance test.
No simulation commands, time scaling or normal-save writes are used.
"""
import argparse
import io
import json
import math
import time
from PIL import Image
import device_egmore_service as smoke
from device_cold_recovery import validate

device = smoke.device
BUTTONS = {'resume': (1400, 537), 'power0': (215, 1080), 'power25': (610, 1080),
           'release': (1795, 1080), 'brake50': (2190, 1080), 'brake100': (2580, 1080),
           'depart': (1228, 1135)}


def controls(remaining, speed, dwell):
    if dwell or remaining <= 1.1:
        return 0, 1
    target = min(2.1, math.sqrt(max(0, remaining - .5) * .40))
    if speed > target + .12:
        return 0, .5
    if speed < target - .18:
        return .25, 0
    return 0, 0


def run(args):
    device.preflight()
    raw_ref = device.adb('exec-out', 'cat', device.SAVED + '/S8/Egmore/AssetReview/last-checkpoint.json').stdout
    reference = json.loads(raw_ref.decode('utf-8-sig'))
    if reference.get('service', {}).get('mode') != 1 or reference['service'].get('phase') != 'Running':
        raise RuntimeError('Requires existing isolated running Standard checkpoint')

    def exercise(out, token, duration):
        (out / 'reference.json').write_bytes(raw_ref)
        deadline = time.monotonic() + duration
        proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        started = False
        departed = False
        stages = []
        samples = []
        last_controls = None

        def tap(name, state):
            focus = device.adb('shell', 'dumpsys', 'window').stdout
            if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
                raise RuntimeError('Game lost focus; no input')
            x, y = BUTTONS[name]
            device.adb('shell', 'input', 'tap', str(x), str(y))
            with (out / 'inputs.jsonl').open('a') as file:
                file.write(json.dumps(dict(button=name, x=x, y=y, tick=state['service']['ticks'])) + '\n')

        while time.monotonic() < deadline:
            time.sleep(.6)
            if not device.exists(smoke.TELEMETRY):
                continue
            raw = device.adb('exec-out', 'cat', smoke.TELEMETRY).stdout
            try:
                state = json.loads(raw.decode('utf-8-sig'))
            except (ValueError, UnicodeError):
                continue
            service = state.get('service', {})
            if not started:
                log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
                start = log.rfind(token.encode())
                if start < 0 or proof not in log[start:]:
                    continue
                if b'S8_ASSET_REVIEW_RESUME isolated=1 recovered=1 blocked=0' not in log[start:]:
                    raise RuntimeError('Missing isolated recovery proof')
                checks = validate(state, reference, 'egmore')
                if not all(checks[k] for k in ('identity', 'fingerprint', 'service', 'paused', 'valid', 'generation')):
                    raise RuntimeError('Checkpoint did not restore exactly')
                if not all(checks.values()):
                    continue
                picture = device.screenshot()
                with Image.open(io.BytesIO(picture)) as png:
                    if png.size != (2800, 1260) or state['locomotive'].get('cabCamera') is not False:
                        raise RuntimeError('Unexpected UI layout')
                (out / 'before-resume.png').write_bytes(picture)
                tap('resume', state)
                started = True
                time.sleep(2)
                continue
            if service.get('mode') != 1 or state.get('content') != 'IR-S8-EGMORE-1':
                raise RuntimeError('Not Standard Egmore mode')
            candidate = dict(state)
            if service.get('phase') == 'Result':
                candidate['paused'] = False
            if not smoke.validate(candidate):
                raise RuntimeError('Lost readiness or paused unexpectedly')
            player = next(row['handling'] for row in state['logical']['trains'] if row['id'] == 2)
            stage = service['station']
            if stage not in stages:
                stages.append(stage)
                (out / f'phase-{stage}.json').write_bytes(raw)
                (out / f'phase-{stage}.png').write_bytes(device.screenshot())
            samples.append(dict(tick=service['ticks'], station=stage, head=player['headMetres'], speed=player['speedMps'],
                                power=player['throttle'], brake=player['brake']))
            (out / 'samples.json').write_text(json.dumps(samples, indent=2))
            (out / 'telemetry.json').write_bytes(raw)
            if service.get('phase') == 'Result':
                passed = service.get('success') is True and departed and service.get('dwellTicks') == 300
                (out / 'standard-itinerary.json').write_text(json.dumps(dict(passed=passed, stages=stages,
                    service=service, scope='scripted Standard UI inputs; reward not settled'), indent=2))
                if not passed:
                    raise RuntimeError('Standard itinerary failed')
                return len(samples)
            if stage == 'ReadyToDepart' and not service.get('departureAuthorized'):
                if not departed:
                    tap('depart', state)
                    departed = True
                continue
            remaining = (1000 if service.get('departureAuthorized') else 700) - player['headMetres']
            desired = controls(remaining, abs(player['speedMps']), stage == 'Dwelling' or
                               (stage == 'ReadyToDepart' and not service.get('departureAuthorized')))
            if desired != last_controls:
                power, brake = desired
                if power == 0:
                    tap('power0', state)
                tap({0:'release', .5:'brake50', 1:'brake100'}[brake], state)
                if power:
                    tap('power25', state)
                last_controls = desired
        raise RuntimeError('Standard itinerary deadline expired')

    smoke.FLAGS += ' -S8AssetReviewResume'
    args.map, args.duration, args.cab = smoke.WORLD, 600, False
    smoke.run(args, exercise=exercise)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    run(parser.parse_args())
