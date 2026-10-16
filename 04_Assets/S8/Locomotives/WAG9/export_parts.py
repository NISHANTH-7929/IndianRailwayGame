import bpy,os,json,math
from mathutils import Vector,Matrix
OUT=os.path.dirname(os.path.abspath(__file__)); SRC=os.path.join(OUT,'WAG9_S8.blend')
bpy.ops.wm.open_mainfile(filepath=SRC)
# Glass transparency in native asset and runtime manifest.
glass=bpy.data.materials.get('Blue window glass')
if glass:
 p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Alpha'].default_value=.14;p.inputs['Transmission Weight'].default_value=.6;glass.diffuse_color=(*glass.diffuse_color[:3],.23)
# Convert all geometry including labels, evaluate bevels, and group by animation ownership.
bpy.ops.object.select_all(action='DESELECT')
groups={};pivots={};roles={};meta={}
def add(key,role,o,pivot,extra=None):
 groups.setdefault(key,[]).append(o);pivots[key]=pivot;roles[key]=role
 if extra:meta[key]=extra
for o in list(bpy.context.scene.objects):
 n=o.name;x,y,z=o.location
 if o.type not in {'MESH','FONT'} or n=='Studio ground':continue
 if o.type=='FONT':
  bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');o.select_set(False)
 side='front' if x>0 else 'rear'; bx=6 if x>0 else -6
 if n.startswith(('Wheel','Axle.')) or n=='Axle':
  ax=min([bx-1.85,bx,bx+1.85],key=lambda v:abs(v-x));key='Wheelset_'+side+'_'+str(round(ax,2));add(key,'wheelset_'+side,o,(ax,0,.545),{'rotation_axis':[0,1,0]})
 elif n.startswith(('Bogie','Primary spring','Primary coil','Suspension damper','Axle box','Axle bearing')):add('Bogie_'+side,'bogie_'+side,o,(bx,0,.75))
 elif n.startswith(('Panto lower','Panto upper','Contact bow','Carbon strip')):
  stage='lower' if n.startswith('Panto lower') else 'upper' if n.startswith('Panto upper') else 'bow'
  pp={'lower':(bx-.45,0,4.56),'upper':(bx+.7,0,5.18),'bow':(bx-.25,0,5.85)}[stage]
  add('Pantograph_'+side+'_'+stage,'pantograph_'+stage,o,pp,{'side':side,'linkage':{'base_cm':[(bx-.45)*100,0,456],'elbow_cm':[(bx+.7)*100,0,518],'contact_cm':[(bx-.25)*100,0,585],'rotation_axis':[0,1,0]}})
 elif n.startswith('Windshield wiper'):add(n.replace('.','_'),'wiper',o,(x,y,z),{'rotation_axis':[1,0,0],'rotation_range_deg':[-25,25]})
 elif n.startswith('Throttle'):add('Throttle','throttle',o,(8.18,-.9,2.49),{'rotation_axis':[0,1,0],'rotation_range_deg':[-20,20]})
 elif n.startswith('TrainBrake'):add('TrainBrake','train_brake',o,(8.18,.82,2.49),{'rotation_axis':[0,1,0],'rotation_range_deg':[-20,20]})
 elif n.startswith('IndependentBrake'):add('IndependentBrake','visual_only',o,(8.18,1.12,2.49))
 elif n.startswith('Gauge needle'):add(n.replace('.','_'),'gauge_brake',o,(8.655,y,2.68),{'rotation_axis':[1,0,0],'rotation_range_deg':[-100,100]})
 elif n.startswith(('Twin headlight glass','Marker lens')):add('Lenses','headlight',o,(0,0,0))
 elif n.startswith(('Windscreen','Cab side window','Door window')) and not n.startswith('Windscreen pillar'):add('WindowGlass','glass',o,(0,0,0))
 elif any(c.name=='CAB_INTERIOR' for c in o.users_collection):add('Cab','cab',o,(0,0,0))
 else:add('Exterior','exterior',o,(0,0,0))
parts=[];folder=os.path.join(OUT,'Parts');os.makedirs(folder,exist_ok=True)
for key,objs in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 # evaluated copies freeze modifiers while preserving all material assignments
 copies=[]
 for src in objs:
  dg=bpy.context.evaluated_depsgraph_get();me=bpy.data.meshes.new_from_object(src.evaluated_get(dg));ob=bpy.data.objects.new('tmp_'+key,me);bpy.context.collection.objects.link(ob);ob.matrix_world=src.matrix_world.copy();copies.append(ob);ob.select_set(True)
 bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=key
 pivot=Vector(pivots[key]);bpy.context.scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');ob.location=(0,0,0)
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 for v in ob.data.vertices:v.co*=100
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 filename='SM_WAG9_01_'+key+'_LOD0.fbx'
 bpy.ops.export_scene.fbx(filepath=os.path.join(folder,filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False)
 verts=len(ob.data.vertices);tri=sum(len(p.vertices)-2 for p in ob.data.polygons)
 part={'name':key,'role':roles[key],'fbx':'Parts/'+filename,'location_cm':[v*100 for v in pivot],'pivot_cm':[v*100 for v in pivot],'rotation_deg':[0,0,0],'scale':[1,1,1],'triangles':tri,'material_slots':len(ob.data.materials)};part.update(meta.get(key,{}));parts.append(part);bpy.data.objects.remove(ob,do_unlink=True)
manifest={'schema_version':1,'locomotive':'WAG9','family':'WAG9','axis':'+X forward +Y right +Z up','coordinate_system':'+X forward +Y right +Z up','units':'centimeters','fbx_unit':'centimeters','fbx_import_settings':{'convert_scene':True,'force_front_x_axis':True,'convert_scene_unit':True},'driver_camera':{'location_cm':[736,-78,298],'rotation_deg':[0,0,0],'fov':75},'parts':parts,'materials':{m.name:{'rgba':list(m.diffuse_color),'base_color':list(m.diffuse_color),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value,'blend_mode':'Translucent' if m==glass else 'Opaque','opacity':.14 if m==glass else 1} for m in bpy.data.materials if m.use_nodes and m.node_tree.nodes.get('Principled BSDF')},'limitations':['Cab layout visually representative and provisional; exact depot variant requires reference validation.','Pantograph planar linkage uses exported pivots; runtime binding requires verification.','Independent brake is visual only until a simulation command is available.','No Android device cost measured.']}
json.dump(manifest,open(os.path.join(OUT,'assembly.json'),'w'),indent=2)
# Source .blend retains metre modeling units, exporter only transforms evaluated copies.
print('ASSEMBLY',len(parts),'parts',sum(p['triangles'] for p in parts),'triangles')
