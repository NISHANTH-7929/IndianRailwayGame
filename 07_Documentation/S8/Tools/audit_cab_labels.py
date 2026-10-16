import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/Cab/LabelReadability-20261010'
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'04_Assets/S8/Cabs/WAP7/WAP7_Cab.blend'))
o=bpy.data.objects['SM_WAP7_Cab_StaticCore']; rows=[]
for p in o.data.polygons:
    m=o.data.materials[p.material_index].name
    if not m.startswith('Lettering'):continue
    c=o.matrix_world@p.center
    n=(o.matrix_world.to_3x3()@p.normal).normalized()
    # Cast from 2mm in front of lettering toward the camera: any hit means
    # that some other cab surface occludes this label from its normal direction.
    hit,loc,norm,index=o.ray_cast(o.matrix_world.inverted()@(c+Vector((-.002,0,0))),Vector((-1,0,0)),distance=.5)
    rows.append(dict(index=p.index,material=m,centre=list(c),normal=list(n),area=p.area,blocked=hit,blockingMaterial=o.data.materials[o.data.polygons[index].material_index].name if hit else None))
(OUT/'label-geometry-audit.json').write_text(json.dumps(rows,indent=2))
print('LABEL_AUDIT',len(rows),'blocked',sum(r['blocked'] for r in rows),'backfacing',sum(r['normal'][0]>0 for r in rows))
