import datetime
import hashlib
import json
import pathlib

root = pathlib.Path(__file__).resolve().parents[3]
stage = root / '07_Documentation/S7'
project = root / '03_UnrealProject/MyProject'
baseline = stage / 'Evidence/Baseline'

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            h.update(block)
    return h.hexdigest().upper()

changes = []
for folder in ['Source', 'Config']:
    for path in sorted((project / folder).rglob('*')):
        if not path.is_file():
            continue
        relative = path.relative_to(project)
        prior = baseline / relative
        new = digest(path)
        old = digest(prior) if prior.exists() else None
        if new != old:
            changes.append(dict(path=str(path.relative_to(root)), status='modified' if old else 'added', before=old, sha256=new))
    for prior in sorted((baseline / folder).rglob('*')):
        if prior.is_file() and not (project / prior.relative_to(baseline)).exists():
            changes.append(dict(path=str(prior.relative_to(baseline)), status='missing', before=digest(prior)))
for path in sorted((project / 'Content/IndianRailway/S7').rglob('*')):
    if path.is_file():
        changes.append(dict(path=str(path.relative_to(root)), status='added', sha256=digest(path)))
(stage / 'source-change-manifest.json').write_text(json.dumps(changes, indent=2), encoding='utf-8')
evidence = []
for path in sorted(stage.rglob('*')):
    if path.is_file() and path.name != 'evidence-hashes.json' and path.suffix not in {'.obj', '.exe', '.bin'}:
        evidence.append(dict(path=str(path.relative_to(stage)), bytes=path.stat().st_size, sha256=digest(path)))
(stage / 'evidence-hashes.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), files=evidence), indent=2), encoding='utf-8')
print(f'{len(changes)} source/config/assets changes; {len(evidence)} evidence files hashed')
