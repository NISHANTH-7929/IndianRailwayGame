"""Check the recorded final integration evidence and emit an auditable manifest."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
E = ROOT / '07_Documentation/S8/Evidence/LHB'
def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

tests = [read(p) for p in (E/'Device/compatible-restored-result/Saved').glob('*tests.json')]
auto = read(E/'UnrealAutomation-FinalOct03/index.json')
before = read(E/'Device/live-service-settled/telemetry.json')
after = read(E/'Device/live-result-cold-result/telemetry.json')
old = read(E/'Device/legacy-final-result/telemetry.json')
measurements = read(E/'android-measurements.json')['runs']
artifact = read(E/'artifact-oct03.json')
checks = {
    'android263Cases': len(tests)==10 and sum(len(t['cases']) for t in tests)==263 and all(t['passed'] for t in tests),
    'unreal9Suites': auto['succeeded']==9 and auto['failed']==0 and auto['succeededWithWarnings']==0,
    'liveServiceCompleted': before['service']['success'] and all(before['service']['objectives']),
    'livePlatformDwell': before['service']['dwellTicks']==300 and abs(before['service']['berthError'])<2,
    'settledColdRecoveryExact': before['logicalFingerprint']==after['logicalFingerprint'] and before['service']==after['service'],
    'legacyRecovery': old['content']=='IR-S6-fixture-1' and old['logicalFingerprint']=='0efccd7c539ec58a',
    'incompatibleSlotsUnchanged': all(read(E/'Device/incompatible-slots/verify.json').values()),
    'incompatibleOriginalsRestored': all(read(E/'Device/incompatible-slots/restore.json').values()),
    'pairedMeasurement': {r['variant'] for r in measurements if r['passed']}=={'WAP7','baseline'},
    'apkHash': hashlib.sha256(Path(artifact['path']).read_bytes()).hexdigest().upper()==artifact['sha256'],
    'oldMapUnchanged': hashlib.sha256((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/S7/L_RailService.umap').read_bytes()).hexdigest().upper()==read(E/'source-map-before.json')['Hash'],
    'phonePowerSettingRestored': read(E/'Device/power-setting.json')['restored'],
}
report = {'integrationValidated':all(checks.values()),'checks':checks,'artifact':artifact,
          'validMeasuredRuns':len(measurements),'artReleaseAccepted':False,
          'limits':['Single valid paired capture, not a sustained thermal budget',
                    'Earlier intermittent engine-startup crash remains unproven fixed',
                    'Source art and WAP-7 prototype fidelity remain limited'],
          'report':'07_Documentation/S8/LHB_VALIDATION_REPORT.md'}
(E/'final-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
raise SystemExit(0 if report['integrationValidated'] else 1)
