"""Normalize pulled on-device samples without fabricating gaps or validity."""
import json
from pathlib import Path
import sys
root=Path(sys.argv[1])
for phase in ['profile-final','sustained-final']:
    folder=root/phase
    completion=json.loads((folder/'completion.json').read_text())
    assert completion['complete']
    paths=sorted((folder/'samples').glob('*.json'),key=lambda p:int(p.stem))
    assert len(paths)==completion['samples']
    with (folder/'samples.jsonl').open('w',encoding='utf-8') as out:
        for path in paths:
            state=json.loads(path.read_text(encoding='utf-8-sig'))
            assert state['logical']['valid'] and not state['logical']['faulted'] and state['logicalFingerprint']!='INVALID',path
            out.write(json.dumps({'seconds':int(path.stem),'state':state},separators=(',',':'))+'\n')
    print(phase,completion)
