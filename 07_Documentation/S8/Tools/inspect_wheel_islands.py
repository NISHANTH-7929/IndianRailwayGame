"""Read-only Blender mesh island audit of retained WAP7 source."""
import bpy,json
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[3]
bpy.ops.wm.open_mainfile(filepath=str(root/'04_Assets/S8/Acquired/WAP7_Selected/Prepared/WAP7_Selected.blend'),load_ui=False,use_scripts=False)
for obj in bpy.data.objects:
 if obj.type!='MESH' or 'Bogie' not in obj.name: continue
 mesh=obj.data; neighbors=[[] for _ in mesh.vertices]
 for e in mesh.edges:
  a,b=e.vertices;neighbors[a].append(b);neighbors[b].append(a)
 seen=set(); rows=[]
 for seed in range(len(neighbors)):
  if seed in seen:continue
  ids=[];todo=[seed];seen.add(seed)
  while todo:
   i=todo.pop();ids.append(i)
   for j in neighbors[i]:
    if j not in seen:seen.add(j);todo.append(j)
  pts=[obj.matrix_world@mesh.vertices[i].co for i in ids]
  lo=[min(p[d] for p in pts) for d in range(3)];hi=[max(p[d] for p in pts) for d in range(3)]
  if hi[0]-lo[0]>.8 and hi[2]-lo[2]>.8 and hi[2]<1.3:rows.append({'seed':seed,'n':len(ids),'centre':[(lo[d]+hi[d])/2 for d in range(3)],'size':[hi[d]-lo[d] for d in range(3)]})
 print('WHEEL_ISLANDS',obj.name,json.dumps(sorted(rows,key=lambda r:-r['n'])))
