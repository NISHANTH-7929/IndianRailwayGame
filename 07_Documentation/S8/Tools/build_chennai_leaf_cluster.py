"""Original deterministic leaf geometry, no photographs or remote generation."""
import bpy,math,random,json
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[3];out=root/'04_Assets/S8/ChennaiFoliageV2';out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
rng=random.Random(1301026);verts=[];faces=[];colors=[]
for n in range(160):
 z=rng.uniform(-1,1);a=rng.uniform(0,math.tau);radius=rng.random()**(1/3)*.42
 p=Vector((math.sqrt(1-z*z)*math.cos(a),math.sqrt(1-z*z)*math.sin(a),z))*radius
 direction=Vector((rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-.4,.5))).normalized();cross=direction.cross(Vector((0,0,1))).normalized()
 length=rng.uniform(.09,.16);width=length*rng.uniform(.35,.55);start=len(verts)
 verts.extend([p-direction*length*.5,p-cross*width*.5,p+Vector((0,0,.012)),p+direction*length*.5,p+cross*width*.5])
 for ids in ((0,1,2),(1,3,2),(3,4,2),(4,0,2)):
  faces.append(tuple(start+i for i in ids));t=rng.uniform(.58,1.0);colors.append((t*.8,t,t*.55,1))
mesh=bpy.data.meshes.new('LeafCluster');mesh.from_pydata(verts,[],faces);mesh.update();obj=bpy.data.objects.new('SM_ChennaiLeafCluster',mesh);bpy.context.collection.objects.link(obj)
mat=bpy.data.materials.new('LeafSurface');mat.diffuse_color=(.10,.22,.035,1);mesh.materials.append(mat)
attr=mesh.color_attributes.new(name='LeafVariation',type='FLOAT_COLOR',domain='CORNER')
for poly,c in zip(mesh.polygons,colors):
 for i in poly.loop_indices:attr.data[i].color=c
obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.fbx(filepath=str(out/'SM_ChennaiLeafCluster.fbx'),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,use_triangles=True,path_mode='STRIP')
(out/'provenance.json').write_text(json.dumps(dict(origin='Original project procedural folded leaf cluster',seed=1301026,leaves=160,triangles=640,external_pixels=False,paid_provider=False,bounds='approximately 1 metre diameter',botanical_replica=False),indent=2))
