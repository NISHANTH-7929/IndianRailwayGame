"""Correct and partition the acquired coach, preserving source geometry as the base."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[3]
BASE=ROOT/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431'
OUT=BASE/'Prepared';(OUT/'Parts').mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(BASE/'Review/LHB_OptimizedStudy.blend'),load_ui=False,use_scripts=False)
report={'asset_id':'Coach.LHB.Acquired01','content_id':'IR-S8-LHB-3A-1','definition_id':3001,'version':1,
 'body_length_m':23.54,'coupled_length_m':24,'coupling_gap_m':0,'bogie_offsets_m':[7.45,-7.45],
 'bogie_wheelbase_m':2.56,'wheel_diameter_m':.915,'status':'candidate; acceptance requires visual/Unreal/device checks',
 'corrections':[],'parts':[],'materials':[]}
# Source dimensions are measured in source-inspection.json. Correct body and running
# gear independently so longitudinal body correction does not turn wheels elliptical.
cx=.0122962;cy=-.003825
sx=23.54/12.0252485;sy=3.24/2.9364452;sz=4.25/3.2900028
inspection=json.loads((BASE/'source-inspection.json').read_text())
byname={o['name']:o for o in inspection['objects']}
axles=[byname[n]['location'][0] for n in ['Cylinder.029','Cylinder.008','Cylinder.012','Cylinder.011']]
bogies=[sum(axles[:2])/2,sum(axles[2:])/2]
wheel_names={'Cylinder.029':0,'Cylinder.008':1,'Cylinder.012':2,'Cylinder.011':3,
 'Cylinder.026':0,'Cylinder.030':1,'Cylinder.028':2,'Cylinder.027':3}
groups={}
for obj in list(bpy.context.scene.objects):
 if obj.type!='MESH' or obj.name.startswith('Text'):
  bpy.data.objects.remove(obj,do_unlink=True);continue
 obj.data=obj.data.copy();mesh=obj.data
 points=[obj.matrix_world@v.co for v in mesh.vertices]
 lo=Vector([min(p[i] for p in points) for i in range(3)]);hi=Vector([max(p[i] for p in points) for i in range(3)])
 center=(lo+hi)/2
 role='Body';pivot=Vector((0,0,0))
 axle=wheel_names.get(obj.name)
 if axle is not None:
  side=0 if axle<2 else 1;target=(-7.45 if side==0 else 7.45)+(-1.28 if axle%2==0 else 1.28)
  # Radial scale is based on source tread radius 0.3547978, not flange bounds.
  radial=.4575/.3547978
  for v,p in zip(mesh.vertices,points):v.co=Vector((target+(p.x-axles[axle])*radial,(p.y-cy)*1.676/1.33339,.4575+(p.z-.4226304)*radial))
  role='Wheel'+str(axle);pivot=Vector((target,0,.4575))
 elif hi.z<.85 and abs(center.x)>3.2 and (hi.x-lo.x)<1.8 and abs(center.x-bogies[0 if center.x<0 else 1])<.6:
  side=0 if center.x<0 else 1;source=bogies[side];target=-7.45 if side==0 else 7.45
  for v,p in zip(mesh.vertices,points):v.co=Vector((target+(p.x-source)*2.56/(axles[side*2+1]-axles[side*2]),(p.y-cy)*sy,p.z*1.15+.07))
  role='Bogie'+str(side);pivot=Vector((target,0,0))
 else:
  for v,p in zip(mesh.vertices,points):
   x=(p.x-cx)*sx
   if abs(x)>11.77:x=math.copysign(min(12,11.77+(abs(x)-11.77)*.23/(6.48*sx-11.77)),x)
   v.co=Vector((x,(p.y-cy)*sy,p.z*sz))
 obj.matrix_world=Matrix.Identity(4)
 groups.setdefault(role,{'objects':[],'pivot':pivot})['objects'].append(obj)
 report['corrections'].append({'source_object':obj.name,'part':role})
# Replace inconsistent source text with subtype label only; do not invent vehicle numbers.
mat=bpy.data.materials.new('LHB_Lettering');mat.diffuse_color=(.8,.8,.72,1);mat.use_nodes=True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.8,.8,.72,1)
for side in [-1,1]:
 bpy.ops.object.text_add(location=(2*side,side*1.627,3.12),rotation=(math.pi/2,0,0 if side==-1 else math.pi))
 obj=bpy.context.object;obj.data.body='AC 3 TIER';obj.data.align_x='CENTER';obj.data.size=.24;obj.data.extrude=.001
 obj.data.materials.append(mat);bpy.ops.object.convert(target='MESH');groups['Body']['objects'].append(bpy.context.object)
parts=[]
for role,group in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for obj in group['objects']:obj.select_set(True)
 bpy.context.view_layer.objects.active=group['objects'][0]
 bpy.ops.object.join();obj=bpy.context.object;obj.name='SM_LHB3A_'+role+'_LOD0'
 bpy.context.scene.cursor.location=group['pivot'];bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 # Shared source materials retained. New UVs are independently generated after geometry correction.
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
 bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015)
 bpy.ops.object.mode_set(mode='OBJECT')
 colors=obj.data.color_attributes.new(name='SurfaceVariation',type='FLOAT_COLOR',domain='CORNER')
 for index,loop in enumerate(obj.data.loops):
  point=obj.matrix_world@obj.data.vertices[loop.vertex_index].co
  variation=.975+.025*math.sin(point.x*13.7+point.y*17.3)*math.sin(point.z*11.1+point.x*3.7)
  dust=.045*max(0,1-point.z/1.2)
  colors.data[index].color=(variation-dust*.35,variation-dust*.5,variation-dust,.95+.05*math.sin(point.x*7+point.z*3))
 obj.data.calc_loop_triangles()
 filename=obj.name+'.fbx';saved=obj.location.copy();obj.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=str(OUT/'Parts'/filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,use_triangles=True,path_mode='STRIP')
 obj.location=saved;parts.append(obj)
 report['parts'].append({'name':role,'role':'body' if role=='Body' else 'wheel' if role.startswith('Wheel') else 'bogie',
 'fbx':'Parts/'+filename,'location_cm':[v*100 for v in saved],'rotation_deg':[0,0,0],'scale':[1,1,1],
 'triangles':len(obj.data.loop_triangles),'material_slots':len(obj.data.materials),
 'local_bounds_m':[[min(v.co[i] for v in obj.data.vertices) for i in range(3)],[max(v.co[i] for v in obj.data.vertices) for i in range(3)]]})
used={m for o in parts for m in o.data.materials if m}
for m in used:
 node=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if m.use_nodes else None
 report['materials'].append({'name':m.name,'base_color':list(node.inputs['Base Color'].default_value) if node else list(m.diffuse_color),
 'metallic':float(node.inputs['Metallic'].default_value) if node else 0,'roughness':max(.38,float(node.inputs['Roughness'].default_value)) if node else .6,'vertex_color':True})
report['triangles']=sum(p['triangles'] for p in report['parts'])
(OUT/'assembly.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LHB3A_Candidate.blend'))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1600;scene.render.resolution_y=650;scene.render.resolution_percentage=100
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.3,.3,.3,1)
bpy.ops.object.light_add(type='AREA',location=(0,-7,13));bpy.context.object.data.energy=2500;bpy.context.object.data.size=10
bpy.ops.object.camera_add(location=(18,-29,12));camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,2))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=29;scene.camera=camera
scene.render.filepath=str(OUT/'candidate.png');bpy.ops.render.render(write_still=True)
print('LHB_PREPARED',report['triangles'])
