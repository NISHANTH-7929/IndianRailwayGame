import json
import pathlib
import re
import sys

folder = pathlib.Path(sys.argv[1])
samples = [json.loads(p.read_text(encoding='utf-8-sig')) for p in sorted((folder/'samples').glob('*.json'), key=lambda p: int(p.stem))]
assert samples, 'No device samples'
last = samples[-1]
service = last['service']
def by_id(items, value):
    return next(item for item in items if item['id'] == value)

holds = [s for s in samples if not by_id(s['logical']['trains'], 2)['routeGranted']
         and by_id(s['logical']['routes'], 1)['owner'] == 1
         and not by_id(s['logical']['signals'], 2)['proceed']]
clears = [s for s in samples if s['service']['trafficCleared']
          and by_id(s['logical']['trains'], 2)['routeGranted']
          and any(a['train'] == 1 and a['state'] == 'Completed' for a in s['logical']['ai'])]
memory = []
thermal = []
for p in folder.glob('memory-*.txt'):
    match = re.search(r'TOTAL PSS:\s+(\d+)', p.read_text())
    if match:
        memory.append(int(match[1])/1024)
for p in folder.glob('thermal-*.txt'):
    match = re.search(r'Thermal Status:\s+(\d+)', p.read_text())
    if match:
        thermal.append(int(match[1]))
checks = {
    'railwayAlwaysValid': all(s['logical']['valid'] and not s['logical']['faulted'] for s in samples),
    'ticksMonotonic': all(a['logical']['tick'] <= b['logical']['tick'] for a,b in zip(samples,samples[1:])),
    'observedActualAIHold': bool(holds),
    'observedAIRearClearAndPlayerGrant': bool(clears),
    'allObjectivesMet': service['objectives'] == [1]*9,
    'completeDwell': service['dwellTicks'] == 300,
    'validBerth': abs(service['berthError']) <= 2,
    'departureAuthorized': service['departureAuthorized'],
    'successfulResult': service['phase'] == 'Result' and service['success'],
    'trafficExcludedFromLateness': abs(service['playerDelaySeconds'] - max(0,(service['departureTick']-service['trafficTicks'])/30-95)) < 1e-8,
}
result = dict(source=str(folder), samples=len(samples), checks=checks,
              stationStatesObserved=sorted({s['service']['station'] for s in samples}),
              firstTick=samples[0]['logical']['tick'], lastTick=last['logical']['tick'],
              result=service, pssMiB=memory, thermalStatus=thermal,
              note='A missing observed phase is not inferred from sparse samples. Host tests cover every fixed-step station transition.')
(folder/'analysis.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
