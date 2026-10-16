"""Audit a completed raw S6 device run against the predeclared finite-run policy."""
import json
from pathlib import Path
import re
import statistics
import sys

root = Path(sys.argv[1])
completion = json.loads((root / 'completion.json').read_text(encoding='utf-8-sig'))
samples = [json.loads(line) for line in (root / 'samples.jsonl').read_text(encoding='utf-8-sig').splitlines() if line.strip()]
memory = []
for path in root.glob('memory-*.txt'):
    match = re.search(r'TOTAL PSS:\s+(\d+)', path.read_text(encoding='utf-8-sig'))
    assert match, f'Missing PSS: {path}'
    memory.append({'seconds': int(path.stem.split('-')[-1]), 'pssKiB': int(match[1])})
memory.sort(key=lambda item: item['seconds'])
thermal = []
for path in root.glob('thermal-*.txt'):
    match = re.search(r'Thermal Status:\s+(\d+)', path.read_text(encoding='utf-8-sig'))
    assert match, f'Missing thermal status: {path}'
    thermal.append(int(match[1]))
assert len(memory) >= 6 and samples
states = [sample['state'] for sample in samples]
touches = [json.loads(line) for line in (root / 'touches.jsonl').read_text(encoding='utf-8-sig').splitlines() if line.strip()]
cycles = sum(t['row'] == 2 and t['column'] == 6 for t in touches)
growth = statistics.median(m['pssKiB'] for m in memory[-3:]) - statistics.median(m['pssKiB'] for m in memory[:3])
logs = root / 'Saved' / 'Logs' / 'MyProject.log'
faults = re.findall(r'Fixed-step overload|Fatal error|Assertion failed|S4 FAULT', logs.read_text(encoding='utf-8-sig', errors='replace')) if logs.exists() else ['Missing fresh engine log']
checks = {
    'completed450Seconds': completion['complete'] and completion['seconds'] >= 450,
    'allLogicalStatesValid': all(s['logical']['valid'] and not s['logical']['faulted'] and s['logicalFingerprint'] != 'INVALID' for s in states),
    'loadedLevelsBounded': max(s['loadedLevels'] for s in states) <= 2,
    'chunkActorsBounded': max(s['chunkActors'] for s in states) <= 2,
    'proxyInstancesBounded': max(s['proxyInstances'] for s in states) <= 1024,
    'unloadObserved': any(s['loadedLevels'] == 0 for s in states),
    'irrelevantChunkUnloaded': any(s['loadedLevels'] == 1 and not s['paused'] and not s['blocked'] for s in states),
    'atLeastNinePairedCycles': cycles >= 9 and max(s['unloads'] for s in states) >= 18,
    'retiredObjectsReleasedAtEnd': states[-1]['retiredActorsAlive'] == 0 and states[-1]['retiredMeshesAlive'] == 0,
    'memoryWithinDeclaredEnvelope': growth <= 32768,
    'noFaults': not faults,
    'continuedSimulation': states[-1]['logical']['tick'] > 2000,
}
result = {
    'passed': all(checks.values()), 'checks': checks, 'completion': completion,
    'sampleCount': len(samples), 'explicitCycles': cycles, 'memory': memory,
    'pssMinKiB': min(m['pssKiB'] for m in memory), 'pssMaxKiB': max(m['pssKiB'] for m in memory),
    'medianPssGrowthKiB': growth,
    'firstThreeMedianPssKiB': statistics.median(m['pssKiB'] for m in memory[:3]),
    'lastThreeMedianPssKiB': statistics.median(m['pssKiB'] for m in memory[-3:]),
    'thermalStatuses': sorted(set(thermal)),
    'peakLoadedLevels': max(s['loadedLevels'] for s in states),
    'peakChunkActors': max(s['chunkActors'] for s in states),
    'loads': max(s['loads'] for s in states), 'unloads': max(s['unloads'] for s in states),
    'loadMsMaximum': max(max(s['loadMs']) for s in states),
    'saveMsMaximum': max(s['saveMs'] for s in states),
    'faultLogMatches': faults,
    'note': 'Finite proxy-fixture evidence only. Includes three explicit fixture resets and nine real paired unload/reload cycles. PSS includes allocator/driver caches; no final-art or nationwide-capacity claim.'
}
(root / 'residency-summary.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps(result, indent=2))
sys.exit(0 if result['passed'] else 1)
