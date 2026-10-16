"""Read-only source polygon-region audit; run in Blender background."""
import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[3]
bpy.ops.wm.open_mainfile(filepath=str(root/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/Prepared/LHB3A_Candidate.blend'),load_ui=False,use_scripts=False)
o=bpy.data.objects['SM_LHB3A_Body_LOD0'];rows=[]
for i,m in enumerate(o.data.materials):
 ps=[p for p in o.data.polygons if p.material_index==i];vs=[o.data.vertices[v].co for p in ps for v in p.vertices]
 if vs:rows.append(dict(slot=i,material=m.name if m else '',faces=len(ps),bounds=[[min(v[k] for v in vs),max(v[k] for v in vs)] for k in range(3)],window_faces=sum(1 for p in ps if abs(p.center.y)>1.4 and 2<p.center.z<3.3),largest=[dict(area=p.area,center=list(p.center),normal=list(p.normal)) for p in sorted(ps,key=lambda p:p.area,reverse=True)[:3]]))
print('LHB_MATERIAL_REGIONS',json.dumps(rows))
