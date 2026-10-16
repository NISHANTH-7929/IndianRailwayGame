"""Summarize existing sealed captures without rewriting their raw evidence."""
import hashlib,json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
E=ROOT/'07_Documentation/S8/Evidence/Station'
APK=ROOT/'08_Builds/S8_Station/Android_ASTC/MyProject-arm64.apk'
sha=hashlib.sha256(APK.read_bytes()).hexdigest()
rows=[]
for family,label in [('Station','station-a-01'),('LHB','station-apk-baseline-01'),
                     ('Station','station-v5-a'),('Station','station-v5-b'),
                     ('LHB','station-v5-baseline'),('Station','station-v6-a'),
                     ('Station','station-v6-b'),('LHB','station-v6-baseline'),
                     ('LHB','station-v6-baseline-retry'),('LHB','station-v6-baseline-final'),
                     ('LHB','station-v6-baseline-resumed')]:
    folder=E.parent/family/'Device'/label
    if not (folder/'completion.json').exists():continue
    d=json.loads((folder/'measurement.json').read_text());m=d['metrics']
    restoration=json.loads((folder/'restoration.json').read_text())
    rows.append(dict(label=label,scope=d['scope'],apk=m['packageBytes']['sha256'].lower(),
                     fps=m['FPS']['value'],p95=m['FrameTime']['p95'],p99=m['FrameTime']['p99'],
                     gpu=m['GPUTime']['mean'],pssMiB=m['PSS']['maximum']/1024,
                     savesUnchanged=restoration['normalCheckpointsUnchanged']))
automation=json.loads((E/'AutomationV6/index.json').read_text(encoding='utf-8-sig'))
content=json.loads((E/'content-validation-v6-preserved.json').read_text())
cold=json.loads((E/'cold-recovery-v6.json').read_text())
recovery_matrix=json.loads((E/'recovery-matrix-v6.json').read_text()) if (E/'recovery-matrix-v6.json').exists() else {'status':'Pending phone unlock for legacy/LHB cold launches; initial station cold recovery passed.'}
result=dict(apkSha256=sha,apkBytes=APK.stat().st_size,contentValidated=content['passed'],
            automationPassed=automation['succeeded'],automationFailed=automation['failed'],
            coldRecovery=cold,recoveryMatrix=recovery_matrix,measurements=rows,visualAccepted=False,fullS8Complete=False,
            limitations=['Single captures per configuration, not a three-run campaign.',
                         'No sustained thermal qualification or compositor timing claim.',
                         'Remaining art gaps are listed in STATION_VISUAL_REVIEW.md.'])
(E/'validation-summary.json').write_text(json.dumps(result,indent=2))
lines=['# Station candidate validation','',
       'The 26-module station kit is integrated and measured as a candidate. It is not final high-realism acceptance or completion of all S8 content.','',
       f'Current Android ASTC APK: {APK.stat().st_size:,} bytes. SHA-256 `{sha}`.','',
       f"Unreal Automation: {automation['succeeded']} passed, {automation['failed']} failed. Source/import byte checks: {content['passed']}. Each module has three imported LODs; aggregate triangle counts are {content['lodTriangles']}.",'',
       f"Android APK-update/cold recovery: {cold['passed']}. Generation {cold['generation']}, service tick {cold['tick']}, exact logical fingerprint `{cold['fingerprint']}`. Service state, eight LHB vehicles and all 4,216 station instances were restored.",'',
       '| Capture | Current APK | FPS | Frame p95 / p99 ms | GPU mean ms | Peak PSS MiB | Saves preserved |',
       '|---|---|---:|---:|---:|---:|---|']
for r in rows:
    lines.append(f"| {r['label']} | {'yes' if r['apk']==sha else 'earlier'} | {r['fps']:.3f} | {r['p95']:.3f} / {r['p99']:.3f} | {r['gpu']:.3f} | {r['pssMiB']:.2f} | {r['savesUnchanged']} |")
lines+=['','Each capture uses 6,000 engine frames, a 30 FPS cap and a 10–170 second analysis window on the connected iQOO Neo 9 Pro, Android API 36, Vulkan/ASTC. CPU/GPU intervals overlap. These are engine intervals, not compositor measurements. Peak memory includes the whole sampled run.',
        '', 'Only compare rows sharing an APK hash. The changed exterior camera also changes visible geometry, so earlier-versus-current differences are not isolated geometry optimization results. One run per configuration cannot establish sustained thermal behavior or a final performance budget.',
        '', 'The first captures inherited a planned `repetitions: 3` profile label despite containing one capture; this report treats them as one. The profiler now writes the actual count. Early station context/device-metrics descriptions also inherited LHB-only wording; launch flags, measurement scope and actual station telemetry identify their station workload. Raw evidence is retained.',
        '', 'Normal S7 and LHB checkpoint hashes are checked around each capture. Later captures additionally check station-service slots. The station service has its own save directory; old fixture content identity and authoritative simulation algorithms are unchanged.',
        '', 'USB-only stay-awake was verified as Android setting `2`. It does not request battery-only wakefulness. Visual limitations and comparison evidence are documented in STATION_VISUAL_REVIEW.md; architecture changes are documented in STATION_ARCHITECTURE.md.',
        '', 'V6 corrects the reflected FBX Y axis. Authored and imported bounds agree within 1 mm, and Unreal Automation checks the actual platform edge and height. V5 timings remain historical evidence; its platform alignment is superseded.',
        '', 'V6 used wireless ADB. Battery and thermal snapshots accompany each capture. The APK contains the earlier Egmore study assets but these measurements exercise the generic station kit; they do not measure the new Egmore architectural revision.',
        '', ('Legacy, LHB and station cold-recovery results are recorded in Evidence/Station/recovery-matrix-v6.json.' if 'status' not in recovery_matrix else recovery_matrix['status'])+' The S7 map was repaired after an art-review script mistakenly edited it; retained actors were audited against prior evidence. Its file hash changed through serialization. See STATION_ARCHITECTURE.md for the repair record.',
        '', 'V6 train-only comparison attempts lost foreground, including sleep/keyguard and later backgrounding. They are excluded and preserved with failure/restoration logs; normal checkpoints stayed unchanged. No valid V6 train-only comparison is claimed unless a completed row appears above.',
        '', 'Failed import-v4.log is preserved: LOD generation rejected commandlet execution. Full-editor imports passed. The V6 Android build succeeded and installed with `adb install -r`.']
(ROOT/'07_Documentation/S8/STATION_VALIDATION_REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
