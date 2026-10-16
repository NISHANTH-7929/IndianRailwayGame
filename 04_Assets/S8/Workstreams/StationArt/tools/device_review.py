"""Exclusive phone-owner review runner; explicit APK identity and verified restoration.

Preflight is read-only. Capture operates only this game's command line and wake
setting, never clears app data, and refuses to overwrite existing evidence.
Static review captures do not constitute gameplay or performance acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time
import io
import shlex
from PIL import Image

ROOT = Path(__file__).resolve().parents[5]
PACKAGE = 'com.indianrailway.s1harness'
AUTHORIZED_SERIAL = '10BE1Y2EL40003J'
SERIAL = os.environ.get('RAIL_DEVICE_ENDPOINT', AUTHORIZED_SERIAL)
BASE = f'/sdcard/Android/data/{PACKAGE}/files/UnrealGame/MyProject'
REMOTE = BASE + '/UECommandLine.txt'
SAVED = BASE + '/MyProject/Saved'
SAVE_FAMILIES = ('S7', 'S8/LHB/Service', 'S8/Station/Service', 'S8/Egmore/Service')
ADB = str(Path(os.environ['ANDROID_HOME']) / 'platform-tools/adb.exe')


def adb(*args, check=True):
    return subprocess.run([ADB, '-s', SERIAL, *args], capture_output=True,
                          check=check, timeout=90)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def exists(path):
    result = adb('shell', f'if [ -f {shlex.quote(path)} ]; then echo EXISTS; else echo ABSENT; fi')
    marker = result.stdout.strip()
    if result.returncode != 0 or marker not in (b'EXISTS', b'ABSENT'):
        raise RuntimeError('Cannot determine remote file existence')
    return marker == b'EXISTS'


def screenshot():
    data = adb('exec-out', 'screencap', '-p').stdout
    with Image.open(io.BytesIO(data)) as picture:
        if picture.format != 'PNG' or min(picture.size) <= 0:
            raise RuntimeError('Invalid screenshot')
        picture.verify()
    return data


def backup_saves(out, state):
    for index, (remote, digest) in enumerate(state.items()):
        if digest is not None:
            data = adb('exec-out', 'cat', remote).stdout
            if sha(data) != digest:
                raise RuntimeError('Checkpoint changed during backup')
            (out / f'checkpoint-backup-{index}.irs').write_bytes(data)


def save_state():
    state = {}
    for family in SAVE_FAMILIES:
        for slot in range(2):
            path = f'{SAVED}/{family}/checkpoint-{slot}.irs'
            state[path] = (sha(adb('exec-out', 'cat', path).stdout)
                           if exists(path) else None)
    return state


def preflight():
    inventory = subprocess.run([ADB, 'devices'], capture_output=True,
                               text=True, check=True, timeout=30).stdout
    if not re.search(r'^' + re.escape(SERIAL) + r'\s+device$', inventory, re.M):
        raise RuntimeError('Selected phone endpoint unavailable')
    hardware_serial = adb('shell', 'getprop', 'ro.serialno').stdout.decode().strip()
    if hardware_serial != AUTHORIZED_SERIAL:
        raise RuntimeError('Selected endpoint is not the authorized phone')
    installed = adb('shell', 'pm', 'path', PACKAGE).stdout.decode().strip().splitlines()
    if len(installed) != 1 or not installed[0].startswith('package:'):
        raise RuntimeError('Expected one installed game base APK')
    apk_hash = adb('shell', 'sha256sum', installed[0][8:]).stdout.decode().split()[0]
    wake = adb('shell', 'settings', 'get', 'global', 'stay_on_while_plugged_in').stdout.strip()
    if wake not in tuple(str(i).encode() for i in range(8)):
        raise RuntimeError('Cannot preserve wake setting')
    return dict(serial=SERIAL, model=adb('shell', 'getprop', 'ro.product.model').stdout.decode().strip(),
                installedApkSha256=apk_hash, wake=wake.decode(), saves=save_state())


def capture(args, out, before, exercise=None):
    apk = Path(args.apk).resolve(strict=True)
    expected = sha(apk.read_bytes())
    if expected != before['installedApkSha256']:
        raise RuntimeError('Installed APK differs from explicitly selected artifact; no device mutation')
    cab_manual = (exercise is not None and args.map == '/Game/IndianRailway/S7/L_RailService'
                  and args.flags == '-S8AssetReview -S8Cab -S8CabManual')
    if not args.map.startswith('/Game/IndianRailway/S8/Review/') and not cab_manual:
        raise RuntimeError('Only isolated S8 review maps are allowed by this runner')
    old_exists = exists(REMOTE)
    original = adb('exec-out', 'cat', REMOTE).stdout if old_exists else None
    if old_exists:
        (out / 'original-commandline.txt').write_bytes(original)
    command = (f'-project="../../../MyProject/MyProject.uproject" {args.map} '
               f'{args.flags} -ExecCmds="r.SetFramePace 30" -log')
    (out / 'review-commandline.txt').write_text(command, encoding='utf-8')
    errors = []
    completed = False
    try:
        adb('shell', 'am', 'force-stop', PACKAGE)
        before['saves'] = save_state()
        (out / 'before-stopped.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
        backup_saves(out, before['saves'])
        log_path = SAVED + '/Logs/MyProject.log'
        previous_log = adb('exec-out', 'cat', log_path).stdout if exists(log_path) else b''
        adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', '3')
        adb('push', str(out / 'review-commandline.txt'), REMOTE)
        launch = adb('shell', 'am', 'start', '-W', '-n', PACKAGE + '/com.epicgames.unreal.SplashActivity')
        (out / 'launch.txt').write_bytes(launch.stdout)
        if exercise:
            exercise(out)
        for index in range(0 if exercise else 3):
            time.sleep(20)
            focus = adb('shell', 'dumpsys', 'window').stdout
            if not any(b'mCurrentFocus' in row and PACKAGE.encode() in row for row in focus.splitlines()):
                raise RuntimeError('Game is not foreground; screenshot intentionally not taken')
            (out / f'screen-{index}.png').write_bytes(screenshot())
            (out / f'memory-{index}.txt').write_bytes(adb('shell', 'dumpsys', 'meminfo', PACKAGE).stdout)
        log = adb('exec-out', 'cat', SAVED + '/Logs/MyProject.log').stdout
        (out / 'engine.log').write_bytes(log)
        fresh_log = log[len(previous_log):] if previous_log and log.startswith(previous_log) else log
        proof = ('LogWorld: Bringing World ' + args.map + '.' + args.map.rsplit('/', 1)[-1] + ' up for play').encode()
        if proof not in fresh_log:
            raise RuntimeError('Requested world has no fresh successful load proof; capture not accepted')
        completed = True
    finally:
        # Attempt every restoration step even when an earlier one fails.
        for name, operation in [
            ('stop', lambda: adb('shell', 'am', 'force-stop', PACKAGE)),
            ('commandline', lambda: adb('push', str(out / 'original-commandline.txt'), REMOTE)
             if old_exists else adb('shell', 'rm', '-f', REMOTE)),
            ('wake', lambda: adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', before['wake'])),
        ]:
            try:
                operation()
            except Exception as error:
                errors.append(f'{name}: {error}')
        result = dict(captureCompleted=completed, apk=str(apk), apkSha256=expected,
                      map=args.map, flags=args.flags, visualAccepted=False,
                      gameplayValidated=False, restorationErrors=errors)
        try:
            after = save_state()
            exists_after = exists(REMOTE)
            result['savesUnchanged'] = after == before['saves']
            result['commandlineRestored'] = (exists_after == old_exists and
                (not old_exists or adb('exec-out', 'cat', REMOTE).stdout == original))
            result['wakeRestored'] = adb('shell', 'settings', 'get', 'global', 'stay_on_while_plugged_in').stdout.decode().strip() == before['wake']
            result['artifactUnchanged'] = sha(apk.read_bytes()) == expected
        except Exception as error:
            errors.append(f'verification: {error}')
        (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        if errors or not all(result.get(k) for k in ('savesUnchanged', 'commandlineRestored', 'wakeRestored', 'artifactUnchanged')):
            raise RuntimeError('Restoration verification failed; inspect result.json before further phone work')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--label', required=True)
    parser.add_argument('--apk')
    parser.add_argument('--map')
    parser.add_argument('--flags', default='')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        parser.error('Label must be a safe unique evidence name')
    if bool(args.apk) != bool(args.map):
        parser.error('--apk and --map must be provided together')
    out = ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    before = preflight()
    (out / 'preflight.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    (out / 'battery.txt').write_bytes(adb('shell', 'dumpsys', 'battery').stdout)
    if args.apk:
        capture(args, out, before)
    print(out)


if __name__ == '__main__':
    main()
