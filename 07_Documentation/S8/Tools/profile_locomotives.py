"""Capture the real packaged locomotive review workload on the connected iQOO.

Uses the game's development-only isolated review service and preserves the normal
S7 checkpoint hashes and any commandline override. No network settings are changed.
Run after packaging/installing the matching APK. Failed evidence is retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[3]
PACKAGE = 'com.indianrailway.s1harness'
BASE = f'/sdcard/Android/data/{PACKAGE}/files/UnrealGame/MyProject'
SAVED = BASE + '/MyProject/Saved'
REMOTE = BASE + '/UECommandLine.txt'


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def run_capture(args):
    adb_path = Path(os.environ.get('ANDROID_HOME', '')) / 'platform-tools/adb.exe'
    if not adb_path.is_file():
        raise RuntimeError('ANDROID_HOME/platform-tools/adb.exe is unavailable')
    inventory = subprocess.run([str(adb_path), 'devices'], capture_output=True, text=True, check=True).stdout
    devices = [line.split()[0] for line in inventory.splitlines()
               if '10BE1Y2EL40003J' in line and line.endswith('\tdevice')]
    if '10BE1Y2EL40003J' in devices: devices=['10BE1Y2EL40003J']
    if len(devices) != 1:
        raise RuntimeError(f'Expected one authorized iQOO endpoint, found {len(devices)}')
    serial = devices[0]
    out = ROOT / '07_Documentation/S8/Evidence/Locomotives/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)

    def adb(*values, check=True, binary=False):
        began = time.time()
        result = subprocess.run([str(adb_path), '-s', serial, *values], capture_output=True,
                                text=not binary, timeout=60)
        with (out / 'commands.jsonl').open('a', encoding='utf-8') as log:
            log.write(json.dumps({'utcEpoch': began, 'arguments': values, 'exitCode': result.returncode}) + '\n')
        if check and result.returncode:
            raise RuntimeError(f'ADB {values} failed: {result.stderr}')
        return result

    def normal_hashes():
        result = adb('shell', 'sha256sum', SAVED + '/S7/checkpoint-0.irs', SAVED + '/S7/checkpoint-1.irs')
        return result.stdout.strip()

    model = adb('shell', 'getprop', 'ro.product.model').stdout.strip()
    api = adb('shell', 'getprop', 'ro.build.version.sdk').stdout.strip()
    if model != 'I2304' or api != '36':
        raise RuntimeError(f'Unexpected device: {model}, API {api}')
    # Review launch has its own save directory; the ordinary game is not resumed.
    adb('shell', 'am', 'force-stop', PACKAGE)
    original = adb('shell', 'cat', REMOTE, check=False)
    existed = original.returncode == 0
    if existed:
        (out / 'original-commandline.txt').write_text(original.stdout, encoding='utf-8')
    hashes_before = normal_hashes()
    (out / 'normal-checkpoints-before.txt').write_text(hashes_before, encoding='utf-8')
    old_csv = set(adb('shell', 'ls', SAVED + '/Profiling/CSV', check=False).stdout.splitlines())
    flags = '-S8DisableLocomotive' if args.variant == 'baseline' else '-S8LocoClass=' + args.variant
    command = (f'-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S7/L_RailService '
               f'-S8AssetReview {flags} {args.extra} -csvCaptureFrames={args.frames} -csvGpuStats '
               '-csvCompression=0 -ExecCmds="r.SetFramePace 30" -log')
    (out / 'review-commandline.txt').write_text(command, encoding='utf-8')
    apk = Path(args.apk) if args.apk else ROOT / '08_Builds/S8_Android/Android_ASTC/MyProject-arm64.apk'
    write(out / 'context.json', {'variant': args.variant, 'serial': serial, 'model': model, 'api': api,
                               'frames': args.frames, 'scope': 'isolated locomotive asset review; not full S8 workload',
                               'apkSha256': hashlib.sha256(apk.read_bytes()).hexdigest(), 'apkBytes': apk.stat().st_size})
    changed = False
    samples, pss, thermals, residency, latency = [], [], [], [], []
    try:
        adb('push', str(out / 'review-commandline.txt'), REMOTE)
        changed = True
        launch = adb('shell', 'am', 'start', '-W', '-n', PACKAGE + '/com.epicgames.unreal.SplashActivity')
        (out / 'launch.txt').write_text(launch.stdout, encoding='utf-8')
        start = time.monotonic()
        # 30 FPS cap + startup/export allowance. Never call a partial capture complete.
        duration = args.frames / 30 + 30
        while time.monotonic() - start < duration:
            time.sleep(15)
            elapsed = time.monotonic() - start
            focus = adb('shell', 'dumpsys', 'window').stdout
            if not re.search(r'mCurrentFocus.*com\.indianrailway\.s1harness', focus):
                (out / 'focus-failure.txt').write_text(focus, encoding='utf-8')
                raise RuntimeError('Game left foreground; capture is excluded')
            state_raw = adb('shell', 'cat', SAVED + '/S8/AssetReview/telemetry.json').stdout
            state = json.loads(state_raw)
            if not state.get('logical', {}).get('valid') or state['logical'].get('faulted'):
                raise RuntimeError('Review logical state invalid/faulted')
            memory = adb('shell', 'dumpsys', 'meminfo', PACKAGE).stdout
            thermal = adb('shell', 'dumpsys', 'thermalservice').stdout
            stamp = round(elapsed)
            (out / f'memory-{stamp}.txt').write_text(memory, encoding='utf-8')
            (out / f'thermal-{stamp}.txt').write_text(thermal, encoding='utf-8')
            match = re.search(r'TOTAL PSS:\s*(\d+)', memory) or re.search(r'^\s*TOTAL\s+(\d+)', memory, re.M)
            if match:
                pss.append(int(match.group(1)))
            match = re.search(r'Thermal Status:\s*(\d+)', thermal)
            if match:
                thermals.append(int(match.group(1)))
            residency.append(state.get('loadedLevels', 0))
            samples.append({'seconds': elapsed, 'state': state})
            write(out / 'samples.json', samples)
            if len(samples) in (2, 5, 9, 13):
                screenshot = adb('exec-out', 'screencap', '-p', binary=True)
                (out / f'screen-{stamp}.png').write_bytes(screenshot.stdout)
            print(f'{args.variant}: {stamp}s; tick {state["logical"].get("tick")}; PSS {pss[-1] if pss else "unknown"}', flush=True)
        fresh = set(adb('shell', 'ls', SAVED + '/Profiling/CSV').stdout.splitlines()) - old_csv
        fresh = sorted(name for name in fresh if name.endswith('.csv'))
        if len(fresh) != 1:
            raise RuntimeError(f'Expected exactly one new CSV, found {fresh}')
        adb('pull', SAVED + '/Profiling/CSV/' + fresh[0], str(out / 'capture.csv'))
        adb('pull', SAVED + '/S8', str(out / 'Saved-S8'))
        write(out / 'device-metrics.json', {'PSS': pss, 'thermalStatus': thermals, 'residency': residency,
                                          'loadLatency': latency, 'activationCounts': [],
                                          'scope': 'paired real-device locomotive review'})
        measurement = subprocess.run(['python', str(Path(__file__).with_name('measure_s8.py')),
                                     str(out / 'capture.csv'), '--device', str(out / 'device-metrics.json'),
                                     '--apk', str(apk), '--output', str(out / 'measurement.json')],
                                    capture_output=True, text=True)
        (out / 'measurement-console.txt').write_text(measurement.stdout + measurement.stderr, encoding='utf-8')
        if measurement.returncode:
            raise RuntimeError('Capture analysis failed; see measurement-console.txt')
        write(out / 'completion.json', {'complete': True, 'variant': args.variant,
                                       'representativeFullS8': False, 'seconds': time.monotonic()-start})
    except Exception as error:
        write(out / 'failure.json', {'complete': False, 'reason': str(error)})
        raise
    finally:
        adb('shell', 'am', 'force-stop', PACKAGE, check=False)
        if changed:
            if existed:
                adb('push', str(out / 'original-commandline.txt'), REMOTE)
            else:
                adb('shell', 'rm', REMOTE)
        hashes_after = normal_hashes()
        (out / 'normal-checkpoints-after.txt').write_text(hashes_after, encoding='utf-8')
        write(out / 'restoration.json', {'commandlineRestored': True,
                                        'normalCheckpointsUnchanged': hashes_after == hashes_before})
        if hashes_after != hashes_before:
            raise RuntimeError('Normal S7 checkpoint hashes changed unexpectedly')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('variant', choices=['baseline', 'WAP7', 'WAG9'])
    parser.add_argument('--label', required=True)
    parser.add_argument('--frames', type=int, default=6000)
    parser.add_argument('--apk')
    parser.add_argument('--extra',default='')
    run_capture(parser.parse_args())

