"""Preserve acquired source geometry, UVs and PBR textures; partition proven wheelsets/bogies."""
import bpy,bmesh,json,math,shutil
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parent
OUT=ROOT/'Prepared';(OUT/'Parts').mkdir(parents=True,exist_ok=True);(OUT/'Textures').mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'WAP7_Original.blend'))
source=[o for o in bpy.context.scene.objects if o.type=='MESH']
rotation=Matrix.Rotation(math.pi/2,4,'Z')
# Retain raw glTF material semantics for Unreal rather than approximating from FBX.
gltf=json.loads((ROOT/'Original/uvwag9.gltf').read_text())
def texture(t):
 t=dict(t); uri=gltf['images'][gltf['textures'][t.pop('index')]['source']]['uri'];shutil.copy2(ROOT/'Original'/uri,OUT/'Textures'/Path(uri).name);t['uri']='Textures/'+Path(uri).name;return t
materials=[]
for original in gltf.get('materials',[]):
 m=json.loads(json.dumps(original));pbr=m.setdefault('pbrMetallicRoughness',{})
 for container in [m,pbr]:
  for k,v in list(container.items()):
   if k.endswith('Texture'):container[k]=texture(v)
 m['base_color']=pbr.get('baseColorFactor',[1,1,1,1]);materials.append(m)
# Source has unassigned slots. Explicit default matches glTF default material.
default=bpy.data.materials.new('Acquired_Default');default.use_nodes=True
materials.append({'name':default.name,'pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1],'metallicFactor':1,'roughnessFactor':1},'base_color':[1,1,1,1]})
groups={};wheelcenters={}
for o in source:
 mesh=o.data;ps=[o.matrix_world@v.co for v in mesh.vertices];parent=list(range(len(ps)))
 def root(a):
  while parent[a]!=a:parent[a]=parent[parent[a]];a=parent[a]
  return a
 for p in mesh.polygons:
  a=root(p.vertices[0])
  for v in p.vertices[1:]:parent[root(v)]=a
 islands={}
 for i in range(len(ps)):islands.setdefault(root(i),[]).append(i)
 labels={}
 for index,ids in enumerate(islands.values()):
  key='Exterior'
  if o.name=='Cube.021':
   lo=Vector([min(ps[v][i] for v in ids) for i in range(3)]);hi=Vector([max(ps[v][i] for v in ids) for i in range(3)]);center=(lo+hi)/2
   if index in range(50,56):key='Wheelset_'+str(index-49);wheelcenters[key]=rotation@center
   elif 2.65<abs(center.y)<9.05 and hi.z<1.55:key='Bogie_Front' if center.y<0 else 'Bogie_Rear'
  for v in ids:labels[v]=key
 for p in mesh.polygons:groups.setdefault(labels[p.vertices[0]],{}).setdefault(o.name,[]).append(p.index)
parts=[];created=[];source_by_name={o.name:o for o in source}
for label,entries in groups.items():
 verts=[];faces=[];uvs=[];normals=[];mats=[];face_materials=[];smooth=[]
 for name,polys in entries.items():
  o=source_by_name[name];me=o.data;transform=rotation@o.matrix_world;normalmatrix=transform.to_3x3().inverted().transposed();mapping={}
  for pi in polys:
   p=me.polygons[pi];face=[]
   for vi in p.vertices:
    if vi not in mapping:mapping[vi]=len(verts);verts.append(transform@me.vertices[vi].co)
    face.append(mapping[vi])
   faces.append(face);smooth.append(p.use_smooth)
   mat=me.materials[p.material_index] if len(me.materials)>p.material_index else default
   if mat is None:mat=default
   if mat not in mats:mats.append(mat)
   face_materials.append(mats.index(mat))
   for li in p.loop_indices:
    uvs.append(tuple(me.uv_layers.active.data[li].uv) if me.uv_layers.active else (0,0))
    normals.append(tuple((normalmatrix@me.corner_normals[li].vector).normalized()))
 pivot=wheelcenters.get(label,Vector((0,0,0)))
 if label.startswith('Bogie'):pivot=Vector((5.85 if label.endswith('Front') else -5.85,0,.594))
 verts=[v-pivot for v in verts]
 mesh=bpy.data.meshes.new(label);mesh.from_pydata(verts,[],faces);mesh.update()
 for m in mats:mesh.materials.append(m)
 uv=mesh.uv_layers.new(name='UVMap')
 for i,v in enumerate(uvs):uv.data[i].uv=v
 for i,p in enumerate(mesh.polygons):p.material_index=face_materials[i];p.use_smooth=smooth[i]
 mesh.normals_split_custom_set(normals)
 ob=bpy.data.objects.new('SM_WAP7_Acquired_'+label+'_LOD0',mesh);bpy.context.collection.objects.link(ob)
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
 filename=ob.name+'.fbx'
 bpy.ops.export_scene.fbx(filepath=str(OUT/'Parts'/filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,path_mode='STRIP')
 ob.location=pivot;created.append(ob);mesh.calc_loop_triangles()
 role='wheelset' if label.startswith('Wheel') else 'bogie' if label.startswith('Bogie') else 'exterior'
 parts.append({'name':label,'role':role,'fbx':'Parts/'+filename,'object':ob.name,'location_cm':[v*100 for v in pivot],'pivot_cm':[v*100 for v in pivot],'rotation_deg':[0,0,0],'scale':[1,1,1],'rotation_axis':[0,1,0] if role=='wheelset' else [0,0,1],'triangles':len(mesh.loop_triangles),'material_slots':len(mats)})
for o in source:bpy.data.objects.remove(o,do_unlink=True)
for im in bpy.data.images:
 if im.source=='FILE':im.pack()
manifest={'schema_version':1,'family':'WAP7','source':'Sketchfab 1935cec9ac9446e3b2524ad73f0902a4 by knitro_vedant','units':'centimetres','axis':'+X forward +Y right +Z up','fbx_import_settings':{'convert_scene':True,'force_front_x_axis':True,'convert_scene_unit':True},'parts':parts,'materials':materials,'driver_camera':{'location_cm':[880,-78,290],'rotation_deg':[0,0,0],'fov':75},'wheel_radius_cm':58.35094,'cab_available':False,'limitations':['Exterior source has no furnished interactive cab. Camera is provisional and must not imply cab completion.','Pantographs, wipers and lights retained as authored static geometry; not rigged.','Full original mesh and textures retained; Android cost unmeasured.'],'triangles':sum(p['triangles'] for p in parts)}
assert manifest['triangles']==225435,manifest['triangles']
(OUT/'assembly.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_Acquired.blend'))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.22,.25,.3,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.6
center=Vector((0,0,2.9526976));extent=21.3277587890625
for offset,power,size in [((.3,-.4,1),1500,.7),((-.4,.5,.5),1000,.5)]:
 bpy.ops.object.light_add(type='AREA',location=center+rotation.to_3x3()@Vector(offset)*extent);light=bpy.context.object;light.data.energy=power*extent*extent/100;light.data.shape='DISK';light.data.size=extent*size;light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=center+rotation.to_3x3()@Vector((.85,-1.1,.65))*extent);camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=extent*1.2;camera.data.clip_end=extent*20;scene.camera=camera
scene.render.filepath=str(OUT/'WAP7_Acquired_preview.png');bpy.ops.render.render(write_still=True)
print('PREPARED',manifest['triangles'],len(parts))

