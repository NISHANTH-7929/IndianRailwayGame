"""Validate the additive Scene02/interior import boundary before packaging."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
CONTENT=ROOT/'03_UnrealProject/MyProject/Content'
checks={};ids=set();count=0
for family,identity,version,n in [('EgmoreScene02','Station.Egmore.Scene.02','2.0.0',1259),('EgmoreInterior01','Station.Egmore.Interior.01','2.1.0',57)]:
    data=json.loads((CONTENT/'IndianRailway/Production/stations'/(family+'.json')).read_text())
    receipt=json.loads((ROOT/'07_Documentation/S8/Evidence'/family/'unreal-import.json').read_text())
    checks[family+'_identity']=data['assetFamilyId']==identity and data['version']==version
    checks[family+'_import']=receipt['passed'] and len(data['importedModules'])==len(data['modules'])
    checks[family+'_bounds']=all(r['boundsErrorM']<.001 for r in receipt['modules'])
    lookup={r['module']:r for r in data['importedModules']}
    checks[family+'_lods']=all(lookup[m['module']]['lodTriangles']==[l['triangles'] for l in m['lods']] for m in data['modules'])
    checks[family+'_placements']=len(data['placements'])==n and all(p['module'] in lookup for p in data['placements'])
    for p in data['placements']:ids.add(p['stableId']);count+=1
    checks[family+'_assets_exist']=all((CONTENT/(r['asset'].split('.')[0].removeprefix('/Game/')+'.uasset')).is_file() for r in lookup.values())
checks['unique_placement_ids']=count==1316 and len(ids)==count
fixture=CONTENT/'IndianRailway/S7/L_RailService.umap'
checks['s7_fixture_unchanged']=hashlib.sha256(fixture.read_bytes()).hexdigest()=='748acd747b5e696e23531b7686932a83381a5dca39b738c9b229b24a6b175d5f'
checks['review_map_separate']=(CONTENT/'IndianRailway/S8/Review/L_EgmoreScene02_Study.umap').is_file()
result=dict(passed=all(checks.values()),checks=checks,placement_count=count,scope='static scene import; no gameplay, phone, lighting or visual acceptance asserted')
(ROOT/'07_Documentation/S8/Evidence/EgmoreScene02/source-runtime-validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
