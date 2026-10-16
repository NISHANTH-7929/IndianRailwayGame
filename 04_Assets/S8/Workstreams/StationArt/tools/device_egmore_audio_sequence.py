"""Bounded audio regression using the previously inspected 2800x1260 cab layout."""
import argparse
import io
import json
import time
from PIL import Image
import device_egmore_service as smoke


def exercise(out, token, duration):
    device = smoke.device
    count = smoke.observe(out, token, 90)
    states = {}

    def snapshot(name):
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Game not foreground; no touch')
        picture = device.screenshot()
        with Image.open(io.BytesIO(picture)) as png:
            if png.size != (2800, 1260):
                raise RuntimeError('Previously inspected layout size changed; no touch')
        (out / (name + '.png')).write_bytes(picture)
        raw = device.adb('exec-out', 'cat', smoke.TELEMETRY).stdout
        state = json.loads(raw.decode('utf-8-sig'))
        if (state.get('content') != 'IR-S8-EGMORE-1' or state.get('blocked')
                or state['service']['phase'] != 'Running'
                or not state['logical']['valid'] or state['logical']['faulted']
                or not state['locomotive']['cabCamera']):
            raise RuntimeError('Expected running-service cab layout; no touch')
        states[name] = state
        (out / (name + '.json')).write_bytes(raw)
        return state

    # Coordinates grounded in controls1/steps0..3 and controls2/step0 captures.
    for name, paused, x, y in (
            ('before-horn', False, 190, 1180),
            ('after-horn', False, 880, 1180),
            ('paused', True, 1400, 537),
            ('resumed', False, 1570, 1180)):
        state = snapshot(name)
        if state['paused'] is not paused:
            raise RuntimeError('Unexpected pause state; no touch')
        device.adb('shell', 'input', 'tap', str(x), str(y))
        time.sleep(4)
    snapshot('after-brake')
    log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
    (out / 'engine.log').write_bytes(log)
    current = log.decode('utf-8', errors='replace').rsplit(token, 1)[-1]
    silent = current.find('RAIL_AUDIO_STATE running=0 rail=0 station=0 horn=0 brake=0')
    checks = dict(hornTriggered='RAIL_AUDIO horn' in current,
                  hornPlayback='horn=1' in current,
                  pausedSilent=silent >= 0,
                  resumedLoops=silent >= 0 and 'RAIL_AUDIO_STATE running=1 rail=1 station=1' in current[silent:],
                  brakeTriggered='RAIL_AUDIO brake engaged=1' in current,
                  brakePlayback='brake=1' in current,
                  pauseTelemetry=states['paused']['paused'] is True,
                  resumeTelemetry=states['resumed']['paused'] is False)
    (out / 'audio-sequence-checks.json').write_text(json.dumps(dict(
        passed=all(checks.values()), checks=checks,
        scope='observed-layout UI and engine playback state; not listening QA'), indent=2))
    if not all(checks.values()):
        raise RuntimeError('Audio sequence check failed')
    return count + len(states)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    smoke.FLAGS += ' -S8AudioReview'
    smoke.TELEMETRY = smoke.device.SAVED + '/S8/AudioReview/telemetry.json'
    args.map, args.duration, args.cab = smoke.WORLD, 150, True
    smoke.run(args, exercise=exercise)
