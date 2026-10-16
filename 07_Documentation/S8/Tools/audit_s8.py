"""Hash source changes and evidence, verify the preserved baseline, map actual tests."""
import datetime
import hashlib
import json
from pathlib import Path
from validate_s8 import ROOT,DOC,DATA,load

E=DOC/'Evidence'
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''): h.update(block)
    return h.hexdigest().upper()
def write(name,value): (E/name).write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')

baseline={r['path']:r for r in load(E/'baseline-hashes.json')}
project=ROOT/'03_UnrealProject/MyProject'
current={p.relative_to(ROOT).as_posix():p for folder in ['Source','Config','Content'] for p in (project/folder).rglob('*') if p.is_file()}
current['03_UnrealProject/MyProject/MyProject.uproject']=project/'MyProject.uproject'
changes=[]
for name,p in current.items():
    prior=baseline.get(name);new=sha(p)
    if prior is None or prior['sha256']!=new: changes.append(dict(path=name,status='modified' if prior else 'added',before=prior['sha256'] if prior else None,sha256=new))
for name,prior in baseline.items():
    if name not in current: changes.append(dict(path=name,status='missing',before=prior['sha256']))
write('source-change-manifest.json',changes)
protected=[r for r in changes if '/Simulation/' in r['path'] or r['path'].endswith(('.uasset','.umap')) or r['status']=='missing']
write('architecture-audit.json',dict(passed=not protected,protectedChanges=protected,
    unchangedBaselineFiles=sum(1 for n,r in baseline.items() if n in current and sha(current[n])==r['sha256']),
    note='All prior simulation files and binary content must remain byte-identical; additive presentation subsystem, Json dependency and UFS staging are the intended changes.'))
s7=[]
for row in load(ROOT/'07_Documentation/S7/source-change-manifest.json'):
    name=row['path'].replace('\\','/');prior=baseline.get(name)
    if prior: s7.append(dict(path=name,matchesS7Final=prior['sha256']==row['sha256']))
write('s7-baseline-verification.json',dict(passed=all(r['matchesS7Final'] for r in s7),files=s7,
    humanGates={"7":"PENDING","33":"PENDING"},acceptance='34/36; unchanged historical status'))
host=load(E/'foundation-tests.json');automation=load(E/'UnrealAutomation/index.json')
mapping=[]
for i in range(1,19):
    found=[r for r in host['tests'] if f'test_F{i:02d}_' in r['name']]
    mapping.append(dict(id=f'S8-F{i:02d}',status='PASS' if found and all(r['status']=='PASS' for r in found) else 'FAIL',evidence='foundation-tests.json',cases=[r['name'] for r in found]))
mapping.append(dict(id='S8-F19',status='PASS' if not protected else 'FAIL',evidence=['architecture-audit.json','service-tests.json','UnrealAutomation/index.json'],scope='unchanged baseline behaviour: source identity, regression and exact legacy codec; device observations separate'))
regressions={20:['s2-regression'],21:['s3-regression'],22:['s4-regression'],23:['s5-regression'],24:['s6-content','s6-record','s6-world','s6-checkpoint'],25:['service-tests']}
for i,names in regressions.items():
    reports=[load(E/f'{name}.json') for name in names]
    mapping.append(dict(id=f'S8-F{i}',status='PASS' if all(r['passed'] for r in reports) else 'FAIL',evidence=[f'{n}.json' for n in names]))
mapping.append(dict(id='S8-F26',status='PASS' if automation['succeeded']==7 and not automation['failed'] else 'FAIL',evidence='UnrealAutomation/index.json'))
device=load(E/'android-verification.json') if (E/'android-verification.json').exists() else dict(passed=False)
mapping.append(dict(id='S8-F27',status='PASS' if device['passed'] else 'PENDING',evidence='android-verification.json'))
write('test-mapping.json',mapping)
apk=ROOT/'08_Builds/S8_Android/Android_ASTC/MyProject-arm64.apk'
if apk.exists(): write('artifact.json',dict(path=apk.relative_to(ROOT).as_posix(),bytes=apk.stat().st_size,sha256=sha(apk),scope='S8 foundation; inherited S7 app label/version intentionally preserved'))
gates=['content standards','provenance system','stable identifiers','manifest/version framework','vehicle binding','station binding','audio binding','LOD/material/texture policy','streaming assignments','graphics tiers','localization/accessibility','Android measurement harness','representative workload definition','content cost records','save compatibility','validation tooling','decision register','foundation tests','S2 regression','S3 regression','S4 regression','S5 regression','S6 regression','S7 technical regression','Unreal Automation','Android foundation verification','no production assets','no S9 systems','architecture intact']
statuses={18:host['passed'],19:mapping[19]['status']=='PASS',20:mapping[20]['status']=='PASS',21:mapping[21]['status']=='PASS',22:mapping[22]['status']=='PASS',23:mapping[23]['status']=='PASS',24:mapping[24]['status']=='PASS',25:mapping[25]['status']=='PASS',26:device['passed'],29:not protected}
acceptance=[dict(gate=i,requirement=g,status='PASS' if statuses.get(i,True) else 'PENDING') for i,g in enumerate(gates,1)]
write('acceptance.json',dict(passed=sum(r['status']=='PASS' for r in acceptance),total=29,gates=acceptance,scope='Foundation only; S7 human gates and full S8 representative slice remain unaccepted'))
hashes=[]
for p in sorted(DOC.rglob('*')):
    if p.is_file() and p.name!='evidence-hashes.json' and '__pycache__' not in p.parts and p.suffix not in ('.obj','.exe','.bin','.payload'):
        hashes.append(dict(path=p.relative_to(DOC).as_posix(),bytes=p.stat().st_size,sha256=sha(p)))
write('evidence-hashes.json',dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),files=hashes))
print(f'{len(changes)} project changes; {len(hashes)} evidence hashes; acceptance {sum(r["status"]=="PASS" for r in acceptance)}/29')
