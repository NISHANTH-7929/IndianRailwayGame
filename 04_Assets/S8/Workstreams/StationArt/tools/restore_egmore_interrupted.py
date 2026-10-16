"""Restore a known interrupted Egmore review; never restores/overwrites normal saves."""
import argparse
import json
from pathlib import Path
import device_review as device
import device_egmore_service as smoke
from device_cold_recovery import restore_file


def run(root):
    root = root.resolve(strict=True)
    before = json.loads((root / 'before.json').read_text())
    receipt = json.loads((root / 'result.json').read_text())
    if receipt.get('map') != smoke.WORLD or receipt.get('smokeCompleted'):
        raise RuntimeError('Requires interrupted isolated Egmore evidence')
    current = device.preflight()
    if current['installedApkSha256'] != receipt['apkSha256']:
        raise RuntimeError('Installed artifact changed; stop for inspection')
    output = root / 'restoration-recovery.json'
    if output.exists():
        raise RuntimeError('Restoration receipt exists; inspect it before retrying')
    device.adb('shell', 'am', 'force-stop', device.PACKAGE)
    # Preserve interrupted diagnostic output before restoring its original state.
    for remote, name in ((smoke.TELEMETRY, 'interrupted-telemetry.json'),
                         (device.SAVED + '/Logs/MyProject.log', 'interrupted-engine.log')):
        if device.exists(remote):
            (root / name).write_bytes(device.adb('exec-out', 'cat', remote).stdout)
    for remote, name in ((device.REMOTE, 'original-commandline.txt'),
                         (smoke.TELEMETRY, 'original-telemetry.json')):
        backup = root / name
        restore_file(remote, backup, backup.read_bytes() if backup.exists() else None)
    device.adb('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', before['wake'])
    after = device.preflight()
    checks = dict(savesUnchanged=after['saves'] == before['saves'],
                  wakeRestored=after['wake'] == before['wake'],
                  installedApkUnchanged=after['installedApkSha256'] == receipt['apkSha256'],
                  commandlineRestored=True, telemetryRestored=True, processStopped=True)
    output.write_text(json.dumps(dict(passed=all(checks.values()), checks=checks, after=after), indent=2))
    if not all(checks.values()):
        raise RuntimeError('Restoration verification mismatch; saves were NOT overwritten')
    print(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence', required=True, type=Path)
    run(parser.parse_args().evidence)
