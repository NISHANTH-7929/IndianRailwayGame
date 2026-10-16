import bpy,json
from pathlib import Path
root=Path('D:/Project/IndianRailwayGame')
bpy.ops.wm.open_mainfile(filepath=str(root/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/Prepared/LHB3A_Candidate.blend'),load_ui=False,use_scripts=False)
o=bpy.data.objects['SM_LHB3A_Body_LOD0'];stats={}
for p in o.data.polygons:
 c=p.center
 if abs(c.y)<1.4 or c.z<2 or c.z>3.4:continue
 name=o.data.materials[p.material_index].name
 row=stats.setdefault(name,{'outward':0,'inward':0,'other':0})
 row['outward' if p.normal.y*c.y>.5 else 'inward' if p.normal.y*c.y<-.5 else 'other']+=1
(root/'07_Documentation/S8/Evidence/LHB/side-face-normals.json').write_text(json.dumps(stats,indent=2))
print(stats)
