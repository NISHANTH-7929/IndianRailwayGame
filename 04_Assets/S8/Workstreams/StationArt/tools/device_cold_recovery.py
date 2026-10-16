"""Owner-run, paused cold recovery; never resumes service or installs an APK.

RAIL_DEVICE_ENDPOINT is inherited through device_review, whose preflight checks
the hardware serial. Backups are retained if save preservation fails.
"""
import argparse
import json
from pathlib import Path
import time
import uuid
import device_review as device

CASES = (
    ('legacy', 'S7', '/Game/IndianRailway/S7/L_RailService', '',
     'LHB/Device/station-v5-legacy-result/telemetry.json'),
    ('lhb', 'S8/LHB/Service', '/Game/IndianRailway/S8/LHB/L_RailService_LHB', '-S8LHB',
     'LHB/Device/station-v5-lhb-result/telemetry.json'),
    ('station', 'S8/Station/Service', '/Game/IndianRailway/S8/Stations/L_RailService_Station', '-S8LHB -S8Station',
     'Station/Device/station-v6-cold-result/telemetry.json'),
)


def validate(state, reference, name):
    checks = dict(identity=state.get('content') == reference['content'],
                  fingerprint=state.get('logicalFingerprint') == reference['logicalFingerprint'],
                  service=state.get('service') == reference['service'],
                  paused=state.get('paused') is True, unblocked=state.get('blocked') is False,
                  valid=state.get('logical', {}).get('valid') is True and state.get('logical', {}).get('faulted') is False,
                  recreated=state.get('visualVehicles') == reference['visualVehicles'] and state.get('loadedLevels', 0) > 0)
    if name == 'station':
        checks.update(station_recreated=state.get('stationInstances') == 4216 and state.get('stationComponents') == 30,
                      lhb_recreated=state.get('lhbVehicles') == 8 and state.get('lhbComponents') == 14,
                      proxy_count_correct=state.get('proxyInstances') == 0)
    if name == 'egmore':
        scene = state.get('egmoreScene', {})
        signals = scene.get('signals', [])
        checks.update(
            generation=state.get('generation') == reference.get('generation'),
            egmore_identity=state.get('content') == 'IR-S8-EGMORE-1' and state.get('service', {}).get('serviceId') == 'S8-Egmore-01',
            egmore_ready=scene.get('ready') is True and scene.get('sceneRebuilt') is True and scene.get('trackExtension') is True and scene.get('modules') == 39 and scene.get('placements') == 2594,
            signals_ready=len(signals) == 2 and {s.get('id') for s in signals} == {1, 2} and all(s.get('aspect') in ('stop', 'proceed') for s in signals),
            locomotives_ready=state.get('locomotive', {}).get('ready') is True and state.get('trafficLocomotive', {}).get('ready') is True)
    return checks


def restore_file(remote, backup, original):
    if original is None:
        device.adb('shell', 'rm', '-f', remote)
    else:
        device.adb('push', str(backup), remote)
    present = device.exists(remote)
    if present != (original is not None) or (present and device.adb('exec-out', 'cat', remote).stdout != original):
        raise RuntimeError('Remote file restoration failed: ' + remote)


def run_case(case, reference, out, timeout=90):
    name, directory, world, flags, _ = case
    out.mkdir()
    remote = device.SAVED + '/' + directory + '/telemetry.json'
    device.adb('shell', 'am', 'force-stop', device.PACKAGE)
    original = device.adb('exec-out', 'cat', remote).stdout if device.exists(remote) else None
    backup = out / 'telemetry-before.json'
    if original is not None:
        backup.write_bytes(original)
    marker = uuid.uuid4().hex
    command = f'-project="../../../MyProject/MyProject.uproject" {world} {flags} -ColdRecoveryRun={marker} -ExecCmds="r.SetFramePace 30" -log'
    local = out / 'commandline.txt'
    local.write_text(command, encoding='utf-8')
    try:
        device.adb('shell', 'rm', '-f', remote)
        if device.exists(remote):
            raise RuntimeError('Cannot clear stale telemetry')
        device.adb('push', str(local), device.REMOTE)
        launch = device.adb('shell', 'am', 'start', '-W', '-n', device.PACKAGE + '/com.epicgames.unreal.SplashActivity')
        (out / 'launch.txt').write_bytes(launch.stdout)
        deadline = time.monotonic() + timeout
        ready_samples = 0
        while time.monotonic() < deadline:
            time.sleep(2)
            if not device.exists(remote):
                continue
            raw = device.adb('exec-out', 'cat', remote).stdout
            try:
                state = json.loads(raw.decode('utf-8-sig'))
            except (ValueError, UnicodeError):
                continue
            log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
            proof = f'LogWorld: Bringing World {world}.{world.rsplit("/", 1)[-1]} up for play'.encode()
            marker_pos = log.rfind(marker.encode())
            if marker_pos < 0 or proof not in log[marker_pos:]:
                continue
            if name == 'egmore' and b'S8_ASSET_REVIEW_RESUME isolated=1 recovered=1 blocked=0' not in log[marker_pos:]:
                raise RuntimeError('Current APK did not prove isolated Egmore resume')
            checks = validate(state, reference, name)
            (out / 'telemetry.json').write_bytes(raw)
            (out / 'engine.log').write_bytes(log)
            (out / 'checks.json').write_text(json.dumps(checks, indent=2), encoding='utf-8')
            if not all(checks.values()):
                ready_samples = 0
                # Logical recovery precedes asynchronous level/presentation load.
                # Keep every readiness sample; wait only for presentation, never
                # excuse wrong saved state or a resumed/faulted simulation.
                core = ('identity', 'fingerprint', 'service', 'paused', 'valid') + (('generation', 'egmore_identity') if name == 'egmore' else ())
                if all(checks[key] for key in core):
                    with (out / 'readiness.jsonl').open('a', encoding='utf-8') as history:
                        history.write(json.dumps(dict(checks=checks, state=state)) + '\n')
                    continue
                raise RuntimeError(f'{name} recovery failed: {checks}')
            ready_samples += 1
            if name == 'egmore' and ready_samples < 2:
                continue
            focus = device.adb('shell', 'dumpsys', 'window').stdout
            if not any(b'mCurrentFocus' in row and device.PACKAGE.encode() in row for row in focus.splitlines()):
                raise RuntimeError('Recovery game is not foreground')
            (out / 'screen.png').write_bytes(device.screenshot())
            return dict(passed=True, checks=checks, fingerprint=state['logicalFingerprint'], reference=case[4])
        raise RuntimeError('No fresh telemetry and current-run successful world-load proof')
    finally:
        try:
            device.adb('shell', 'am', 'force-stop', device.PACKAGE)
        finally:
            restore_file(remote, backup, original)


def run(args, cases=CASES, references=None):
    if not device.re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        raise ValueError('Invalid unique evidence label')
    apk = Path(args.apk).resolve(strict=True)
    digest = device.sha(apk.read_bytes())
    if references is None:
        references = [json.loads((device.ROOT / '07_Documentation/S8/Evidence' / case[4]).read_text(encoding='utf-8-sig')) for case in cases]
    if len(references) != len(cases):
        raise ValueError('Every recovery case requires one reference')
    before = device.preflight()
    if before['installedApkSha256'] != digest:
        raise RuntimeError('Installed APK differs from selected artifact; no mutation')
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    # No override/wake writes until stop, snapshots and backups have succeeded.
    device.adb('shell', 'am', 'force-stop', device.PACKAGE)
    before['saves'] = device.save_state()
    required = [f'{device.SAVED}/{case[1]}/checkpoint-{slot}.irs'
                for case in cases for slot in range(2)]
    if any(before['saves'].get(path) is None for path in required):
        raise RuntimeError('All six existing checkpoints are required')
    device.backup_saves(out, before['saves'])
    original = device.adb('exec-out', 'cat', device.REMOTE).stdout if device.exists(device.REMOTE) else None
    backup = out / 'original-commandline.txt'
    if original is not None:
        backup.write_bytes(original)
    (out / 'before.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    result = dict(passed=False, apk=str(apk), apkSha256=digest, cases={}, restorationErrors=[])
    try:
        device.adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', '3')
        for case, reference in zip(cases, references):
            result['cases'][case[0]] = run_case(case, reference, out / case[0])
            if device.save_state() != before['saves']:
                raise RuntimeError('Checkpoint changed; stop testing and inspect backups')
        result['passed'] = True
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        operations = (
            ('stop', lambda: device.adb('shell', 'am', 'force-stop', device.PACKAGE)),
            ('commandline', lambda: restore_file(device.REMOTE, backup, original)),
            ('wake', lambda: device.adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', before['wake'])),
        )
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
        restored = not result['restorationErrors'] and all(result.get(key) for key in ('stopRestored', 'commandlineRestored', 'wakeRestored', 'savesUnchanged', 'artifactUnchanged', 'installedApkUnchanged'))
        result['passed'] = result['passed'] and restored
        (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        if not restored:
            raise RuntimeError('Preservation verification failed; inspect result.json and backups')
    print(out)
    return out


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    run(parser.parse_args())
