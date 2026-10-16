"""Summarize measured PresentMon intervals; no inferred GPU/CPU frame timings."""
import csv,json,math,sys
from pathlib import Path
source=Path(sys.argv[1]);out=Path(sys.argv[2])
if out.exists(): raise SystemExit('Use fresh output')
with source.open(newline='',encoding='utf-8-sig') as f: rows=list(csv.DictReader(f))
groups={}
for row in rows:
 key=(row.get('ProcessID'),row.get('SwapChainAddress'))
 groups.setdefault(key,[]).append(row)
results=[]
for (pid,swap),group in groups.items():
 values=sorted(float(r['msBetweenPresents']) for r in group if float(r['msBetweenPresents'])>0)
 if not values: continue
 def p(q):return values[max(0,math.ceil(len(values)*q)-1)]
 results.append(dict(processId=pid,swapChain=swap,samples=len(values),meanMs=sum(values)/len(values),p50Ms=p(.5),p95Ms=p(.95),p99Ms=p(.99),maxMs=max(values),over33_33Ms=sum(v>1000/30 for v in values),over50Ms=sum(v>50 for v in values),over100Ms=sum(v>100 for v in values),presentRateFps=1000*len(values)/sum(values),dropped=sum(r.get('Dropped')=='1' for r in group)))
result=dict(source=str(source),metric='Application present intervals, not GPU execution time or guaranteed displayed FPS',qualification='Measured window only; no whole-route or hardware-floor acceptance',streams=results)
out.write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result,indent=2))
