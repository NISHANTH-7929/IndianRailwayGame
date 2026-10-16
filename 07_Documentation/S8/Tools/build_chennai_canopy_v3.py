"""Original dense folded foliage, retained V2 is untouched; Blender background."""
import bpy, math, random, json, hashlib
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[3];out=root/'04_Assets/S8/ChennaiFoliageV3';out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
rng=random.Random(1301033);verts=[];faces=[];colors=[]
# A dense, uneven leaf envelope avoids the bare twig appearance of sparse V2.
# Folded geometry is two-sided in-engine and has no transparency overdraw.
for n in range(260):
 z=rng.uniform(-1,1);a=rng.uniform(0,math.tau);r=rng.uniform(.24,.43)
 p=Vector((math.sqrt(1-z*z)*math.cos(a),math.sqrt(1-z*z)*math.sin(a),z*.72))*r
 d=Vector((math.cos(a+rng.uniform(-1,1)),math.sin(a+rng.uniform(-1,1)),rng.uniform(-.5,.5))).normalized();c=d.cross(Vector((0,0,1))).normalized()
 length=rng.uniform(.18,.28);width=length*rng.uniform(.45,.7);start=len(verts)
 verts.extend([p-d*length*.5,p-c*width*.5,p+Vector((0,0,.018)),p+d*length*.5,p+c*width*.5])
 for f in ((0,1,2),(1,3,2),(3,4,2),(4,0,2)):
  faces.append(tuple(start+i for i in f));t=rng.uniform(.65,1);colors.append((t,t,t,1))
mesh=bpy.data.meshes.new('DenseFoldedCanopy');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('SM_ChennaiDenseCanopy',mesh);bpy.context.collection.objects.link(obj)
mat=bpy.data.materials.new('DenseLeaf');mat.diffuse_color=(.12,.28,.045,1);mesh.materials.append(mat)
attr=mesh.color_attributes.new(name='LeafVariation',type='FLOAT_COLOR',domain='CORNER')
for p,c in zip(mesh.polygons,colors):
 for i in p.loop_indices:attr.data[i].color=c
obj.select_set(True);bpy.context.view_layer.objects.active=obj
fbx=out/'SM_ChennaiDenseCanopy.fbx';glb=out/'SM_ChennaiDenseCanopy.glb'
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,use_triangles=True,path_mode='STRIP')
bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',use_selection=True)
(out/'provenance.json').write_text(json.dumps(dict(origin='Original local procedural geometry',seed=1301033,leaves=260,triangles=1040,dimensions_metres=list(obj.dimensions),paid_provider=False,external_pixels=False,botanical_replica=False,files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (fbx,glb)}),indent=2))
(out/'ATTRIBUTION.md').write_text('Original project-authored procedural folded-leaf geometry. No remote model, texture, font, or photographic pixels used. Representative tropical broadleaf foliage; not a certified botanical species replica. Retained V2 is unchanged.\n')
