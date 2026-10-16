"""Compare native captured PCM levels, not perceived loudness or speaker output."""
import json
import math
from pathlib import Path
import sys

baseline = json.loads(Path(sys.argv[1]).read_text())
candidate = json.loads(Path(sys.argv[2]).read_text())
gain = 20 * math.log10(candidate['rms'] / baseline['rms'])
result = dict(rmsGainDb=gain, peak=candidate['peak'], rms=candidate['rms'],
              clippedSamples=candidate['clippedSamples'],
              passed=gain >= 10 and candidate['clippedSamples'] == 0 and candidate['peak'] < .95,
              scope='Same staged motion scenario, aggregate PCM comparison; not LUFS or listening acceptance.')
Path(sys.argv[3]).write_text(json.dumps(result, indent=2))
print(json.dumps(result))
sys.exit(0 if result['passed'] else 1)
