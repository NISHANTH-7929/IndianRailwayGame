"""Reference-informed original FIAT-style bogie and underfloor equipment.
User photo is visual reference only: no pixels are copied into textures.
Retained licensed coach shell remains a restricted incorporated derivative.
"""
import bpy,bmesh,math,json,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'04_Assets/S8/LHBUnderframeV2';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/Prepared/LHB3A_Candidate.blend'),load_ui=False,use_scripts=False)
body=next(o for o in bpy.data.objects if o.type=='MESH' and o.name=='SM_LHB3A_Body_LOD0');body.data=body.data.copy()
for o in list(bpy.data.objects):
 if o!=body:bpy.data.objects.remove(o,do_unlink=True)
# Remove crude underfloor centre/old bogie-area boxes only; keep end steps,
# gangway/CBC fittings and all cabin/body/window geometry above the solebar.
bm=bmesh.new();bm.from_mesh(body.data)
remove=[v for v in bm.verts if v.co.z<1.22 and abs(v.co.x)<10.7]
bmesh.ops.delete(bm,geom=remove,context='VERTS');bm.to_mesh(body.data);bm.free()
mats={}
definitions=[('FrameSteel',(.19,.20,.19,1),.48,.64),('DarkSteel',(.055,.060,.055,1),.55,.60),('DustSteel',(.23,.21,.17,1),.25,.78),('TankIvory',(.46,.44,.37,1),.10,.74),('Rubber',(.023,.025,.022,1),.0,.83),('PistonSteel',(.43,.45,.46,1),.9,.23),('WeldSteel',(.16,.17,.16,1),.6,.55),('LHB_GlassV2',(.045,.13,.15,1),.0,.19)]
for n,c,metal,rough in definitions:
 m=bpy.data.materials.new(n);m.diffuse_color=c;m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=c;bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough;mats[n]=m
# Broad, outward-facing black panes are isolated from the thin rubber frames.
# Opaque coated blue glass avoids sorting/interior leaks in the source shell.
glass_slot=len(body.data.materials);body.data.materials.append(mats['LHB_GlassV2']);glass_faces=0
visible_glass_faces=0
for p in body.data.polygons:
 # Ray casts showed the old two-sided Material.012 surface hides the inner
 # Material.019 pane. Refinish the visible outer pane too, keeping door panels.
 outer=p.material_index==2 and p.area>.09 and 2.45<p.center.z<3.1
 inner=p.material_index==6 and p.area>.09 and 2.1<p.center.z<3.45
 if (outer or inner) and abs(p.center.y)>1.4 and abs(p.normal.y)>.9:
  p.material_index=glass_slot;glass_faces+=1;visible_glass_faces+=int(outer)
  if p.normal.y*p.center.y<0:p.flip()
body.data.update()
assert visible_glass_faces>=16, 'Visible pane isolation failed'
groups={'Body':[body],'Bogie':[]};group='Bogie'
def finish(o,material):
 o.data.materials.append(mats[material]);groups[group].append(o);return o
def box(p,size,material='FrameSteel',bevel=.025):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Cast edge bevel','BEVEL');mod.width=bevel;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
 return finish(o,material)
def rod(a,b,r,material='DarkSteel',vertices=20):
 a,b=Vector(a),Vector(b);v=b-a;bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=v.length,location=(a+b)*.5);o=bpy.context.object;o.rotation_euler=v.to_track_quat('Z','Y').to_euler()
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return finish(o,material)
def beam(a,b,width,depth,material='FrameSteel'):
 a,b=Vector(a),Vector(b);v=b-a;o=box((a+b)*.5,(width,depth,v.length),material,.02);o.rotation_euler=v.to_track_quat('Z','Y').to_euler();return o

def bolt_y(x,y,z,side,r=.018):
 # Washer and hex head remain subordinate to structural shapes at close range.
 rod((x,y,z),(x,y+side*.007,z),r*1.45,'DarkSteel',12)
 rod((x,y+side*.007,z),(x,y+side*.021,z),r,'FrameSteel',6)

def tank_head(x,y,z):
 # Dished pressure-vessel end, not the old visibly flat cylinder cap.
 bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=1,location=(x,y,z))
 o=bpy.context.object;o.scale=(.285,.085,.285);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 for p in o.data.polygons:p.use_smooth=True
 return finish(o,'TankIvory')
def hose(points,r=.018,material='Rubber',segments=8):
 # A single smooth swept mesh, not a cylinder object per segment.
 vertices=[];faces=[];ps=[Vector(p) for p in points]
 for i,p in enumerate(ps):
  t=(ps[min(i+1,len(ps)-1)]-ps[max(i-1,0)]).normalized();u=t.cross(Vector((0,0,1)))
  if u.length<.01:u=t.cross(Vector((0,1,0)))
  u.normalize();v=t.cross(u).normalized()
  for j in range(segments):vertices.append(p+r*(u*math.cos(j*math.tau/segments)+v*math.sin(j*math.tau/segments)))
 for i in range(len(ps)-1):
  for j in range(segments):faces.append((i*segments+j,i*segments+(j+1)%segments,(i+1)*segments+(j+1)%segments,(i+1)*segments+j))
 mesh=bpy.data.meshes.new('Swept hose');mesh.from_pydata(vertices,[],faces);mesh.update();o=bpy.data.objects.new('Swept hose',mesh);bpy.context.collection.objects.link(o)
 for p in mesh.polygons:p.use_smooth=True
 return finish(o,material)
def weld(a,b):
 a,b=Vector(a),Vector(b);count=max(2,int((b-a).length/.025))
 # Tiny shallow scallops along the joining edge, bounded six-sided geometry.
 ps=[a+(b-a)*i/count+Vector((0,.002*math.sin(i*1.7),.0015*math.cos(i*1.3))) for i in range(count+1)]
 hose(ps,.0045,'WeldSteel',6)
def spring(x,y,z,r,h,turns=6,wire=.027):
 points=turns*20;verts=[];faces=[]
 for i in range(points+1):
  a=i/points*turns*math.tau;c=Vector((x+r*math.cos(a),y+r*math.sin(a),z+h*i/points));radial=Vector((math.cos(a),math.sin(a),0))
  for j in range(8):
   b=j/8*math.tau;verts.append(c+radial*(wire*math.cos(b))+Vector((0,0,wire*math.sin(b))))
 for i in range(points):
  for j in range(8):faces.append((i*8+j,i*8+(j+1)%8,(i+1)*8+(j+1)%8,(i+1)*8+j))
 mesh=bpy.data.meshes.new('Coil');mesh.from_pydata(verts,[],faces);mesh.update();o=bpy.data.objects.new('Coil',mesh);bpy.context.collection.objects.link(o)
 for p in mesh.polygons:p.use_smooth=True
 return finish(o,'DustSteel')
for side in (-1,1):
 y=side*1.09
 # Welded FIAT-style Y box-section longitudinal members.
 box((0,y,.90),(1.72,.18,.22));box((0,y,1.025),(1.60,.23,.065),'DustSteel',.018)
 for end in (-1,1):
  beam((end*.68,y,.90),(end*1.32,y,1.10),.20,.22)
  box((end*1.34,y,1.09),(.85,.23,.13));box((end*1.7,y,.94),(.15,.21,.3))
  weld((end*.72,y+side*.116,.99),(end*1.3,y+side*.116,1.17))
  weld((end*1.0,y+side*.119,1.09),(end*1.68,y+side*.119,1.09))
  rod((end*1.28,side*.97,.4575),(end*1.28,side*1.25,.4575),.165,'FrameSteel',32)
  rod((end*1.28,side*1.25,.4575),(end*1.28,side*1.275,.4575),.128,'DarkSteel',32)
  # Bearing cover has a shallow raised centre and a separate perimeter lip,
  # not a single featureless wheel-like cylinder. It does not rotate with axle.
  rod((end*1.28,side*1.275,.4575),(end*1.28,side*1.282,.4575),.107,'FrameSteel',24)
  rod((end*1.28,side*1.282,.4575),(end*1.28,side*1.291,.4575),.052,'DarkSteel',20)
  for k in range(6):
   a=k/6*math.tau;x=end*1.28+math.cos(a)*.095;z=.4575+math.sin(a)*.095
   rod((x,side*1.275,z),(x,side*1.295,z),.016,'PistonSteel',6)
  spring(end*1.28,y,.64,.105,.34,5,.022)
  spring(end*1.28,y,.66,.063,.30,5,.014)
  box((end*1.28,y,.63),(.3,.3,.045),'DarkSteel',.008)
  # Thin spring seat above the primary nest reads as load-bearing suspension;
  # avoid enlarging coils or occluding the separately animated wheel tread.
  rod((end*1.28,y,.975),(end*1.28,y,1.005),.135,'FrameSteel',24)
  box((end*1.49,y,.995),(.08,.15,.085),'Rubber',.008)
  rod((end*1.55,y,.56),(end*.75,y,.85),.04,'FrameSteel')
  rod((end*1.55,y,.56),(end*1.2,y,.69),.065,'DarkSteel')
  rod((end*1.55,y-side*.03,.56),(end*1.55,y+side*.08,.56),.08,'Rubber',20)
  # Primary vertical damper adjacent to each axlebox crown.
  rod((end*1.0,y,.64),(end*.92,y,1.05),.026,'PistonSteel',16)
  rod((end*1.0,y,.64),(end*.97,y,.85),.048,'DarkSteel',16)
  hose([(end*1.28,side*.89,.59),(end*1.30,side*.91,.74),(end*1.12,side*.96,.86),(end*.70,side*.96,.87)],.012)
  # Stamped gusset plates stiffen the sloped sideframe without hiding the wheel.
  beam((end*.60,y+side*.10,.88),(end*1.02,y+side*.10,1.025),.028,.16,'DustSteel')
  # Separate axlebox-guide attachment plates give the casting legible joints.
  box((end*1.69,y+side*.125,.99),(.15,.025,.18),'FrameSteel',.009)
  for z in (.945,1.04):bolt_y(end*1.69,y+side*.139,z,side,.015)
 spring(0,y,1.06,.19,.27,5,.029)
 spring(0,y,1.075,.12,.24,5,.018)
 rod((0,y,1.035),(0,y,1.065),.235,'DarkSteel',32)
 rod((0,y,1.32),(0,y,1.365),.235,'FrameSteel',32)
 # Vertical damper next to secondary suspension.
 rod((.4,y,1.00),(.4,y,1.36),.035,'PistonSteel');rod((.4,y,1.00),(.4,y,1.18),.065,'DarkSteel')
 # Yaw damper and its rubber attachment bushes are horizontal above frame.
 rod((-.66,y+side*.12,1.16),(.70,y+side*.12,1.23),.024,'PistonSteel',16)
 rod((-.66,y+side*.12,1.16),(.25,y+side*.12,1.207),.052,'DarkSteel',20)
 for x,z in [(-.66,1.16),(.70,1.23)]:rod((x,y+side*.07,z),(x,y+side*.18,z),.067,'Rubber',20)
 # Secondary damper clevis brackets, with visible fastener/washer separation.
 for z in (1.01,1.34):
  box((.4,y+side*.045,z),(.16,.05,.075),'FrameSteel',.008)
  bolt_y(.4,y+side*.077,z,side,.021)
for x in (-.58,.58):rod((x,-1.04,.92),(x,1.04,.92),.105,'FrameSteel',24)
rod((.83,-1.03,.97),(.83,1.03,.97),.037,'DarkSteel',16)
for side in (-1,1):
 rod((.83,side*1.03,.97),(.83,side*1.03,1.24),.025,'DarkSteel',16)
 for z in (.97,1.24):
  rod((.83,side*.985,z),(.83,side*1.075,z),.048,'Rubber',16)
  bolt_y(.83,side*1.077,z,side,.018)
box((0,0,1.01),(.76,.75,.19),'DarkSteel')
for axle in (-1.28,1.28):
 for y in (-.40,.40):
  box((axle+.22,y,.68),(.22,.18,.27),'DarkSteel',.035)
  rod((axle+.25,y,.73),(axle+.4,y,.90),.025,'FrameSteel')
  hose([(axle+.24,y,.73),(axle+.36,y,.81),(axle+.34,y+.18,.89),(axle+.3,y+.28,.80)],.015)
 rod((axle+.3,-.85,.76),(axle+.3,.85,.76),.034,'DarkSteel')
group='Body'
# Solebars, cross members and a shallow floor keep open daylight around bogies.
for y in (-1.43,1.43):box((0,y,1.425),(21.3,.11,.37),'FrameSteel',.018)
# Retained shell lower side edge is z=1.585m. Overlap it, rather than leaving
# daylight between the derivative shell and its structural solebars.
box((0,0,1.555),(21.2,2.90,.12),'DarkSteel',.01)
for x in (-7.45,7.45):
 box((x,0,1.435),(.62,2.55,.16),'FrameSteel',.02)
 for y in (-1.09,1.09):box((x,y,1.365),(.50,.51,.055),'DarkSteel',.012)
for x in range(-10,11,2):box((x,0,1.30),(.13,2.75,.17),'FrameSteel',.015)
# Equipment occupies the inter-bogie bay, matching the reference's varied silhouette.
for x,length in [(-3.75,2.4),(.6,2.3),(3.35,1.8)]:
 box((x,-.78,.91),(length,1.25,.78),'FrameSteel',.05)
 for dx in (-length*.25,length*.25):
  box((x+dx,-1.421,.93),(length*.45,.025,.61),'DustSteel',.012)
  box((x+dx+.15,-1.444,.95),(.08,.025,.026),'PistonSteel',.008)
  for k in range(7):box((x+dx,-1.444,.70+k*.05),(length*.32,.019,.013),'DarkSteel',.003)
  # Hinged service covers rather than uniformly blank equipment boxes.
  for z in (.73,1.12):
   rod((x+dx-length*.20,-1.447,z-.035),(x+dx-length*.20,-1.447,z+.035),.014,'FrameSteel',10)
  for sx in (-1,1):bolt_y(x+dx+sx*length*.19,-1.445,1.16,-1,.011)
 for dx in (-length*.4,length*.4):box((x+dx,-.78,1.29),(.1,1.38,.14),'DarkSteel',.008)
for x in (-1.9,-.9):
 rod((x,-1.24,.86),(x,1.24,.86),.285,'TankIvory',48)
 for y in (-1.24,1.24):tank_head(x,y,.86)
 for y in (-.85,.85):
  rod((x,y-.025,.86),(x,y+.025,.86),.30,'DarkSteel',40)
  box((x,y,1.18),(.13,.07,.23),'FrameSteel',.014)
 rod((x,-1.37,.86),(x,-1.32,.86),.035,'DarkSteel')
 # Low drain tap, pipe elbow and small transverse handle; all above rail level.
 rod((x,0,.58),(x,0,.51),.024,'FrameSteel',12)
 rod((x-.065,0,.525),(x+.065,0,.525),.011,'DarkSteel',10)
 hose([(x,-1.37,.86),(x,-1.39,1.02),(x+.16,-1.34,1.16),(x+.3,-.35,1.17)],.016,'DarkSteel',8)
for y in (-.35,.35):rod((-5,y,1.17),(5,y,1.17),.025,'DarkSteel')
for x in (-4.8,-2.6,.1,1.8,4.3):
 for y in (-1.31,1.31):
  box((x,y,1.21),(.18,.07,.23),'DustSteel',.008)
  rod((x,y-.045,1.21),(x,y+.045,1.21),.025,'DarkSteel',12)
# End brake-pipe flexible loops stay on the coach body and clear rotating bogies.
for end in (-1,1):
 for side in (-1,1):
  hose([(end*10.7,side*.42,1.23),(end*11.25,side*.43,1.10),(end*11.57,side*.38,.92),(end*11.72,side*.27,.94),(end*11.80,side*.25,1.12)],.023)
  rod((end*11.72,side*.25,1.10),(end*11.80,side*.25,1.10),.04,'FrameSteel',12)
for y in (-1.447,):
 for x in (-4.95,-2.55,-.55,1.75,2.45,4.25):weld((x,y,.59),(x,y,1.25))
report={'visual_reference':'User attached LHB coach photo, 2026-10-13; reference only, no pixel reuse','engineering_replica':False,'detail_revision':'2026-10-14 spring seats, bearing-cover relief, anti-roll joints, layered glass response; representative not engineering-certified','source_shell':'LHB_Sketchfab_c6c3d431/Prepared/LHB3A_Candidate.blend','removed_underfloor_vertices':len(remove),'glass_faces_refined':glass_faces,'visible_outer_glass_faces':visible_glass_faces,'research':['https://rdso.indianrailways.gov.in/uploads/files/Revised_LHB_Manual_Vol_II_Chapter_I_Introduction_Draft.pdf'],'parts':[]}
for name,objects in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_LHB_'+name+'V2';bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');o.data.calc_loop_triangles()
 # Object-local baked wear travels with the coach, with no world-space swimming.
 # The imported material base colour carries paint; RGB is its wear multiplier.
 color=o.data.color_attributes.get('Color') or o.data.color_attributes.new(name='Color',type='BYTE_COLOR',domain='CORNER')
 o.data.color_attributes.active_color=color
 for p in o.data.polygons:
  mat=o.data.materials[p.material_index];mn=mat.name if mat else ''
  for li in p.loop_indices:
   v=o.data.vertices[o.data.loops[li].vertex_index].co;noise=.5+.25*math.sin(v.x*17+v.y*23+v.z*9)+.25*math.sin(v.x*5-v.y*11+v.z*29)
   if 'Glass' in mn:
    # A restrained top-to-bottom tint represents coated sealed glazing depth.
    # Baked local colour remains attached to the pane, not the world/camera;
    # specular reflections still come from Unreal. No fake scenery is painted.
    height=max(0.,min(1.,(v.z-2.1)/1.35))
    shade=.68+.26*height+.035*noise;rgb=(shade*.94,shade,shade*1.035)
   elif 'PistonSteel' in mn:shade=.91+.08*noise;rgb=(shade,shade,shade)
   elif 'Rubber' in mn:shade=.72+.19*noise;rgb=(shade,shade,shade)
   elif name=='Bogie':shade=.51+.39*noise;rgb=(shade*1.08,shade,shade*.89)
   elif v.z<1.62:shade=.58+.28*noise;rgb=(shade*1.065,shade,shade*.91)
   else:
    dust=max(0.,min(1.,(2.2-v.z)/.65))*.22
    streak=max(0.,math.sin(v.x*13.7+math.sin(v.x*3.9)))**12*.085
    shade=.93+.06*noise-dust-streak
    rgb=(shade*(1+dust*.15),shade,shade*(1-dust*.18))
   color.data[li].color=(*rgb,1)
 bounds=[[min(v.co[k] for v in o.data.vertices),max(v.co[k] for v in o.data.vertices)] for k in range(3)]
 assert all(math.isfinite(v) for pair in bounds for v in pair)
 assert len(o.data.loop_triangles)<(120000 if name=='Body' else 40000)
 bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,use_triangles=True,path_mode='STRIP',prioritize_active_color=True)
 fbx=OUT/(o.name+'.fbx');report['parts'].append({'name':o.name,'triangles':len(o.data.loop_triangles),'vertices':len(o.data.vertices),'bounds_m':bounds,'sha256':hashlib.sha256(fbx.read_bytes()).hexdigest(),'materials':[m.name if m else '' for m in o.data.materials]})
(OUT/'manifest.json').write_text(json.dumps(report,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LHB_UnderframeV2_Refined.blend'))
print('LHB_UNDERFRAME_V2',json.dumps(report))
