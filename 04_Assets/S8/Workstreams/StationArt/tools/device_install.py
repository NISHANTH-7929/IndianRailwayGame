"""Install an explicitly handed-off APK without deleting game state.

No uninstall/data clearing fallback. Verify exact artifact SHA supplied by owner,
snapshot normal saves and launch override, replace-install, then verify everything.
"""
import argparse
import json
from pathlib import Path
import device_review as device


def install(args):
    apk = Path(args.apk).resolve(strict=True)
    if not device.re.fullmatch(r'[a-fA-F0-9]{64}', args.sha256):
        raise ValueError('Expected full explicit SHA256')
    expected = args.sha256.lower()
    if device.sha(apk.read_bytes()) != expected:
        raise RuntimeError('Handoff APK SHA256 mismatch; no phone mutation')
    if not device.re.fullmatch(r'[A-Za-z0-9-]+', args.label):
        raise ValueError('Invalid label')
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    out.mkdir(parents=True, exist_ok=False)
    before = device.preflight()
    exists = device.exists(device.REMOTE)
    original = device.adb('exec-out', 'cat', device.REMOTE).stdout if exists else None
    if exists:
        (out / 'original-commandline.txt').write_bytes(original)
    (out / 'before.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    # Stop before backup to eliminate save writes racing with this snapshot.
    device.adb('shell', 'am', 'force-stop', device.PACKAGE)
    before['saves'] = device.save_state()
    (out / 'before-stopped.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    for index, (remote, digest) in enumerate(before['saves'].items()):
        if digest is not None:
            data = device.adb('exec-out', 'cat', remote).stdout
            if device.sha(data) != digest:
                raise RuntimeError('Checkpoint changed during backup; install refused')
            (out / f'checkpoint-backup-{index}.irs').write_bytes(data)
    receipt = dict(apk=str(apk), expectedSha256=expected, installed=None,
                   savesUnchanged=None, commandlineUnchanged=None, wakeUnchanged=None,
                   verificationStatus='not-run')
    try:
        options = ['--no-streaming'] if getattr(args, 'no_streaming', False) else []
        receipt['transferMode'] = 'non-streaming' if options else 'streamed'
        result = device.adb('install', *options, '-r', str(apk), check=False)
        (out / 'install-stdout.txt').write_bytes(result.stdout)
        (out / 'install-stderr.txt').write_bytes(result.stderr)
        receipt['installExitCode'] = result.returncode
        if result.returncode or b'Success' not in result.stdout:
            raise RuntimeError('Replace-install failed; no destructive fallback allowed')
        after = device.preflight()
        receipt['verificationStatus'] = 'running'
        receipt['installed'] = after['installedApkSha256'] == expected
        receipt['savesUnchanged'] = after['saves'] == before['saves']
        exists_after = device.exists(device.REMOTE)
        receipt['commandlineUnchanged'] = exists_after == exists and (not exists or
            device.adb('exec-out', 'cat', device.REMOTE).stdout == original)
        receipt['wakeUnchanged'] = after['wake'] == before['wake']
        receipt['artifactUnchanged'] = device.sha(apk.read_bytes()) == expected
        receipt['verificationStatus'] = 'complete'
        if not all(receipt.get(k) for k in ('installed', 'savesUnchanged', 'commandlineUnchanged', 'wakeUnchanged', 'artifactUnchanged')):
            raise RuntimeError('Post-install verification failed; stop and inspect preserved backups')
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        (out / 'result.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(out)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--apk', required=True)
    parser.add_argument('--sha256', required=True)
    parser.add_argument('--label', required=True)
    parser.add_argument('--no-streaming', action='store_true')
    install(parser.parse_args())
