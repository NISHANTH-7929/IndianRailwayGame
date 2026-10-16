"""Bind current source and retained S9 acceptance files without copying builds."""
from pathlib import Path
import hashlib,json,sys
r=Path(__file__).resolve().parents[3];out=Path(sys.argv[1])
if out.exists(): raise SystemExit('Use a fresh manifest path')
paths=[]
for folder in ['03_UnrealProject/MyProject/Source','03_UnrealProject/MyProject/Config','07_Documentation/S9']:
 for p in (r/folder).rglob('*'):
  if p.is_file() and p.suffix.lower() in {'.cpp','.h','.cs','.ini','.py','.ps1','.cmd','.json','.md','.txt','.wav','.png','.jpg'} and 'Baseline-20261015' not in p.parts: paths.append(p)
records=[dict(path=p.relative_to(r).as_posix(),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sorted(set(paths))]
out.write_text(json.dumps(dict(content='IR-S8-MSTBM-6',service='IR-S8-MSTBM-service-7',saveDirectory='ServiceV6',status='partial desktop candidate',files=records),indent=2))
print(json.dumps(dict(files=len(records),manifest=str(out))))
