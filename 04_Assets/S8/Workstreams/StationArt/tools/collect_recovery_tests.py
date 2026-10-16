"""Read-only collection of startup test reports following an owner-run recovery."""
import argparse
import datetime
import json
from pathlib import Path
import re
import device_review as device

EXPECTED = {'kernel': 39, 'handling': 28, 'operations': 39, 'ai': 36,
            'content': 7, 'record': 15, 'world-snapshot': 19, 'checkpoint': 9,
            'lhb': 25, 'service': 46}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--recovery', type=Path, required=True)
    args = parser.parse_args()
    root = args.recovery.resolve(strict=True)
    receipt = json.loads((root / 'result.json').read_text())
    if not receipt.get('passed'):
        raise RuntimeError('Requires completed successful recovery')
    before = device.preflight()
    if before['installedApkSha256'] != receipt['apkSha256']:
        raise RuntimeError('Installed APK changed after recovery')
    log = (root / 'station/engine.log').read_text(encoding='utf-8-sig')
    marker = re.search(r'\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}):\d+\]', log)
    if not marker:
        raise RuntimeError('No engine UTC timestamp for report freshness')
    earliest = datetime.datetime.strptime(marker[1], '%Y.%m.%d-%H.%M.%S').replace(tzinfo=datetime.timezone.utc).timestamp()
    out = root / 'startup-tests'
    out.mkdir(exist_ok=False)
    reports = []
    for name, count in EXPECTED.items():
        remote = device.SAVED + '/S8/Station/Service/' + name + '-tests.json'
        raw = device.adb('exec-out', 'cat', remote).stdout
        modified = int(device.adb('shell', 'stat', '-c', '%Y', remote).stdout.strip())
        (out / (name + '-tests.json')).write_bytes(raw)
        report = json.loads(raw.decode('utf-8-sig'))
        valid = (report.get('passed') is True and len(report.get('cases', [])) == count
                 and all(row.get('passed') is True for row in report['cases']))
        reports.append(dict(name=name, count=len(report.get('cases', [])), passed=valid,
                            modifiedEpoch=modified, freshAfterEngineStart=modified >= earliest))
    after = device.preflight()
    unchanged = before == after
    passed = unchanged and all(r['passed'] and r['freshAfterEngineStart'] for r in reports)
    (out / 'summary.json').write_text(json.dumps(dict(passed=passed, apkSha256=receipt['apkSha256'],
        sourceRecovery=str(root), engineStartEpoch=earliest, groups=reports,
        caseCount=sum(r['count'] for r in reports), deviceStateUnchanged=unchanged), indent=2))
    print(json.dumps(dict(passed=passed, groups=len(reports), cases=sum(r['count'] for r in reports))))
    if not passed:
        raise RuntimeError('Report validation/freshness/preservation failed')


if __name__ == '__main__':
    main()
