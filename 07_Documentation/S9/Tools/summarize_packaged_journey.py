"""Read-only evidence arithmetic for the packaged trip and settled recovery."""
import hashlib, json, re, sys
from pathlib import Path

root = Path(__file__).resolve().parents[3]
evidence = Path(sys.argv[1])
output = Path(sys.argv[2])
if output.exists(): raise SystemExit('Use fresh summary output')
rows = []
for name in ('journey-observations.jsonl', 'journey-observations-resumed.jsonl', 'journey-realism-candidate.jsonl', 'journey-completion-20261017.jsonl'):
    path = evidence/name
    if path.exists(): rows += [json.loads(line) for line in path.read_text().splitlines() if line]
rows.sort(key=lambda row: row['tick'])
stream_path = evidence/'journey-streaming-resumed.jsonl'
stream = [json.loads(line) for line in stream_path.read_text().splitlines() if line] if stream_path.exists() else []
for name in ('journey-realism-candidate.jsonl','journey-completion-20261017.jsonl'):
    p=evidence/name
    if p.exists(): stream += [json.loads(line) for line in p.read_text().splitlines() if line]
alignment = (root/'03_UnrealProject/MyProject/Source/MyProject/Simulation/RailTambaramAlignment.generated.h').read_text()
stations = [float(v) for v in re.search(r'Stations\{\{([^}]+)', alignment)[1].split(',')]
names = ['Chennai Egmore','Chennai Chetpet','Nungambakkam','Kodambakkam','Mambalam','Saidapet','Guindy','St. Thomas Mount','Palavanthangal','Meenambakkam','Tirusulam','Pallavaram','Chrompet','Tambaram Sanatorium','Tambaram']
terminal = json.loads((evidence/'terminal-telemetry.json').read_text(encoding='utf-8-sig'))
settled = json.loads((evidence/'settled-telemetry.json').read_text(encoding='utf-8-sig'))
restored = json.loads((evidence/'settled-reloaded-telemetry.json').read_text(encoding='utf-8-sig'))
baseline = json.loads((root/'07_Documentation/S9/Evidence/PackagedUI-20261016-01/normal-save-before.json').read_text(encoding='utf-8-sig'))
changed = [path for path, digest in baseline.items() if not Path(path).exists() or hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest]
result = {
    'scope':'Shipping packaged journey; elapsed-time resumes and UI settlement; isolated profile',
    'logicalSamples':len(rows), 'streamingSamples':len(stream),
    'sampledFaults':sum(bool(row['faulted']) for row in rows),
    'sampledInvalidFingerprints':sum(row['fingerprint']=='INVALID' for row in rows),
    'overloadEvidence':[p.name for p in evidence.glob('Overload-*-SaveBackup')],
    'sampledBlockedStates':sum(bool(row['blocked']) for row in rows),
    'maxWorkingSetBytes':max(row['workingSetBytes'] for row in rows),
    'maxPrivateBytes':max(row['privateBytes'] for row in rows),
    'stationCrossings':[dict(station=name,marker=position,observed=any(row['head']>=position-1 for row in rows)) for name,position in zip(names,stations)],
    'streamingAllSampledReady':all(row['streaming']=='Essential presentation ready' and all(view['ready'] and view['station']['ready'] for view in row['views']) for row in stream),
    'maxCellsPerView':max((view['cells'] for row in stream for view in row['views']),default=0),
    'retiredActorPeak':max((row['retiredActorsAlive'] for row in stream),default=0),
    'terminal':terminal['service'],
    'settledProfile':settled['service']['profile'],
    'restoredProfile':restored['service']['profile'],
    'settlementRecoveredOnce':settled['service']['settled'] and restored['service']['settled'] and settled['service']['profile']==restored['service']['profile'] and restored['service']['profile']['awards']==1,
    'terminalFingerprintPreserved':terminal['logicalFingerprint']==settled['logicalFingerprint']==restored['logicalFingerprint'],
    'settledFingerprintPreserved':settled['logicalFingerprint']==restored['logicalFingerprint'],
    'fingerprintScope':'Fingerprint includes service/profile state; reward settlement changes it. Cold recovery must preserve the settled fingerprint.',
    'normalCheckpointsChecked':len(baseline), 'normalCheckpointsChanged':changed,
    'limits':['5-second sampling cannot exclude sub-sample faults or streaming stalls.', 'Station crossings prove logical progression, not surveyed art fidelity.', 'Frame pacing is reported separately from PresentMon captures.']
}
output.write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
