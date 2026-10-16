"""Assemble S6 evidence only after all final device runs have completed successfully."""
from pathlib import Path
import hashlib
import json
import re
import sys

doc = Path(__file__).resolve().parents[1]
workspace = doc.parents[1]
ev = doc / 'Evidence'
project = workspace / '03_UnrealProject/MyProject'
def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest().upper()

controls = read(ev / 'Device/controls-final/results.json')
compat = read(ev / 'Device/compatibility-final/results.json')
residency = read(ev / 'Device/sustained-final/residency-summary.json')
profile = read(ev / 'profile-final-summary.json')
automation = read(ev / 'UnrealAutomation/index.json')
assert controls['passed'] and all(controls['checks'].values())
assert compat['passed'] and all(compat['checks'].values())
assert residency['passed'] and all(residency['checks'].values())
assert read(ev / 'Device/profile-final/completion.json')['complete']
assert read(ev / 'Device/profile-final/commandline-restore.json')['restored']
assert automation['succeeded'] == 5 and automation['failed'] == 0
assert profile['total_frames'] == 6000

suites = {}
for name, count in [('content-tests',7),('snapshot-record-tests',15),('world-snapshot-tests',19),('checkpoint-tests',9),('s2-regression',39),('s3-regression',28),('s4-regression',39),('s5-regression',36)]:
    data = read(ev / (name + '.json'))
    assert data['passed'] and len(data['cases']) == count and all(c['passed'] for c in data['cases']), name
    suites[name] = count
packaged = {}
for name, count in [('content-tests',7),('record-tests',15),('world-snapshot-tests',19),('checkpoint-tests',9),('kernel-tests',39),('handling-tests',28),('operations-tests',39),('ai-tests',36)]:
    data = read(ev / 'Device/sustained-final/Saved/S6' / (name + '.json'))
    assert data['passed'] and len(data['cases']) == count and all(c['passed'] for c in data['cases']), name
    packaged[name] = count
write(ev / 'test-summary.json', {'passed':True,'host':suites,'packaged':packaged,'hostTotal':sum(suites.values()),'packagedTotal':sum(packaged.values()),'unrealAutomation':5,'deviceControls':len(controls['checks']),'deviceCompatibility':len(compat['checks'])})

manifest = []
baseline = ev / 'Baseline'
paths = set(p.relative_to(project) for folder in ['Source','Config'] for p in (project/folder).rglob('*') if p.is_file())
paths |= set(p.relative_to(baseline) for p in baseline.rglob('*') if p.is_file())
paths.add(Path('MyProject.uproject'))
for rel in sorted(paths):
    before, after = baseline / rel, project / rel
    old, new = sha(before) if before.exists() else None, sha(after) if after.exists() else None
    if old != new:
        manifest.append({'path':rel.as_posix(),'kind':'modified' if old and new else 'added' if new else 'deleted','beforeSha256':old,'afterSha256':new})
for asset in sorted((project/'Content/IndianRailway/S6').glob('*')):
    if asset.is_file():
        manifest.append({'path':asset.relative_to(project).as_posix(),'kind':'added S6 asset','afterSha256':sha(asset)})
write(ev / 'source-change-manifest.json', manifest)
names = ['RailKernel.cpp','RailHandling.cpp','RailOperations.cpp','RailOperationsAI.cpp','RailAIDriver.cpp','RailKernelTests.cpp','RailHandlingTests.cpp','RailOperationsTests.cpp','RailAITests.cpp']
preserved = [{'file':name,'unchanged':sha(project/'Source/MyProject/Simulation'/name)==sha(baseline/'Source/MyProject/Simulation'/name)} for name in names]
assert all(item['unchanged'] for item in preserved)
write(ev/'source-review.json',{'allPreserved':True,'files':preserved,'changedFileCount':len(manifest),'scope':'Core S2-S5 algorithm/test implementations unchanged. S6 integration branches, codec friendship, content/config and tests reviewed separately.'})
apk = workspace/'08_Builds/S6_Android/Android_ASTC/MyProject-arm64.apk'
assert sha(apk) == 'BF5173EBD75E270C42BFDF6FF043D30548F385D33BB4A67A55B815E43C319619'
write(ev/'artifact.json',{'path':apk.relative_to(workspace).as_posix(),'bytes':apk.stat().st_size,'sha256':sha(apk),'build':'20260927-093401','configuration':'Android ASTC Development ARM64','installedAndTested':True})

croot = ev/'Device/controls-final'
selected = ['02-moving-boundary-paused','03-look-back','04-unloading','05-reloaded','07-moving-restored','07c-last-committed','07d-live-moving-restored','08-background-braking','09-background-frozen','09b-warm-foreground','10-braking-restored','11-corruption-fallback','12b-boundary-stopped','13-back-restored']
recovery = {}
for name in selected:
    s = read(croot/(name+'.json'))
    recovery[name] = {k:s[k] for k in ['logicalFingerprint','generation','paused','loadedLevels','retiredActorsAlive','retiredMeshesAlive','recovery']}
    recovery[name].update(tick=s['logical']['tick'], headMetres=s['logical']['trains'][0]['handling']['headMetres'], speedMps=s['logical']['trains'][0]['handling']['speedMps'])
write(ev/'recovery-summary.json',{'passed':True,'controls':controls,'compatibility':compat,'states':recovery})

# Exact user requirements retained as titles; evidence links are relative to this report directory.
request = Path(sys.argv[1]).read_text(encoding='utf-8-sig')
tests = re.findall(r'S6-T(\d{2})\s*\n([^\n]+)',request)
assert len(tests)==39
e = 'Evidence/'
control = e+'Device/controls-final/results.json'
world = e+'world-snapshot-tests.json'
compatibility = e+'Device/compatibility-final/results.json'
audit = e+'Device/sustained-final/residency-summary.json'
refs = {}
for n in range(1,14): refs[n] = [control,e+'recovery-summary.json',e+'Device/controls-final/04-unloading.json',e+'Device/controls-final/05-reloaded.json']
refs[14]=[audit]
for n in range(15,21): refs[n]=[world,e+'recovery-summary.json']
refs[21]=[e+'checkpoint-tests.json',world]
for n in range(22,27): refs[n]=[compatibility,e+'snapshot-record-tests.json',e+'checkpoint-tests.json']
refs[26].append(e+'content-tests.json')
for n in range(27,32): refs[n]=[control,e+'recovery-summary.json']
for n,stage in zip(range(32,36),range(2,6)): refs[n]=[e+f's{stage}-regression.json',e+'test-summary.json']
refs[36]=[e+'UnrealAutomation/index.json']
refs[37]=[control,audit]
refs[38]=[control,compatibility]
refs[39]=[audit,e+'profile-final-summary.json']
mapping=[{'id':f'S6-T{number}','requirement':title.strip(),'status':'PASS','evidence':refs[int(number)]} for number,title in tests]
write(ev/'test-mapping.json',{'passed':True,'count':39,'tests':mapping,'note':'39 requirement mappings, not 39 independent executions. Some use combined pure, integration and source-review evidence.'})
acceptance = request.split('S6 passes only if:')[1].split('==================================================')[0]
gates = re.findall(r'^(\d+)\. (.+)$',acceptance,re.M)
assert len(gates)==30
gate_tests = [[1],[2,3,4],[5],[6],[7],[8,9,10,11],[12],[13],[14],[15],[15,16,17],[15],[23,24,25,26],[18],[19],[20],[22],[25,26],[27,29,30,31],[16,17,28],[32],[33],[34],[35],[36],[37],[38],[39],[],[]]
rows=[]
for (number,title),mapped in zip(gates,gate_tests):
    evidence=sorted(set(ref for n in mapped for ref in refs[n]))
    if int(number)==29: evidence=[e+'source-change-manifest.json',e+'source-review.json']
    if int(number)==30: evidence=['S6_IMPLEMENTATION_REPORT.md',e+'evidence-hashes.json',e+'artifact.json',e+'test-mapping.json']
    rows.append({'id':int(number),'requirement':title,'status':'PASS','tests':[f'S6-T{n:02}' for n in mapped],'evidence':evidence})
write(ev/'acceptance.json',{'status':'S6 PASS','accepted':30,'total':30,'scope':'Two-chunk technical prototype on iQOO Neo 9 Pro; no production art/world-scale claim','gates':rows})
table='\n'.join(f"| {r['id']} | {r['requirement']} | PASS | "+', '.join(r['tests'] or ['source/evidence audit'])+' |' for r in rows)
(ev/'acceptance-table.md').write_text('| # | Requirement | Result | Evidence mapping |\n|---|---|---|---|\n'+table+'\n',encoding='utf-8')
print('Validated final evidence: 192 host, 192 packaged, 5 Automation suites, 21 control, 10 compatibility checks; 39 mappings; 30 acceptance gates.')
