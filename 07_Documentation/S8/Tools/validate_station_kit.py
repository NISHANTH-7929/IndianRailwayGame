"""Validate authored/imported station evidence; never infer visual acceptance."""
import json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
source=ROOT/'04_Assets/S8/StationKit'
assembly=json.loads((source/'assembly.json').read_text())
runtime=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/stations/Kit01.json').read_text())
checks={}
receipt=json.loads((ROOT/'07_Documentation/S8/Evidence/Station/unreal-import.json').read_text())
checks['import_matches_source_bytes']=bool(receipt.get('sourceHashes')) and all((source/f).is_file() and hashlib.sha256((source/f).read_bytes()).hexdigest()==h for f,h in receipt.get('sourceHashes',{}).items())
checks['26_unique_modules']=len(assembly['parts'])==len({p['name'] for p in assembly['parts']})==26
checks['source_geometry']=all((source/p['fbx']).is_file() and p['triangles']>0 for p in assembly['parts'])
checks['42_pbr_maps']=len(assembly['materials'])==14 and all(set(m['textures'])=={'BC','N','ORM'} and all((source/f).is_file() for f in m['textures'].values()) for m in assembly['materials'])
checks['runtime_matches_source']={p['name'] for p in assembly['parts']}=={p['name'] for p in runtime['parts']}
checks['imported_metric_basis']=all('imported_bounds_m' in p and all(abs(p['imported_bounds_m'][i][j]-p['bounds_m'][i][j])<.001 for i in range(2) for j in range(3)) for p in runtime['parts'])
checks['unreal_meshes']=all((ROOT/'03_UnrealProject/MyProject/Content'/p['asset'].split('.')[0].removeprefix('/Game/')).with_suffix('.uasset').is_file() for p in runtime['parts'])
checks['three_lods']=all(len(p['lod_triangles'])==3 and all(n>0 for n in p['lod_triangles']) for p in runtime['parts'])
checks['metre_dimensions']=assembly['units']=='metres' and all(all(v>0 for v in p['dimensions_m']) for p in assembly['parts'])
result={'passed':all(checks.values()),'checks':checks,'modules':len(assembly['parts']),
 'lodTriangles':[sum(p['lod_triangles'][i] for p in runtime['parts']) for i in range(3)],
 'visualAccepted':False,'androidMeasured':False,'notes':'Geometry dimensions are estimates except declared gauge/platform constraints. Source and imported presence are not visual acceptance.'}
e=ROOT/'07_Documentation/S8/Evidence/Station';e.mkdir(parents=True,exist_ok=True)
(e/'content-validation.json').write_text(json.dumps(result,indent=2))
files={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file() and p.suffix in ('.fbx','.png','.json','.blend')}
(e/'source-hashes.json').write_text(json.dumps(files,indent=2))
print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
