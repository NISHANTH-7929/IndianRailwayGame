"""Isolated Egmore service smoke, not full service or performance acceptance."""
import argparse
import json
from pathlib import Path
import time
import uuid
import device_review as device
from device_cold_recovery import restore_file

WORLD = '/Game/IndianRailway/S8/Egmore/L_RailService_Egmore'
TELEMETRY = device.SAVED + '/S8/Egmore/AssetReview/telemetry.json'
FLAGS = '-S8Egmore -S8Station -S8LHB -S8AssetReview'


def validate(state):
    if (state.get('content') != 'IR-S8-EGMORE-1'
            or state.get('service', {}).get('serviceId') != 'S8-Egmore-01'):
        raise RuntimeError('Unexpected Egmore service identity')
    logical = state.get('logical', {})
    if logical.get('valid') is not True or logical.get('faulted') is not False:
        raise RuntimeError('Invalid or faulted Egmore logical state')
    scene = state.get('egmoreScene', {})
    signals = scene.get('signals', [])
    scene_ready = (scene.get('ready') is True and scene.get('sceneRebuilt') is True
                   and scene.get('trackExtension') is True
                   and scene.get('modules') == 39 and scene.get('placements') == 2594
                   and len(signals) == 2 and {row.get('id') for row in signals} == {1, 2}
                   and all(row.get('aspect') in ('stop', 'proceed') for row in signals))
    return (state.get('blocked') is False and state.get('paused') is False
            and logical.get('tick', 0) > 0
            and state.get('logicalFingerprint') not in (None, '', 'INVALID')
            and state.get('loadedLevels', 0) > 0
            and scene_ready and state.get('trafficLocomotive', {}).get('ready') is True
            and state.get('locomotive', {}).get('ready') is True)


def observe(out, token, duration):
    deadline = time.monotonic() + duration
    ready_samples = 0
    last_ready = False
    samples = []
    proof = f'LogWorld: Bringing World {WORLD}.L_RailService_Egmore up for play'.encode()
    while time.monotonic() < deadline:
        time.sleep(3)
        if not device.exists(TELEMETRY):
            continue
        raw = device.adb('exec-out', 'cat', TELEMETRY).stdout
        try:
            state = json.loads(raw.decode('utf-8-sig'))
        except (ValueError, UnicodeError):
            continue
        log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
        start = log.rfind(token.encode())
        if start < 0 or proof not in log[start:]:
            continue
        last_ready = validate(state)
        samples.append(dict(state=state, ready=last_ready))
        (out / 'samples.json').write_text(json.dumps(samples, indent=2), encoding='utf-8')
        (out / 'telemetry.json').write_bytes(raw)
        (out / 'engine.log').write_bytes(log)
        if not last_ready:
            continue
        focus = device.adb('shell', 'dumpsys', 'window').stdout
        if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
            raise RuntimeError('Egmore game is not foreground')
        # Keep screenshot volume bounded while preserving periodic visual proof.
        if ready_samples % 5 == 0:
            (out / f'screen-{ready_samples:03d}.png').write_bytes(device.screenshot())
        ready_samples += 1
    if not ready_samples or not last_ready:
        raise RuntimeError('No fresh current-run ready Egmore smoke evidence at completion')
    return ready_samples


def run(args, exercise=None):
    if args.map != WORLD:
        raise ValueError('Only isolated Egmore service map is allowed')
    if not 60 <= args.duration <= (600 if exercise else 180):
        raise ValueError('Duration must be 60..180 seconds')
    if not device.re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        raise ValueError('Invalid unique evidence label')
    apk = Path(args.apk).resolve(strict=True)
    digest = device.sha(apk.read_bytes())
    before = device.preflight()
    if before['installedApkSha256'] != digest:
        raise RuntimeError('Installed APK differs from selected artifact; no mutation')
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    device.adb('shell', 'am', 'force-stop', device.PACKAGE)
    before['saves'] = device.save_state()
    expected_paths = {f'{device.SAVED}/{family}/checkpoint-{slot}.irs'
                      for family in ('S7', 'S8/LHB/Service', 'S8/Station/Service', 'S8/Egmore/Service')
                      for slot in range(2)}
    if set(before['saves']) != expected_paths:
        raise RuntimeError('All eight normal save slots must be inventoried')
    device.backup_saves(out, before['saves'])
    originals = []
    for remote, name in ((device.REMOTE, 'original-commandline.txt'), (TELEMETRY, 'original-telemetry.json')):
        original = device.adb('exec-out', 'cat', remote).stdout if device.exists(remote) else None
        backup = out / name
        if original is not None:
            backup.write_bytes(original)
        originals.append((remote, backup, original))
    (out / 'before.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    token = uuid.uuid4().hex
    flags = FLAGS + (' -S8Cab' if args.cab else '')
    command = f'-project="../../../MyProject/MyProject.uproject" {WORLD} {flags} -EgmoreSmokeRun={token} -ExecCmds="r.SetFramePace 30" -log'
    command_path = out / 'smoke-commandline.txt'
    command_path.write_text(command, encoding='utf-8')
    result = dict(smokeCompleted=False, gameplayValidated=False, serviceCompletionValidated=False,
                  apk=str(apk), apkSha256=digest, map=WORLD, flags=flags,
                  durationSeconds=args.duration, restorationErrors=[])
    try:
        device.adb('shell', 'rm', '-f', TELEMETRY)
        if device.exists(TELEMETRY):
            raise RuntimeError('Unable to remove stale review telemetry')
        device.adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', '3')
        device.adb('push', str(command_path), device.REMOTE)
        launch = device.adb('shell', 'am', 'start', '-W', '-n', device.PACKAGE + '/com.epicgames.unreal.SplashActivity')
        (out / 'launch.txt').write_bytes(launch.stdout)
        result['readySamples'] = (exercise or observe)(out, token, args.duration)
        result['smokeCompleted'] = True
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        operations = [('stop', lambda: device.adb('shell', 'am', 'force-stop', device.PACKAGE))]
        operations += [(name, lambda item=item: restore_file(*item))
                       for name, item in zip(('commandline', 'telemetry'), originals)]
        operations.append(('wake', lambda: device.adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', before['wake'])))
        for name, operation in operations:
            try:
                operation()
                result[name + 'Restored'] = True
            except Exception as error:
                result['restorationErrors'].append(f'{name}: {error}')
        try:
            result['savesUnchanged'] = device.save_state() == before['saves']
            result['wakeRestored'] = device.adb('shell', 'settings', 'get', 'global', 'stay_on_while_plugged_in').stdout.decode().strip() == before['wake']
            result['artifactUnchanged'] = device.sha(apk.read_bytes()) == digest
            result['installedApkUnchanged'] = device.preflight()['installedApkSha256'] == digest
        except Exception as error:
            result['restorationErrors'].append('verification: ' + str(error))
        restored = not result['restorationErrors'] and all(result.get(key) for key in ('stopRestored', 'commandlineRestored', 'telemetryRestored', 'wakeRestored', 'savesUnchanged', 'artifactUnchanged', 'installedApkUnchanged'))
        result['smokeCompleted'] = result['smokeCompleted'] and restored
        (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        if not restored:
            raise RuntimeError('Preservation verification failed; inspect result.json and backups')
    print(out)
    return out


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    parser.add_argument('--map', default=WORLD)
    parser.add_argument('--duration', type=int, default=60)
    parser.add_argument('--cab', action='store_true')
    run(parser.parse_args())
