"""Record hashes of final review evidence and the immutable Android candidate."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/Stabilization-20261010'
APK=ROOT/'08_Builds/S8_EgmoreService_20261010-214817/Android_ASTC/MyProject-arm64.apk'
expected='9483a78c0df9bea5cdb57b1954a27b9a487863241e11a87fadc4db7b8b0f86d3'
def row(path):
    data=path.read_bytes()
    return dict(path=path.relative_to(ROOT).as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
artifact=row(APK)
assert artifact['sha256']==expected,'Packaged candidate changed'
files=[]
for folder in ['InteriorFinal','StationDesktop','AutomationFinal']:
    files.extend(p for p in (OUT/folder).rglob('*') if p.is_file())
for name in ['preservation-verification.json','cab-validation-final.json','device-tools-unittest-expanded.log','REPORT.md']:
    path=OUT/name
    assert path.is_file(),f'Missing final evidence: {name}'
    files.append(path)
result=dict(artifact=artifact,evidence=[row(p) for p in sorted(files)],
            androidAcceptance=False,phoneTests='deferred by user',
            scope='Integrity roster only; hashes do not prove visual or runtime acceptance')
(OUT/'final-evidence-hashes.json').write_text(json.dumps(result,indent=2))
print(json.dumps(dict(artifact=artifact,evidenceFiles=len(files)),indent=2))
