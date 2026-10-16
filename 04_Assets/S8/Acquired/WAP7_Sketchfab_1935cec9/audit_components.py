import bpy,json
from pathlib import Path
from mathutils import Vector
r=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(r/'WAP7_Original.blend'))
report=[]
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 ps=[o.matrix_world@v.co for v in o.data.vertices]; parent=list(range(len(ps)))
 def root(a):
  while parent[a]!=a:parent[a]=parent[parent[a]];a=parent[a]
  return a
 for p in o.data.polygons:
  a=root(p.vertices[0])
  for v in p.vertices[1:]:parent[root(v)]=a
 groups={}
 for i in range(len(ps)):groups.setdefault(root(i),[]).append(i)
 for idx,ids in enumerate(groups.values()):
  lo=[min(ps[v][i] for v in ids) for i in range(3)];hi=[max(ps[v][i] for v in ids) for i in range(3)]
  report.append({'name':o.name,'island':idx,'lo':lo,'hi':hi,'n':len(ids)})
(r/'components.json').write_text(json.dumps(report,indent=2))
