"""Egmore combined cab/audio readiness capture; playback state, not listening QA."""
import argparse
import json
import re
import device_egmore_service as smoke


def exercise(out, token, duration):
    count = smoke.observe(out, token, duration)
    log = (out / 'engine.log').read_text(encoding='utf-8', errors='replace')
    log = log[log.rfind(token):]
    expected = ('SW_Rail.SW_Rail', 'SW_Station.SW_Station', 'SW_Horn.SW_Horn',
                'SW_Brake.SW_Brake', 'SW_StationCue.SW_StationCue')
    states = re.findall(r'RAIL_AUDIO_STATE running=(\d) rail=(\d) station=(\d) horn=(\d) brake=(\d)', log)
    checks = dict(allSoundsLoaded=all(any('RAIL_AUDIO loaded ' in line and name in line
                                      for line in log.splitlines()) for name in expected),
                  noMissingSounds='RAIL_AUDIO missing ' not in log,
                  runningRail=any(row[0:2] == ('1', '1') for row in states),
                  runningStation=any(row[0] == '1' and row[2] == '1' for row in states))
    (out / 'audio-checks.json').write_text(json.dumps(dict(passed=all(checks.values()), checks=checks,
        stateSamples=len(states), scope='packaged asset loading and reported loop playback; not audible quality or horn/brake interaction acceptance'), indent=2))
    if not all(checks.values()):
        raise RuntimeError('Packaged audio smoke failed')
    return count


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    smoke.FLAGS += ' -S8AudioReview'
    smoke.TELEMETRY = smoke.device.SAVED + '/S8/AudioReview/telemetry.json'
    args.map, args.duration, args.cab = smoke.WORLD, 90, True
    smoke.run(args, exercise=exercise)
