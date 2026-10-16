"""Update only two existing label materials and the presentation revision."""
import os,json,runpy,time
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/Cab/LabelReadability-20261010'
# Reuse the canonical material adapter's definitions without running its full
# mesh-import entrypoint. No geometry, LOD or control-transform mutation here.
script=Path(__file__).with_name('import_wap7_cab.py')
text=script.read_text().split('base.material_for=cab_material')[0]
namespace={'__file__':str(script),'__name__':'cab_material_adapter'}
exec(compile(text,str(script),'exec'),namespace)
d=json.loads((ROOT/'04_Assets/S8/Cabs/WAP7/assembly.json').read_text())
paths=[]
for row in d['materials']:
    if row['name'] in ['Lettering','Lettering_StaticAO']:
        m=namespace['cab_material']('WAP7/Cab',row);paths.append(m.get_path_name())
runtime=ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/locomotives/WAP7.json'
r=json.loads(runtime.read_text());r['cab_presentation_revision']=d['presentation_revision'];r['cab_reference_limits']=d['limitations'];runtime.write_text(json.dumps(r,indent=2))
(OUT/'material-import.json').write_text(json.dumps(dict(materials=paths,revision=d['presentation_revision'],geometry_reimported=False),indent=2))
os.environ['RAIL_CAB_REVIEW_ONLY']='1'
os.environ['RAIL_CAB_REVIEW_OUT']=str(OUT/'AfterReview')
runpy.run_path(str(Path(__file__).with_name('review_wap7_instruments.py')),run_name='__main__')
