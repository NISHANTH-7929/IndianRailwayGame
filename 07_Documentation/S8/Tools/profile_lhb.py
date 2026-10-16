"""Capture the real packaged WAP-7 + LHB workload on the connected iQOO.

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


def read_live_json(reader, rejected, attempts=5):
    """A live telemetry file can be truncated briefly during its next write.

    Retry only JSON parse failures, retaining each rejected payload. Never
    substitute a previous sample, or hide persistent corruption / ADB errors.
    """
    for attempt in range(attempts):
        raw=reader()
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            rejected(attempt,raw)
            if attempt+1==attempts:raise
            time.sleep(.2)


def run_capture(args):
    adb_path = Path(os.environ.get('ANDROID_HOME', '')) / 'platform-tools/adb.exe'
    if not adb_path.is_file():
        raise RuntimeError('ANDROID_HOME/platform-tools/adb.exe is unavailable')
    inventory = subprocess.run([str(adb_path), 'devices'], capture_output=True, text=True, check=True).stdout
    devices = [line.split()[0] for line in inventory.splitlines()
               if '10BE1Y2EL40003J' in line and line.endswith('\tdevice')]
    if len(devices) != 1:
        raise RuntimeError(f'Expected one authorized iQOO endpoint, found {len(devices)}')
    serial = devices[0]
    station = getattr(args, 'station', False)
    out = ROOT / ('07_Documentation/S8/Evidence/Station/Device' if station else '07_Documentation/S8/Evidence/LHB/Device') / args.label
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
        result = adb('shell', 'sha256sum', SAVED + '/S7/checkpoint-0.irs', SAVED + '/S7/checkpoint-1.irs', SAVED + '/S8/LHB/Service/checkpoint-0.irs', SAVED + '/S8/LHB/Service/checkpoint-1.irs')
        rows=[result.stdout.strip()]
        for slot in range(2):
            path=SAVED+f'/S8/Station/Service/checkpoint-{slot}.irs'
            if adb('shell','test','-f',path,check=False).returncode==0:
                rows.append(adb('shell','sha256sum',path).stdout.strip())
        return '\n'.join(rows)

    model = adb('shell', 'getprop', 'ro.product.model').stdout.strip()
    api = adb('shell', 'getprop', 'ro.build.version.sdk').stdout.strip()
    (out/'battery-before.txt').write_text(adb('shell','dumpsys','battery').stdout,encoding='utf-8')
    if model != 'I2304' or api != '36':
        raise RuntimeError(f'Unexpected device: {model}, API {api}')
    apk = Path(args.apk) if args.apk else ROOT / '08_Builds/S8_LHB/Android_ASTC/MyProject-arm64.apk'
    installed_paths=adb('shell','pm','path',PACKAGE).stdout.strip().splitlines()
    if len(installed_paths)!=1 or not installed_paths[0].startswith('package:'):
        raise RuntimeError('Expected one installed base APK')
    installed_hash=adb('shell','sha256sum',installed_paths[0].removeprefix('package:')).stdout.split()[0]
    artifact_hash=hashlib.sha256(apk.read_bytes()).hexdigest()
    write(out/'installed-apk.json',{'path':installed_paths[0],'sha256':installed_hash,'artifactSha256':artifact_hash,'matches':installed_hash.lower()==artifact_hash})
    if installed_hash.lower()!=artifact_hash:
        raise RuntimeError('Installed APK does not match the workload artifact')
    # Review launch has its own save directory; the ordinary game is not resumed.
    adb('shell', 'am', 'force-stop', PACKAGE)
    original = adb('shell', 'cat', REMOTE, check=False)
    existed = original.returncode == 0
    if existed:
        (out / 'original-commandline.txt').write_text(original.stdout, encoding='utf-8')
    hashes_before = normal_hashes()
    (out / 'normal-checkpoints-before.txt').write_text(hashes_before, encoding='utf-8')
    old_csv = set(adb('shell', 'ls', SAVED + '/Profiling/CSV', check=False).stdout.splitlines())
    flags = '-S8LocoClass=WAP7 -S8DisableCoach' if args.variant == 'baseline' else '-S8LocoClass=' + args.variant
    command = (f'-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S8/LHB/L_RailService_LHB '
               f'-S8AssetReview -S8LHB {flags} {args.extra} -csvCaptureFrames={args.frames} -csvGpuStats '
               '-csvCompression=0 -ExecCmds="r.SetFramePace 30" -log')
    if station:
        command=command.replace('/Game/IndianRailway/S8/LHB/L_RailService_LHB','/Game/IndianRailway/S8/Stations/L_RailService_Station')
        command+=' -S8Station'+(' -S8StationB' if args.arrangement=='B' else '')
    (out / 'review-commandline.txt').write_text(command, encoding='utf-8')
    apk = Path(args.apk) if args.apk else ROOT / '08_Builds/S8_LHB/Android_ASTC/MyProject-arm64.apk'
    write(out / 'context.json', {'variant': args.variant, 'serial': serial, 'model': model, 'api': api,
                               'frames': args.frames, 'scope': ('isolated WAP-7 + LHB + station arrangement '+args.arrangement if station else 'isolated WAP-7 + LHB service review; no other S8 content'),
                               'apkSha256': hashlib.sha256(apk.read_bytes()).hexdigest(), 'apkBytes': apk.stat().st_size})
    changed = False
    samples, pss, thermals, residency, latency, activations = [], [], [], [], [], []
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
            telemetry_path=SAVED + ('/S8/Station/AssetReview/telemetry.json' if station else '/S8/LHB/AssetReview/telemetry.json')
            state = read_live_json(
                lambda: adb('shell','cat',telemetry_path).stdout,
                lambda attempt,raw: (out/f'telemetry-rejected-{round(elapsed)}-{attempt}.txt').write_text(raw,encoding='utf-8'))
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
            latency.extend(v for v in state.get('loadMs',[]) if v>0)
            activations.append(state.get('loads',0))
            if state.get('content')!='IR-S8-LHB-3A-1' or state.get('blocked'):
                raise RuntimeError('Wrong content identity or blocked workload')
            if args.variant=='WAP7' and state.get('lhbVehicles',0)==0:
                raise RuntimeError('No LHB vehicles presented')
            if station and state.get('stationInstances',0)<100:
                raise RuntimeError('Station content missing')
            samples.append({'seconds': elapsed, 'state': state})
            write(out / 'samples.json', samples)
            if elapsed <= 180 and (state.get('paused') or state.get('blocked')):
                raise RuntimeError('Workload paused/blocked during measurement; exclude this run')
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
                                          'loadLatency': latency, 'activationCounts': activations,
                                          'scope': ('real-device WAP-7 + LHB + station '+args.arrangement if station else 'real-device WAP-7 + LHB review')})
        profile=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/profile.json').read_text(encoding='utf-8-sig'))
        profile.update(profileId='Profile.LHB.01',workloadId='Workload.WAP7.LHB.01',scope='WAP7-LHB-service-'+args.variant)
        if station:
            profile.update(profileId='Profile.Station.01',workloadId='Workload.Station.01',scope='WAP7-LHB-station-'+args.arrangement)
        # This invocation records one run, regardless of the planned campaign.
        profile['repetitions']=1
        write(out/'profile.json',profile)
        measurement = subprocess.run(['python', str(Path(__file__).with_name('measure_s8.py')),
                                     str(out / 'capture.csv'), '--device', str(out / 'device-metrics.json'),
                                     '--profile', str(out/'profile.json'),
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
        (out/'battery-after.txt').write_text(adb('shell','dumpsys','battery',check=False).stdout,encoding='utf-8')
        engine_log = adb('shell', 'cat', SAVED + '/Logs/MyProject.log', check=False)
        (out / 'engine.log').write_text(engine_log.stdout, encoding='utf-8')
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
    parser.add_argument('variant', choices=['baseline', 'WAP7'])
    parser.add_argument('--label', required=True)
    parser.add_argument('--frames', type=int, default=6000)
    parser.add_argument('--apk')
    parser.add_argument('--extra',default='')
    parser.add_argument('--station',action='store_true')
    parser.add_argument('--arrangement',choices=['A','B'],default='A')
    run_capture(parser.parse_args())



