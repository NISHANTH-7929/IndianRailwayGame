"""Compare device checkpoints against the pre-install backup, without changing them."""
import argparse, json, subprocess
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--evidence', type=Path, required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
expected = json.loads((a.evidence/'checkpoint-hashes-before.json').read_text())
base = '/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/MyProject/Saved/'
raw = subprocess.run(['adb','shell','sha256sum',*[base+x for x in expected]],
                     capture_output=True, text=True, timeout=30, check=True).stdout
actual = {}
for line in raw.splitlines():
    digest, name = line.split(None, 1)
    actual[name.strip().removeprefix(base)] = digest
result = {'checked':len(expected), 'changed':[k for k,v in expected.items() if actual.get(k)!=v]}
result['passed'] = bool(expected) and not result['changed']
(a.evidence/a.output).write_text(json.dumps(result,indent=2))
print(json.dumps(result))
raise SystemExit(not result['passed'])
