"""Bounded isolated Egmore cab checks; parent exclusively owns phone execution.

--layout-proof must be a parent-inspected screenshot of this build's new dock.
This runner records both readouts for human comparison; it does not claim OCR
or visual acceptance from telemetry. Normal saves/settings use the existing
eight-slot preservation runner. Never taps Save/Exit or changes content mode.
"""
import argparse
import io
import json
import math
from pathlib import Path
import subprocess
import time
from PIL import Image
import device_egmore_service as smoke


# Centers of the authored Egmore dock anchors in RailServiceWidget.cpp.
DOCK = {
    'Power0': (.040, .8977), 'Power25': (.110, .8977),
    'Power50': (.180, .8977), 'Brake0': (.0515, .9623),
    'Brake50': (.1445, .9623), 'Brake100': (.238, .9623),
    'Reverse': (.3165, .8977), 'Neutral': (.3595, .8977),
    'Forward': (.403, .8977), 'Emergency': (.935, .8977),
    'ResetEmergency': (.935, .9623), 'CabLamp': (.47665, .9623),
    'Instruments': (.560, .9623), 'CabCamera': (.8225, .8977),
}
SIZE = (2800, 1260)


def verify_picture(data):
    with Image.open(io.BytesIO(data)) as picture:
        if picture.format != 'PNG' or picture.size != SIZE:
            raise RuntimeError('Expected parent-inspected 2800x1260 PNG layout')
        picture.verify()


def player(state):
    return next(row['handling'] for row in state['logical']['trains'] if row['id'] == 2)


def exercise(out, token, duration, proof, physical_controls=False):
    device = smoke.device
    deadline = time.monotonic() + min(duration, 540)
    checks, states, actions = {}, {}, []
    previous_tick = -1
    serial = 0
    expected_cab = True
    error = None
    (out / 'parent-inspected-layout.png').write_bytes(proof)
    world_proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()

    def adb(*args, check=True):
        # Exercise I/O cannot consume the runner's full 90-second per-call
        # timeout repeatedly. Restoration still uses the established runner.
        remaining()
        return subprocess.run([device.ADB, '-s', device.SERIAL, *args],
                              capture_output=True, check=check,
                              timeout=min(20, max(.1, deadline - time.monotonic())))

    def remaining():
        if time.monotonic() >= deadline:
            raise RuntimeError('Bounded stabilization deadline reached')

    def check(name, value):
        checks[name] = bool(value)

    def fresh(name, startup=False):
        nonlocal previous_tick, serial
        stop = min(deadline, time.monotonic() + (90 if startup else 15))
        while time.monotonic() < stop:
            remaining()
            # The preservation runner removes stale telemetry before launch.
            # Missing/not-yet-created files are an ordinary bounded startup wait.
            raw = adb('exec-out', 'cat', smoke.TELEMETRY, check=False).stdout
            try:
                state = json.loads(raw.decode('utf-8-sig'))
            except (ValueError, UnicodeError):
                time.sleep(1)
                continue
            log = adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log', check=False).stdout
            start = log.rfind(token.encode())
            if start < 0 or world_proof not in log[start:]:
                time.sleep(1)
                continue
            ready = smoke.validate(state)
            tick = state.get('logical', {}).get('tick', -1)
            if not ready or state['service']['phase'] != 'Running':
                if startup:
                    time.sleep(1)
                    continue
                raise RuntimeError('Service not fresh, logical-valid, unpaused and Running; no touch')
            if tick <= previous_tick:
                time.sleep(1)
                continue
            focus = adb('shell', 'dumpsys', 'window').stdout
            if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
                raise RuntimeError('Game not foreground; no touch')
            if state['locomotive']['cabCamera'] is not expected_cab:
                raise RuntimeError('Unexpected cab/outside layout; no touch')
            picture = adb('exec-out', 'screencap', '-p').stdout
            verify_picture(picture)
            remaining()
            stem = f'{serial:03d}-{name}'
            serial += 1
            (out / (stem + '.png')).write_bytes(picture)
            (out / (stem + '.json')).write_bytes(raw)
            (out / 'engine.log').write_bytes(log)
            states[stem] = state
            previous_tick = tick
            return state
        raise RuntimeError('Timed out awaiting fresh current-run state; no touch')

    def pose_checks(name, state):
        loco = state['locomotive']
        h = player(state)
        check(name + '-applied-brake-authoritative', abs(loco['appliedBrake'] - h['appliedBrake']) < .00001)
        check(name + '-applied-power-authoritative', abs(loco['appliedPower'] - h['appliedThrottle']) < .00001)
        check(name + '-emergency-authoritative', loco['emergency'] == h['emergency'])
        poses = {row['role']: row['rotation'] for row in loco['controlPoses']}
        # Unreal's displayed Pitch has the opposite sign to the authored +Y axis.
        expected = {
            'throttle': (24 - 48 * loco['commandThrottle'], 0, 0),
            'train_brake': (0, -30 + 70 * loco['commandBrake'], 0),
            'reverser': (-18 * loco['direction'], 0, 0),
            'gauge_speed': (0, 0, -130 + 260 * min(abs(h['speedMps']) / 50, 1)),
        }
        for role, rotation in expected.items():
            actual = poses.get(role, [])
            check(name + '-' + role + '-pose', len(actual) == 3 and
                  all(abs(a - b) < .06 for a, b in zip(actual, rotation)))

    def tap(control, name, wait=2):
        nonlocal expected_cab
        before = fresh('before-' + name)
        if control in ('Reverse', 'Neutral', 'Forward', 'ResetEmergency'):
            h = player(before)
            if abs(h['speedMps']) >= .02 or h['throttle'] != 0:
                raise RuntimeError('Direction/reset requires stopped zero-throttle state; no touch')
        remaining()
        x, y = DOCK[control]
        adb('shell', 'input', 'tap', str(round(x * SIZE[0])), str(round(y * SIZE[1])))
        actions.append(dict(control=control, name=name, tick=previous_tick))
        if control == 'CabCamera':
            expected_cab = not expected_cab
        time.sleep(wait)
        after = fresh('after-' + name)
        pose_checks(name, after)
        return after

    def stop_train(name):
        tap('Power0', name + '-power-zero')
        state = tap('Brake100', name + '-brake-full')
        stop = min(deadline, time.monotonic() + 45)
        while abs(player(state)['speedMps']) >= .02 and time.monotonic() < stop:
            time.sleep(2)
            state = fresh(name + '-decelerating')
        stopped = abs(player(state)['speedMps']) < .02
        check(name + '-stopped-below-0.05mps', stopped)
        if not stopped:
            raise RuntimeError('Train did not stop within bounded wait')
        return state

    def physical(role, name):
        before = fresh('before-' + name)
        loco = before['locomotive']
        h = player(before)
        if (not loco['cabCamera'] or loco['instrumentView']
                or abs(h['speedMps']) >= .02 or h['throttle'] != 0):
            raise RuntimeError('Physical controls require normal cab, standstill and zero applied command throttle')
        viewport = loco.get('viewport', [])
        if (len(viewport) != 2 or any(not isinstance(v, (int, float)) or
                not math.isfinite(v) or v <= 0 for v in viewport)):
            raise RuntimeError('Invalid current viewport; no physical touch')
        matches = [p for p in loco.get('controlPoses', []) if p.get('role') == role]
        if len(matches) != 1:
            raise RuntimeError('Missing or ambiguous physical-control screen point; no touch')
        point = matches[0].get('screen', [])
        if (len(point) != 2 or any(not isinstance(v, (int, float)) or
                not math.isfinite(v) for v in point)):
            raise RuntimeError('Invalid physical-control projection; no touch')
        nx, ny = point[0] / viewport[0], point[1] / viewport[1]
        # Only the inspected unobstructed normal-cab region. Never guess an
        # offscreen control position or touch through the upper HUD/bottom dock.
        if not (0 < nx < 1 and .18 <= ny < .86):
            raise RuntimeError('Projected control outside guarded cab region; no touch')
        x, y = round(nx * SIZE[0]), round(ny * SIZE[1])
        remaining()
        adb('shell', 'input', 'swipe', str(x), str(y), str(x), str(y), '160')
        actions.append(dict(control=role, name=name, physical=True,
                            viewport=viewport, projectedPoint=point,
                            touchPoint=[x, y], tick=previous_tick))
        time.sleep(2)
        after = fresh('after-' + name)
        interacted = after['locomotive']['interactions'] == loco['interactions'] + 1
        check(name + '-interaction-count', interacted)
        pose_checks(name, after)
        if not interacted:
            raise RuntimeError('Physical action did not produce exactly one cab interaction')
        return before, after

    def physical_sequence():
        before, after = physical('throttle', 'physical-throttle')
        changed = abs(before['locomotive']['commandThrottle']) < .001 and abs(after['locomotive']['commandThrottle'] - .25) < .001
        held = after['locomotive']['commandBrake'] >= .999 and abs(player(after)['speedMps']) < .02
        check('physical-throttle-requested-quarter', changed)
        check('physical-throttle-full-brake-held', held)
        # Always attempt the guarded zero-power dock action after this exercise.
        restored = tap('Power0', 'physical-throttle-restore')
        check('physical-throttle-restored-zero', restored['locomotive']['commandThrottle'] == 0)
        if not changed or not held or restored['locomotive']['commandThrottle'] != 0:
            raise RuntimeError('Physical throttle check failed; no further physical actions')
        before, after = physical('train_brake', 'physical-brake')
        cycled = before['locomotive']['commandBrake'] >= .999 and after['locomotive']['commandBrake'] == 0
        check('physical-brake-cycles-full-to-release', cycled)
        restored = tap('Brake100', 'physical-brake-restore')
        check('physical-brake-restored-full', restored['locomotive']['commandBrake'] >= .999)
        if not cycled or restored['locomotive']['commandBrake'] < .999:
            raise RuntimeError('Physical brake check failed; no further physical actions')
        before, after = physical('switch_emergency', 'physical-emergency')
        latched = not player(before)['emergency'] and player(after)['emergency'] and player(after)['appliedBrake'] >= .999
        check('physical-emergency-latched-full-brake', latched)
        after = tap('ResetEmergency', 'physical-emergency-reset-at-zero')
        check('physical-emergency-reset', not player(after)['emergency'])
        check('physical-sequence-no-movement', abs(player(after)['speedMps']) < .02)
        if not latched or player(after)['emergency']:
            raise RuntimeError('Physical emergency check failed')

    try:
        initial = fresh('initial-cab', startup=True)
        check('initial-expected-cab', initial['locomotive']['cabCamera'])
        stop_train('initial-stop')
        tap('Emergency', 'emergency-engage', wait=4)
        # Reproduce the reported discrepancy: requested brake zero while the
        # emergency latch still commands full actual braking.
        state = tap('Brake0', 'emergency-both-readouts', wait=3)
        check('emergency-requested-brake-zero', state['locomotive']['commandBrake'] == 0)
        check('emergency-latched', player(state)['emergency'] is True)
        check('emergency-full-applied-brake', player(state)['appliedBrake'] >= .999)
        check('emergency-power-cutout', player(state)['appliedThrottle'] <= .00001)
        state = tap('ResetEmergency', 'reset-at-zero', wait=3)
        check('emergency-reset', player(state)['emergency'] is False)
        for control, requested in (('Brake50', .5), ('Brake100', 1), ('Brake0', 0)):
            state = tap(control, control.lower(), wait=4)
            check(control + '-requested', abs(state['locomotive']['commandBrake'] - requested) < .001)
        tap('Brake100', 'hold-for-reverser')
        for index, (control, direction) in enumerate((('Neutral', 0), ('Forward', 1), ('Reverse', -1), ('Forward', 1))):
            name = f'direction-{index}-{control.lower()}'
            state = tap(control, name)
            # A rejection is recorded as failed, never silently accepted as success.
            check(name, player(state)['direction'] == direction)
        if player(state)['direction'] != 1:
            raise RuntimeError('Forward direction not established; no traction test')
        tap('Brake0', 'release-for-throttle', wait=4)
        for control, requested in (('Power25', .25), ('Power50', .5), ('Power0', 0)):
            state = tap(control, control.lower(), wait=5)
            check(control + '-requested', abs(state['locomotive']['commandThrottle'] - requested) < .001)
        state = stop_train('final-stop')
        if physical_controls:
            physical_sequence()
            state = fresh('physical-controls-complete')
        light_before = state['locomotive']['cabLight']
        state = tap('CabLamp', 'cab-light-toggle')
        check('cab-light-toggle', state['locomotive']['cabLight'] is not light_before)
        check('cab-light-visible-matches-selection', state['locomotive']['cabLightVisible'] == state['locomotive']['cabLight'])
        state = tap('CabLamp', 'cab-light-restore')
        check('cab-light-restored', state['locomotive']['cabLight'] == light_before)
        normal_fov = state['locomotive']['cameraFov']
        state = tap('Instruments', 'label-view')
        check('label-zoom-active', state['locomotive']['instrumentView'] and abs(state['locomotive']['cameraFov'] - 55) < .01)
        state = tap('CabCamera', 'outside-resets-zoom')
        check('outside-camera', not state['locomotive']['cabCamera'])
        check('outside-cab-light-hidden', not state['locomotive']['cabLightVisible'])
        check('outside-zoom-reset', not state['locomotive']['instrumentView'] and abs(state['locomotive']['cameraFov'] - normal_fov) < .01)
        state = tap('CabCamera', 'return-driver-view')
        check('cab-return', state['locomotive']['cabCamera'] and not state['locomotive']['instrumentView'])
        check('final-stopped', abs(player(state)['speedMps']) < .05)
        check('nonzero-speed-gauge-exercised', any(s['locomotive']['speedKmh'] > .1 for s in states.values()))
        check('sequence-completed', True)
    except Exception as exc:
        error = str(exc)
        check('sequence-completed', False)
        raise
    finally:
        (out / 'cab-stabilization-checks.json').write_text(json.dumps(dict(
            automatedPassed=bool(checks) and all(checks.values()) and error is None,
            checks=checks, error=error, actions=actions, screenshotCount=len(states),
            physicalControlsRequested=physical_controls,
            layoutProofSha256=device.sha(proof),
            visualReadoutAgreementVerified=None,
            requiredHumanReview=['Compare emergency-both-readouts PNG: upper HUD Brake 100%/EMERGENCY and cab B100/EMG',
                                 'Inspect label-view, normal dashboard, side-window framing and icon legibility'],
            limitations=['No OCR: paired screenshots do not automatically prove rendered text agreement',
                         ('Physical 3D ray-touch requested; inspect individual checks for completion' if physical_controls
                          else 'Physical 3D ray-touch not requested; only dock controls tested'),
                         'Free camera swipe not tested',
                         'Reverse/neutral rejection is a failed check to investigate, not a false pass',
                         'Applied power can be limited by preserved assistance/authority logic',
                         '540-second exercise deadline and capped I/O exclude runner preflight/restoration'],
        ), indent=2), encoding='utf-8')
    if not all(checks.values()):
        raise RuntimeError('Cab stabilization checks failed; inspect JSON/PNG evidence')
    return len(states)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    parser.add_argument('--layout-proof', required=True,
                        help='Parent-inspected new-dock screenshot for the selected APK')
    parser.add_argument('--physical-controls', action='store_true',
                        help='Enable only after parent inspects normal-cab screenshot and projected control points')
    args = parser.parse_args()
    proof = Path(args.layout_proof).resolve(strict=True).read_bytes()
    verify_picture(proof)  # Fail before phone preflight or mutation on wrong layout size.
    smoke.FLAGS += ' -S8AudioReview'
    smoke.TELEMETRY = smoke.device.SAVED + '/S8/AudioReview/telemetry.json'
    args.map, args.duration, args.cab = smoke.WORLD, 540, True
    smoke.run(args, exercise=lambda out, token, duration: exercise(out, token, duration, proof, args.physical_controls))


if __name__ == '__main__':
    main()
