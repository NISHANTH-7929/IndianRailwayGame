"""Read-only baseline geometry selection audit; run with Blender --python."""
import bpy, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
E=ROOT/'07_Documentation/S8/Evidence/Stabilization-20261010'
bpy.ops.wm.open_mainfile(filepath=str(E/'Before/WAP7/WAP7_Cab.blend'))
result=[]
for o in bpy.data.objects:
    if o.type!='MESH':continue
    mesh=o.data;world=[o.matrix_world@v.co for v in mesh.vertices]
    selected=[9.015<v.x<9.255 and 2.78<v.z<3.235 for v in world]
    adjacency=[[] for v in mesh.vertices]
    for edge in mesh.edges:
        a,b=edge.vertices;adjacency[a].append(b);adjacency[b].append(a)
    seen=set();components=[]
    for index in range(len(world)):
        if index in seen:continue
        seen.add(index);pending=[index];component=[]
        while pending:
            i=pending.pop();component.append(i)
            for j in adjacency[i]:
                if j not in seen:seen.add(j);pending.append(j)
        components.append(component)
    material_by_vertex=[set() for v in mesh.vertices]
    for p in mesh.polygons:
        name=mesh.materials[p.material_index].name if p.material_index<len(mesh.materials) else '?'
        for i in p.vertices:material_by_vertex[i].add(name)
    for component in components:
        count=sum(selected[i] for i in component)
        coords=[world[i] for i in component]
        low=[min(v[k] for v in coords) for k in range(3)]
        high=[max(v[k] for v in coords) for k in range(3)]
        # Include all nearby panel/label components, selected or not, to expose
        # labels left in front of a moving panel and discontinuous shearing.
        if high[0]<8.99 or low[0]>9.30 or high[2]<2.78 or low[2]>3.25:continue
        materials=sorted(set.union(*(material_by_vertex[i] for i in component)))
        result.append(dict(object=o.name,vertices=len(component),selected=count,
                           classification='partial' if 0<count<len(component) else ('full' if count else 'none'),
                           bounds_min=low,bounds_max=high,materials=materials))
report={'components':result,'partial_components':sum(r['classification']=='partial' for r in result)}
(E/'cab-tilt-audit.json').write_text(json.dumps(report,indent=2))
print('CAB_TILT_AUDIT',json.dumps(report))
