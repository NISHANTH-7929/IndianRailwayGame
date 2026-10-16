"""Read-only integrity check for the sealed handoff."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1]/'v2.0.0'
records=json.loads((R/'SHA256SUMS.json').read_text());errors=[]
for r in records:
    p=R/r['path']
    if not p.is_file() or p.stat().st_size!=r['bytes'] or hashlib.sha256(p.read_bytes()).hexdigest()!=r['sha256']:errors.append(r['path'])
actual={p.relative_to(R).as_posix() for p in R.rglob('*') if p.is_file() and p.name!='SHA256SUMS.json'}
if actual!={r['path'] for r in records}:errors.append('file roster changed')
print(json.dumps({'status':'PASS' if not errors else 'FAIL','verifiedFiles':len(records),'errors':errors}));raise SystemExit(bool(errors))
