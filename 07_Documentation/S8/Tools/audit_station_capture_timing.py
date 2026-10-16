"""Reject late fixed-station review views; does not establish visual acceptance."""
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[3]
source = root / '03_UnrealProject/MyProject/Source/MyProject/Simulation/RailTambaramAlignment.generated.h'
stations = [float(v) for v in re.search(r'Stations\{\{([^}]+)', source.read_text()).group(1).split(',')]
run = Path(sys.argv[1]).resolve()
samples = []
for path in sorted((run / 'native').glob('*-frame.json')):
    frame = json.loads(path.read_text(encoding='utf-8-sig'))
    match = re.search(r'station-(\d+)$', frame['label'])
    if not match:
        continue
    index = int(match.group(1))
    delta = frame['headMetres'] - stations[index]
    # Trigger is station-120m. Keep station +/-250m inside the player's
    # rear-360 / head+720 corridor, with margin for the ~190m formation.
    samples.append(dict(station=index, headOffsetMetres=delta,
                        passed=-130 <= delta <= 250))
passed = sorted(s['station'] for s in samples) == list(range(1, 14)) and all(s['passed'] for s in samples)
result = dict(passed=passed, samples=samples,
              scope='Capture scheduling only; inspect rendered ground/track coverage separately.')
(run / 'station-capture-timing.json').write_text(json.dumps(result, indent=2))
print(json.dumps(result))
sys.exit(0 if passed else 1)
