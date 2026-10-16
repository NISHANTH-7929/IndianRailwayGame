import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[3]
bpy.ops.wm.open_mainfile(filepath=str(root/'04_Assets/S8/LHBUnderframeV2/LHB_UnderframeV2_Refined.blend'),load_ui=False,use_scripts=False)
o=bpy.data.objects['SM_LHB_BodyV2']
rows=[]
for side in (-1,1):
 for x in (-6.76824,-3.53816,6.33056):
  hit,p,n,index=o.ray_cast(Vector((x,side*5,2.58)),Vector((0,-side,0)))
  rows.append(dict(side=side,x=x,hit=hit,position=list(p),normal=list(n),face=index,material=o.data.materials[o.data.polygons[index].material_index].name if hit else None))
print('VISIBLE_GLASS_AUDIT '+json.dumps(rows))
passed=all(r['hit'] and r['material']=='LHB_GlassV2' and r['normal'][1]*r['side']>.9 for r in rows)
(root/'07_Documentation/S8/Evidence/EgmoreTambaram/lhb-visible-glass-audit.json').write_text(json.dumps(dict(passed=passed,samples=rows),indent=2)+'\n')
assert passed, 'Outer visible panes must have glass material and outward normals'
