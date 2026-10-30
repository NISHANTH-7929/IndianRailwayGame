"""Diagnostic summary, explicitly distinguishing partial CSV captures."""
import csv,json,math,statistics,sys
from pathlib import Path
csv.field_size_limit(64*1024*1024)
p=Path(sys.argv[1]);rows=list(csv.reader(p.open(encoding='utf-8-sig')))
h=[r for r in rows if r and r[0]=='EVENTS'][-1]
d={'finalized':bool(rows[-1]) and rows[-1][0]=='[HasHeaderRowAtEnd]',
   'scope':'Mixed driving, pause and background intervals; not a driving-only performance qualification', 'metrics':{}}
for name in ['FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime']:
    if name not in h:continue
    ix=h.index(name);values=[]
    for row in rows:
        if not row or row[0]=='EVENTS' or row[0].startswith('['):continue
        try:value=float(row[ix])
        except (ValueError,IndexError):continue
        if math.isfinite(value) and value>=0:values.append(value)
    if values:
        values.sort();d['metrics'][name]={'samples':len(values),'mean':statistics.fmean(values),
            'p95':values[math.ceil(len(values)*.95)-1], 'p99':values[math.ceil(len(values)*.99)-1],
            'max':max(values),'over100ms':sum(v>100 for v in values)}
p.with_suffix('.diagnostic.json').write_text(json.dumps(d,indent=2))
print(json.dumps(d,indent=2))
