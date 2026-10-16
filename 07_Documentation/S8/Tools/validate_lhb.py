"""Validate candidate content metadata; acceptance and device cost remain separate."""
import json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
BASE=ROOT/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431'
a=json.loads((BASE/'Prepared/assembly.json').read_text(encoding='utf-8-sig'))
r=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/coaches/LHB3A.json').read_text(encoding='utf-8-sig'))
p=json.loads((BASE/'provenance.json').read_text(encoding='utf-8-sig'))
checks={}
checks['content_identity']=a['content_id']==r['content_id']=='IR-S8-LHB-3A-1' and a['definition_id']==r['definition_id']==3001
checks['version']=a['version']==r['version']==1
checks['dimensions']=a['body_length_m']==23.54 and a['coupled_length_m']==24 and a['coupling_gap_m']==0 and a['bogie_offsets_m']==[7.45,-7.45]
parts={part['name']:part for part in a['parts']}
checks['parts']=set(parts)=={'Body','Bogie0','Bogie1','Wheel0','Wheel1','Wheel2','Wheel3'}
checks['body_envelope']=all(abs(parts['Body']['local_bounds_m'][i][0])<=12.001 for i in [0,1])
checks['bogie_pivots']=all(abs(parts['Bogie'+str(i)]['location_cm'][0]-(-745 if i==0 else 745))<.001 for i in range(2))
checks['wheelbase']=all(abs(parts['Wheel'+str(i+1)]['location_cm'][0]-parts['Wheel'+str(i)]['location_cm'][0]-256)<.001 for i in [0,2])
checks['wheel_height']=all(abs(parts['Wheel'+str(i)]['location_cm'][2]-45.75)<.001 for i in range(4))
checks['sources_exist']=all((BASE/'Prepared'/part['fbx']).is_file() for part in a['parts'])
checks['unreal_assets_exist']=all((ROOT/'03_UnrealProject/MyProject/Content'/part['asset'].split('.')[0].replace('/Game/','')).with_suffix('.uasset').is_file() for part in r['parts'])
checks['lods']=len(r['validation'])==7 and all(v['lod_count']==3 and len(v['lod_triangles'])==3 and all(t>0 for t in v['lod_triangles']) for v in r['validation'])
checks['provenance']=p['AssetId']==a['asset_id'] and p['CommercialUse']=='verified' and p['ModificationPermission']=='verified' and 'user-source-confirmation.json' in p['VerificationEvidence']
device_report=ROOT/'07_Documentation/S8/Evidence/LHB/android-measurements.json'
device_runs=json.loads(device_report.read_text()).get('runs',[]) if device_report.is_file() else []
measured=any(run.get('variant')=='WAP7' and run.get('passed') for run in device_runs)
result={'passed':all(checks.values()),'checks':checks,'releaseAccepted':False,'androidMeasured':measured,
 'androidEvidence':'Evidence/LHB/android-measurements.json' if measured else None,
 'lod_triangle_totals':[sum(v['lod_triangles'][i] for v in r['validation']) for i in range(3)],
 'note':'Checks metadata and imported asset presence. Does not prove visual fidelity, legal authenticity, collision shape or on-device cost.'}
(ROOT/'07_Documentation/S8/Evidence/LHB/content-validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
raise SystemExit(0 if result['passed'] else 1)
