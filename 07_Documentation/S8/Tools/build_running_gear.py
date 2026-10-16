"""Original wheelsets and non-destructive derivatives of retained WAP7 bogies.
Run with Blender. No source blend or existing imported asset is overwritten.
"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'04_Assets/S8/RunningGearV2';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'04_Assets/S8/Acquired/WAP7_Selected/Prepared/WAP7_Selected.blend'),load_ui=False,use_scripts=False)
report={'version':2,'units':'metres','axis':'X longitudinal Y axle Z vertical','parts':[],'original_wheel_geometry':True}
def export(obj,label):
 bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
 saved=obj.location.copy();obj.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=str(OUT/(label+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,use_triangles=True,path_mode='STRIP')
 obj.location=saved;obj.data.calc_loop_triangles()
 report['parts'].append({'name':label,'triangles':len(obj.data.loop_triangles),'materials':[m.name if m else '' for m in obj.data.materials]})
for obj in list(bpy.data.objects):
 if obj.type!='MESH' or 'Bogie' not in obj.name:continue
 front='Front' in obj.name
 axles=[3.7941656,5.8503966,7.9066277] if front else [-7.8839245,-5.8276935,-3.7714624]
 obj.data=obj.data.copy();bm=bmesh.new();bm.from_mesh(obj.data)
 remove=[]
 for v in bm.verts:
  p=obj.matrix_world@v.co
  if abs(p.y)<.97 and any((p.x-x)**2+(p.z-.59390035)**2<.607**2 for x in axles):remove.append(v)
 bmesh.ops.delete(bm,geom=remove,context='VERTS');bm.to_mesh(obj.data);bm.free()
 export(obj,'SM_WAP7_Frame_'+('Front' if front else 'Rear'))
 report['parts'][-1].update(removed_vertices=len(remove),axles_m=axles,source='WAP7_Selected/Prepared/WAP7_Selected.blend; retained original licence applies')
for obj in list(bpy.data.objects):bpy.data.objects.remove(obj,do_unlink=True)
materials=[]
for name,color,metal,rough in [('WheelWeb',(.12,.105,.09,1),.65,.5),('WheelTread',(.46,.48,.5,1),.95,.24),('WheelHub',(.20,.21,.22,1),.8,.34),('Witness',(.65,.60,.45,1),.1,.7)]:
 m=bpy.data.materials.new(name);m.diffuse_color=color;materials.append(m)
def wheelset(label,radius,back):
 verts=[];faces=[];slots=[]
 def lathe(profile,side=1,slot=0,cx=0,cz=0):
  start=len(verts);n=96
  for y,r in profile:
   for i in range(n):
    a=2*math.pi*i/n;verts.append((cx+r*math.cos(a),side*y,cz+r*math.sin(a)))
  for j in range(len(profile)-1):
   for i in range(n):
    ids=(start+j*n+i,start+j*n+(i+1)%n,start+(j+1)*n+(i+1)%n,start+(j+1)*n+i)
    faces.append(ids if side==1 else ids[::-1]);slots.append(slot)
 b=back/2
 # Dished forged web, hub boss, chamfered outer rim, tapered polished tread,
 # rounded flange on the inner face. Dimensions are a visual approximation.
 for side in (-1,1):
  lathe([(b,.12),(b+.02,.15),(b+.04,.18),(b+.055,radius*.64),(b+.025,radius-.055),(b+.025,radius-.01),(b+.133,radius-.018),(b+.133,radius-.065),(b+.085,radius-.075),(b+.08,radius*.63),(b+.09,.18),(b+.16,.15),(b+.16,.12),(b,.12)],side,0)
  lathe([(b,radius-.03),(b,radius+.017),(b+.006,radius+.027),(b+.016,radius+.029),(b+.027,radius+.02),(b+.034,radius+.004),(b+.052,radius+.001),(b+.073,radius),(b+.12,radius-.003),(b+.133,radius-.018)],side,1)
  lathe([(b-.06,.001),(b-.06,.12),(b+.18,.12),(b+.18,.001)],side,2)
  # Small physical inspection paint stripe on outer web makes roll visible.
  for i in range(3):
   y=side*(b+.091);r=.23+i*.04;start=len(verts)
   verts.extend([(r,y,-.009),(r+.035,y,-.009),(r+.035,y,.009),(r,y,.009)])
   faces.append((start,start+1,start+2,start+3));slots.append(3)
 lathe([(-b,.001),(-b,.095),(b,.095),(b,.001)],1,2)
 if 'LHB' in label:
  for y in (-.40,.40):lathe([(y-.035,.10),(y-.035,.30),(y+.035,.30),(y+.035,.10),(y-.035,.10)],1,2)
 mesh=bpy.data.meshes.new(label);mesh.from_pydata(verts,[],faces);mesh.update()
 obj=bpy.data.objects.new(label,mesh);bpy.context.collection.objects.link(obj)
 for m in materials:mesh.materials.append(m)
 for p,slot in zip(mesh.polygons,slots):p.material_index=slot;p.use_smooth=True
 bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(mesh);bm.free()
 export(obj,label);report['parts'][-1].update(radius_m=radius,back_to_back_m=back,rail_head_centres_m=1.746)
wheelset('SM_LHB_Wheelset',.4575,1.600)
wheelset('SM_WAP7_Wheelset',.546,1.596)
(OUT/'manifest.json').write_text(json.dumps(report,indent=2))
print('RUNNING_GEAR_COMPLETE',json.dumps(report))
