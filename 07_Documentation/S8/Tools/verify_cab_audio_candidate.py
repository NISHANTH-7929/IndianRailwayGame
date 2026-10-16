"""Source preservation and audio payload verification for the combined candidate."""
import json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/Cab/LabelReadability-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
checks={}
checks['static_core_fbx_unchanged']=sha(OUT/'Before/SM_WAP7_Cab_StaticCore.fbx')==sha(ROOT/'04_Assets/S8/Cabs/WAP7/Parts/SM_WAP7_Cab_StaticCore.fbx')
old=json.loads((OUT/'Before/WAP7.json').read_text())
new=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/locomotives/WAP7.json').read_text())
checks['all_runtime_parts_unchanged']=old['parts']==new['parts']
checks['driver_camera_unchanged']=old['driver_camera']==new['driver_camera']
content=ROOT/'03_UnrealProject/MyProject/Content'
checks['all_cab_meshes_unchanged']=all(sha(p)==sha(content/'IndianRailway/S8/Locomotives/WAP7/Cab/Meshes'/p.name) for p in (OUT/'Before/UnrealCab/Meshes').glob('*.uasset'))
checks['s7_fixture_unchanged']=sha(content/'IndianRailway/S7/L_RailService.umap')=='748acd747b5e696e23531b7686932a83381a5dca39b738c9b229b24a6b175d5f'
audio=json.loads((ROOT/'05_Audio/Pixabay_20261009/manifest.json').read_text())
for row in audio['clips']:
    for key in ['original','prepared']:
        checks[row['name']+'_'+key]=sha(ROOT/'05_Audio'/row[key])==row[key+'Sha256']
    checks[row['name']+'_unreal_exists']=(content/(row['asset'].removeprefix('/Game/')+'.uasset')).is_file()
audio_files=list((ROOT/'05_Audio/Pixabay_20261009/Prepared').glob('*.wav'))+list((ROOT/'03_UnrealProject/MyProject/Source/MyProject').glob('RailServiceAudio*'))
receipt=dict(passed=all(checks.values()),checks=checks,audio_hashes={str(p.relative_to(ROOT)):sha(p) for p in audio_files},cab_presentation_revision=new['cab_presentation_revision'],visual_accepted=False,android_verified=False)
(OUT/'combined-source-verification.json').write_text(json.dumps(receipt,indent=2))
print(json.dumps(receipt,indent=2))
raise SystemExit(not receipt['passed'])
