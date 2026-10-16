"""Verify S8 stabilization's immutable baseline; no Unreal/ADB access.

Run from any directory with Python 3. Default output is the stabilization evidence
directory. Exit 0 = preservation pass and current cab import, 1 = scope failure,
2 = preserved so far but cab import pending. This is not a visual/test pass.
"""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / '07_Documentation/S8/Evidence/Stabilization-20261010'
BEFORE = EVIDENCE / 'Before'
PROJECT = ROOT / '03_UnrealProject/MyProject'
ALLOWED_SOURCE = {
    'MyProject/RailDesktopStabilizationReview.cpp': 'editor-only isolated desktop control capture harness',
    'MyProject/RailDiskRecoveryTests.cpp': 'editor-only isolated disk recovery regression test',
    'MyProject/RailAutomationTests.cpp': 'additional presentation regression tests',
    'MyProject/RailCabStabilizationTests.cpp': 'additional cab regression tests',
    'MyProject/RailControlPresentation.h': 'read-only applied-control projection',
    'MyProject/RailControlIcon.cpp': 'Egmore control icon rendering',
    'MyProject/RailControlIcon.h': 'Egmore control icon rendering',
    'MyProject/RailLocomotiveApplication.cpp': 'instrument-view action wiring',
    'MyProject/RailLocomotiveView.cpp': 'cab readout, reversible camera, telemetry',
    'MyProject/RailLocomotiveView.h': 'cab presentation declarations',
    'MyProject/RailServiceApplication.cpp': 'HUD applied-control readout',
    'MyProject/RailServiceWidget.cpp': 'Egmore icon dock and label-view button',
    'MyProject/RailServiceWidget.h': 'Egmore icon dock declarations',
}
EXPECTED_REVISION = 'cab-stabilization-20261010'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def inventory(directory):
    return {p.relative_to(directory).as_posix(): p for p in directory.rglob('*') if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=EVIDENCE / 'preservation-verification.json')
    args = parser.parse_args()
    failures, checks, changes = [], [], []

    def check(name, passed, detail=None):
        checks.append({'name': name, 'passed': bool(passed), 'detail': detail})
        if not passed:
            failures.append(name)

    # Validate the retained baseline itself before using it as evidence.
    manifest = json.loads((EVIDENCE / 'baseline-hashes.json').read_text(encoding='utf-8-sig'))
    for item in manifest:
        original = item['Path'].replace('\\', '/')
        relative = original.split('/Before/', 1)[1]
        check('baseline_integrity/' + relative, sha(BEFORE / relative) == item['Hash'].lower())

    for category, current in [('Source', PROJECT / 'Source'), ('Config', PROJECT / 'Config'),
                              ('Production', PROJECT / 'Content/IndianRailway/Production')]:
        old, new = inventory(BEFORE / category), inventory(current)
        check(category + '/baseline_present', bool(old))
        unchanged = 0
        for relative in sorted(old.keys() | new.keys()):
            previous = sha(old[relative]) if relative in old else None
            actual = sha(new[relative]) if relative in new else None
            if previous == actual:
                unchanged += 1
                continue
            allowed = (category == 'Source' and relative in ALLOWED_SOURCE and actual is not None)
            allowed |= category == 'Production' and relative == 'locomotives/WAP7.json' and actual is not None
            changes.append({'category': category, 'path': relative, 'before_sha256': previous,
                            'after_sha256': actual, 'allowed_file': allowed,
                            'reason': ALLOWED_SOURCE.get(relative) if category == 'Source' else None})
            check(category + '/change_scope/' + relative, allowed)
        checks.append({'name': category + '/unchanged_file_count', 'passed': True, 'detail': unchanged})

    old_wap = json.loads((BEFORE / 'Production/locomotives/WAP7.json').read_text())
    new_wap = json.loads((PROJECT / 'Content/IndianRailway/Production/locomotives/WAP7.json').read_text())
    mutable = {'parts', 'instrument_display', 'cab_reference_limits', 'cab_presentation_revision'}
    check('WAP7/immutable_metadata_and_driver_camera',
          {k: v for k, v in old_wap.items() if k not in mutable} ==
          {k: v for k, v in new_wap.items() if k not in mutable})
    check('WAP7/exterior_runtime_parts_exact',
          [p for p in old_wap['parts'] if not p.get('cab_only')] ==
          [p for p in new_wap['parts'] if not p.get('cab_only')])
    identities = lambda data: sorted((p['name'], p['role']) for p in data['parts'] if p.get('cab_only'))
    check('WAP7/cab_part_identity_and_count', identities(old_wap) == identities(new_wap))
    for part in new_wap['parts']:
        if part.get('cab_only'):
            check('WAP7/cab_asset_scope/' + part['name'],
                  part.get('asset', '').startswith('/Game/IndianRailway/S8/Locomotives/WAP7/Cab/'))
    old_assembly = json.loads((BEFORE / 'WAP7/assembly.json').read_text())
    assembly = json.loads((ROOT / '04_Assets/S8/Cabs/WAP7/assembly.json').read_text())
    check('WAP7/source_driver_camera_unchanged', old_assembly['driver_camera'] == assembly['driver_camera'])
    ready = (assembly.get('presentation_revision') == EXPECTED_REVISION and
             new_wap.get('cab_presentation_revision') == EXPECTED_REVISION)
    report = {
        'generated_utc': datetime.now(timezone.utc).isoformat(),
        'status': 'FAIL' if failures else ('PASS' if ready else 'PENDING_IMPORT'),
        'scope': 'Source, Config, Production baseline hashes and WAP7 runtime/source camera invariants',
        'cab_source_revision': assembly.get('presentation_revision'),
        'cab_runtime_revision': new_wap.get('cab_presentation_revision'),
        'checks': checks, 'changes': changes, 'failures': failures,
        'allowed_presentation_files': ALLOWED_SOURCE,
        'limitations': [
            'Allowed source-file changes still require code review and regression tests; file scope is not semantic proof.',
            'No exterior binary-asset baseline was supplied: exact exterior runtime records are checked, not exterior uasset hashes.',
            'Does not establish visual fidelity, successful package, device behaviour, save integrity, or runtime regression results.',
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': report['status'], 'failures': failures, 'output': str(args.output)}, indent=2))
    return 1 if failures else (0 if ready else 2)


if __name__ == '__main__':
    raise SystemExit(main())
