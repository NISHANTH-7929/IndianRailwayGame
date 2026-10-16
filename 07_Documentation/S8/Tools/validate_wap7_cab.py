"""Validate cab source/runtime consistency independently of visual acceptance."""
import argparse,hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
source=ROOT/'04_Assets/S8/Cabs/WAP7'
content=ROOT/'03_UnrealProject/MyProject/Content'
a=json.loads((source/'assembly.json').read_text())
r=json.loads((content/'IndianRailway/Production/locomotives/WAP7.json').read_text())
checks={};errors=[]
def check(name,condition):
    checks[name]=bool(condition)
    if not condition:errors.append(name)
sp={p['name']:p for p in a['parts']}
rp={p['name']:p for p in r['parts'] if p.get('cab_only')}
check('unique_source_parts',len(sp)==len(a['parts']))
check('unique_runtime_parts',len(rp)==len([p for p in r['parts'] if p.get('cab_only')]))
check('same_part_set',sp.keys()==rp.keys())
check('component_budget',len(sp)==12 and len(r['parts'])==15)
check('presentation_revision',a.get('presentation_revision')==r.get('cab_presentation_revision')=='cab-stabilization-20261010')
check('source_triangle_total',a['triangles']==sum(p['triangles'] for p in sp.values()) and a['triangles']<=50000)
materials={m['name']:m for m in a['materials']}
for name,part in sp.items():
    check(name+'_source_exists',(source/part['fbx']).is_file())
    match=rp.get(name,{})
    check(name+'_transform_and_role',all(part.get(k)==match.get(k) for k in ['role','location_cm','rotation_deg','scale','rotation_axis','rotation_range_deg']))
    path=match.get('asset','')
    check(name+'_cab_asset_namespace',path.startswith('/Game/IndianRailway/S8/Locomotives/WAP7/Cab/'))
    check(name+'_unreal_asset_exists',(content/(path.split('.')[0].removeprefix('/Game/')+'.uasset')).is_file())
for name,m in materials.items():
    for key in ['detail_texture','ao_texture']:
        if key in m:check(name+'_'+key,(source/m[key]).is_file())
    if m.get('ao_texture'):check(name+'_uv_contract',m.get('ao_uv')==0 and m.get('detail_uv')==1)
    if m.get('instrument_emission'):
        check(name+'_bounded_emission',len(m['instrument_emission'])==3 and all(math.isfinite(x) and 0<=x<=1 for x in m['instrument_emission']) and 0<m['instrument_emission_strength']<=.25)
check('speed_needle_contract',sp['gauge_speed']['rotation_range_deg']==[-130,130] and sp['gauge_speed']['rotation_axis']==[-1,0,0])
check('driver_camera_matches',a['driver_camera']==r['driver_camera'] and a['driver_camera']['fov']==95)
check('no_new_pressure_or_power_channels',not any(p['role'] in ['gauge_brake','gauge_power'] for p in sp.values()))
fixture=content/'IndianRailway/S7/L_RailService.umap'
check('s7_fixture_preserved',hashlib.sha256(fixture.read_bytes()).hexdigest()=='748acd747b5e696e23531b7686932a83381a5dca39b738c9b229b24a6b175d5f')
receipt=dict(passed=not errors,checks=checks,errors=errors,source_triangles=a['triangles'],cab_parts=len(sp),total_parts=len(r['parts']),visual_accepted=False,android_validated=False)
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(receipt,indent=2))
print(json.dumps({k:v for k,v in receipt.items() if k!='checks'},indent=2))
raise SystemExit(bool(errors))
