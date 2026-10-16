from pathlib import Path
import json, hashlib

ROOT=Path(__file__).resolve().parents[1]; V=ROOT/'v1.0.0'
required=[V/'ChennaiEgmore_StationArt_v1.0.0.blend',V/'exports/ChennaiEgmore_StationArt_v1.0.0.fbx',V/'exports/ChennaiEgmore_StationArt_v1.0.0.glb',V/'placement_manifest.json']
errors=[]
for p in required:
    if not p.exists() or p.stat().st_size<100: errors.append(f'missing/empty: {p}')
m=json.loads((V/'placement_manifest.json').read_text(encoding='utf-8'))
ids=[p['stableId'] for p in m['placements']]
if len(ids)!=len(set(ids)): errors.append('duplicate stableId')
if m['trackGaugeM']!=1.676: errors.append('gauge contract changed')
pngs=list((V/'previews').glob('*.png'))
if len(pngs)<3: errors.append('fewer than 3 previews')
textures=list((V/'textures').glob('T_*_*.png'))
if len(textures)!=30: errors.append(f'expected 30 texture maps, got {len(textures)}')
hashes=[]
for p in sorted([q for q in V.rglob('*') if q.is_file() and q.name not in ('SHA256SUMS.json','validation.json')]):
    hashes.append({'path':p.relative_to(V).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(V/'SHA256SUMS.json').write_text(json.dumps(hashes,indent=2),encoding='utf-8')
report={'status':'PASS' if not errors else 'FAIL','errors':errors,'assetFamilyId':m['assetFamilyId'],'version':m['version'],'placements':len(ids),'lodPolicy':m['lodPolicy'],'textures':len(textures),'previews':len(pngs),'hashRecords':len(hashes),'validationBoundary':'source/export/static package only; no Unreal import, gameplay, performance or Android validation'}
(V/'validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2)); raise SystemExit(1 if errors else 0)
