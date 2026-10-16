"""Summarize completed LHB captures without treating missing evidence as a pass."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / '07_Documentation/S8/Evidence/LHB'
results = []
for folder in sorted((EVIDENCE / 'Device').iterdir()):
    if (folder / 'exclusion.json').is_file():
        continue
    if not (folder / 'completion.json').is_file():
        continue
    completion = json.loads((folder / 'completion.json').read_text())
    if not completion.get('complete'):
        continue
    measurement = json.loads((folder / 'measurement.json').read_text())
    samples = json.loads((folder / 'samples.json').read_text())
    restoration = json.loads((folder / 'restoration.json').read_text())
    # The CSV analysis excludes startup and export. Telemetry times begin at
    # activity launch; this conservative subset stays before CSV finalization.
    states = [s['state'] for s in samples if 10 < s['seconds'] <= 170]
    ticks = [s['logical']['tick'] for s in states]
    checks = {
        'logicalValid': all(s['logical']['valid'] and not s['logical']['faulted'] for s in states),
        'running': all(not s['paused'] and not s['blocked'] for s in states),
        'ticksAdvance': all(b > a for a, b in zip(ticks, ticks[1:])),
        'contentIdentity': all(s['content'] == 'IR-S8-LHB-3A-1' for s in states),
        'oldSavesUnchanged': restoration['normalCheckpointsUnchanged'],
        'commandlineRestored': restoration['commandlineRestored'],
    }
    if completion['variant'] == 'WAP7':
        checks['coachesPresent'] = all(s['lhbVehicles'] == 8 and s['lhbComponents'] == 14 for s in states)
    results.append({'label': folder.name, 'variant': completion['variant'],
                    'checks': checks, 'passed': all(checks.values()),
                    'firstTick': ticks[0], 'lastTick': ticks[-1],
                    'pausedSamplesAfterWindow': [s['seconds'] for s in samples if s['seconds'] > 170 and s['state']['paused']],
                    'frames': measurement['frames'], 'analysisFrames': measurement['analysisFrames'],
                    'metrics': measurement['metrics']})
output = {'runs': results, 'note': 'Per-run engine intervals; no compositor or sustained-session claim. Load latency includes repeated telemetry values.'}
(EVIDENCE / 'android-measurements.json').write_text(json.dumps(output, indent=2) + '\n')
for r in results:
    m = r['metrics']
    print(r['label'], 'valid=', r['passed'], 'FPS=', m['FPS'].get('value'),
          'p95=', m['FrameTime'].get('p95'), 'maxPSSMiB=', m['PSS'].get('maximum', 0)/1024)
if not results or not all(r['passed'] for r in results):
    raise SystemExit(1)
