import bpy, math, os, json
from mathutils import Vector
OUT=os.path.dirname(os.path.abspath(__file__))
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
def mat(n,c,metal=0,rough=.45,emit=0):
 m=bpy.data.materials.new(n); m.diffuse_color=(*c,1); m.use_nodes=True; p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*c,1); p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if emit: p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=emit
 return m
green=mat('WAG9 enamel green',(.025,.20,.095),.3); yellow=mat('Safety yellow',(.92,.68,.035),.15); steel=mat('Bogies graphite',(.055,.065,.07),.65); silver=mat('Machined steel',(.35,.39,.4),.8); rubber=mat('Rubber',(.015,.02,.022)); glass=mat('Blue window glass',(.045,.13,.17),.45,.16); ivory=mat('Cab grey',(.43,.49,.47)); panel=mat('Desk enamel',(.095,.17,.16)); black=mat('Instrument black',(.008,.015,.018)); white=mat('Lettering',(.85,.88,.81)); light=mat('Headlamp',(.95,.86,.63),0,.2,2); red=mat('Red lamp',(.6,.015,.008),0,.3,1); screen=mat('Display cyan',(.025,.5,.65),0,.4,1)
def cube(n,loc,scale,ma,bevel=.025):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=n;o.dimensions=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(ma)
 if bevel: mod=o.modifiers.new('Edge highlights','BEVEL');mod.width=bevel;mod.segments=2; o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return o
def cyl(n,loc,r,depth,ma,axis='Z',verts=24):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc);o=bpy.context.object;o.name=n;o.data.materials.append(ma)
 if axis=='Y':o.rotation_euler[0]=math.pi/2
 if axis=='X':o.rotation_euler[1]=math.pi/2
 return o
def rod(n,a,b,r,ma):
 d=Vector(b)-Vector(a);o=cyl(n,(Vector(a)+Vector(b))/2,r,d.length,ma,verts=12);o.rotation_euler=d.to_track_quat('Z','Y').to_euler();return o
def text(n,s,loc,size,ma,rot):
 bpy.ops.object.text_add(location=loc,rotation=rot);o=bpy.context.object;o.name=n;o.data.body=s;o.data.size=size;o.data.align_x='CENTER';o.data.extrude=.001;o.data.materials.append(ma);return o
# X longitudinal, front at +X. Rail top Z=0. Indian broad gauge 1.676 m.
cube('Underframe',(0,0,1.24),(19.1,3.05,.38),steel)
cube('Body lower',(0,0,2.05),(18.6,3.12,1.30),green,.10)
cube('Equipment room',(0,0,3.22),(13.8,3.10,1.45),green,.10)
cube('Roof',(0,0,4.02),(18.5,2.86,.20),ivory,.12)
for sign in (-1,1):
 # front base with inward sloping upper windscreen
 cube('Cab nose',(sign*8.66,0,2.58),(1.3,3.1,.75),green,.08)
 cube('Cab fascia',(sign*9.05,0,3.62),(.16,3.0,.55),green,.04)
 for y in (-1.48,0,1.48):cube('Windscreen pillar',(sign*8.98,y,3.25),(.20,.10,1.05),green,.015)
 for y in (-.76,.76):
  w=cube('Windscreen',(sign*9.0,y,3.30),(.045,1.36,.67),glass,.01)
  rod('Windshield wiper',(sign*9.055,y-.45,3.02),(sign*9.065,y+.22,3.37),.014,rubber)
 for y in (-1.54,1.54):
  cube('Cab side sill',(sign*7.8,y,2.75),(2.45,.10,.35),green)
  cube('Cab side window',(sign*8.05,y,3.3),(1.5,.035,.68),glass,.025)
  cube('Door',(sign*6.92,y,2.80),(.65,.05,1.83),green,.015)
  cube('Door window',(sign*6.92,y*1.01,3.34),(.43,.035,.50),glass)
  rod('Door handrail',(sign*6.45,y*1.03,1.65),(sign*6.45,y*1.03,2.95),.023,silver)
  for z in (1.05,1.3,1.55):cube('Access step',(sign*6.9,y*1.07,z),(.75,.22,.06),silver)
  cube('Yellow waist stripe',(0,y*1.02,2.54),(18.45,.026,.24),yellow,.002)
  text('Class lettering','WAG - 9',(sign*2.7,y*1.025,2.90),.40,white,(math.pi/2,0,0 if y<0 else math.pi))
  text('Number','31082',(sign*6.7,y*1.028,2.42),.18,black,(math.pi/2,0,0 if y<0 else math.pi))
 cube('Front yellow stripe',(sign*9.32,0,2.54),(.025,3.05,.24),yellow,.002)
 text('Front number','31082',(sign*9.34,0,2.30),.20,white,(math.pi/2,0,sign*math.pi/2))
 for y in (-.97,.97):
  cyl('Buffer stem',(sign*9.55,y,1.35),.12,.5,steel,'X');cyl('Buffer plate',(sign*9.84,y,1.35),.25,.09,steel,'X')
  for z,ma in ((2.15,light),(1.95,red)):cyl('Marker bezel',(sign*9.33,y,z),.115,.08,black,'X');cyl('Marker lens',(sign*9.38,y,z),.08,.02,ma,'X')
 for y in (-.14,.14):cyl('Twin headlight housing',(sign*9.16,y,3.77),.14,.18,steel,'X');cyl('Twin headlight glass',(sign*9.26,y,3.77),.108,.025,light,'X')
 cube('Coupler',(sign*9.9,0,1.04),(.65,.32,.27),steel)
 cube('Pilot',(sign*9.10,0,.66),(.17,2.75,.47),steel)
 for y in (-.5,.5):rod('Brake hose',(sign*9.35,y,1.18),(sign*9.5,y,.73),.04,rubber)
# Louvred sides
for y in (-1.565,1.565):
 for x in (-5,-2.6,0,2.6,5):
  cube('Vent backing',(x,y,3.27),(1.65,.025,.95),black,.02)
  for z in range(12):cube('Vent slat',(x,y*1.007,2.85+z*.075),(1.59,.03,.025),green,.002)
# Two Co-Co bogies
for bx in (-6.0,6.0):
 cube('Bogie bolster',(bx,0,.75),(4.5,2.05,.28),steel)
 for y in (-1.1,1.1):
  cube('Bogie side frame',(bx,y,.67),(4.55,.20,.28),steel,.08)
  for dx in (-1.85,0,1.85):
   cyl('Wheel',(bx+dx,y*.81,.545),.545,.16,steel,'Y',32);cyl('Wheel rim',(bx+dx,y*.89,.545),.475,.03,silver,'Y',32);cyl('Axle box',(bx+dx,y*1.10,.545),.19,.24,steel,'Y')
   for sx in (-.30,.30):
    for iz in range(5):cyl('Primary spring',(bx+dx+sx,y,.78+iz*.047),.115,.023,silver,verts=12)
  for dx in (-.9,.9):rod('Suspension damper',(bx+dx,y,.68),(bx+dx+.15,y,1.18),.055,silver)
 for dx in (-1.85,0,1.85):cyl('Axle',(bx+dx,0,.545),.1,2.12,steel,'Y')
for y in (-.83,.83):cyl('Air reservoir',(0,y,.90),.26,2.2,steel,'X')
cube('Transformer enclosure',(0,0,.9),(2.4,1.10,.55),steel)
# Roof equipment and single arm pantographs
for x in (-3,0,3):
 cube('Roof ventilation unit',(x,0,4.18),(1.8,1.6,.25),steel)
 for i in range(9):cube('Roof grille',(x-.75+i*.18,0,4.315),(.04,1.48,.02),silver,.001)
for x in (-6.0,6.0):
 for y in (-.62,.62):
  cyl('Pantograph insulator',(x,y,4.28),.13,.36,ivory)
 cube('Pantograph base',(x,0,4.50),(1.30,1.40,.10),steel)
 for y in (-.43,.43):
  rod('Panto lower arm',(x-.45,y,4.56),(x+.7,y,5.18),.045,steel)
  rod('Panto upper arm',(x+.7,y,5.18),(x-.25,y,5.83),.033,steel)
 rod('Contact bow',(x-.25,-.95,5.85),(x-.25,.95,5.85),.04,steel)
 rod('Carbon strip',(x-.25,-.70,5.90),(x-.25,.70,5.90),.025,rubber)
for x in (-4,-2,2,4):
 for iz in range(5):cyl('Roof HV insulator',(x,.85,4.2+iz*.06),.12,.025,ivory,verts=12)
rod('HV bus cable',(-4,.85,4.5),(4,.85,4.5),.025,silver)
# Interior, separate collection for visibility and export control
cab=bpy.data.collections.new('CAB_INTERIOR');bpy.context.scene.collection.children.link(cab)
before=set(bpy.data.objects)
cube('Cab floor',(7.65,0,1.68),(2.6,2.9,.10),rubber)
cube('Cab rear wall',(6.48,0,2.82),(.10,2.9,2.22),ivory)
cube('Desk pedestal',(8.48,0,2.02),(.64,2.70,.63),panel,.08)
cube('Desk worktop',(8.4,0,2.39),(.88,2.73,.12),ivory,.04)
d=cube('Instrument fascia',(8.79,0,2.62),(.12,2.65,.45),panel);d.rotation_euler[1]=-.16
for y in (-1.00,-.65,.65,1.00):
 cyl('Pressure gauge rim',(8.70,y,2.68),.14,.035,silver,'X');cyl('Pressure dial',(8.675,y,2.68),.12,.014,black,'X')
 rod('Gauge needle',(8.655,y,2.68),(8.655,y+.055,2.74),.006,white)
 for i in range(9):
  ang=i*math.pi/4;cube('Gauge tick',(8.65,y+math.sin(ang)*.10,2.68+math.cos(ang)*.10),(.008,.008,.015),white,.001)
cube('Driver display bezel',(8.69,-.12,2.66),(.05,.62,.35),black)
cube('Driver display',(8.66,-.12,2.66),(.016,.53,.27),screen,.004)
text('Display readout','000 km/h',(8.644,-.12,2.67),.07,white,(math.pi/2,0,-math.pi/2))
for y,name in ((-.9,'Throttle'),(.82,'TrainBrake'),(1.12,'IndependentBrake')):
 cyl(name+' base',(8.18,y,2.49),.095,.05,black)
 rod(name+' handle',(8.18,y,2.50),(8.12,y,2.73),.025,steel);cube(name+' grip',(8.12,y,2.73),(.12,.16,.06),rubber)
for y in [-.48,-.30,-.12,.06,.24,.42]:
 cyl('Console button',(8.16,y,2.48),.028,.025,red if y==.42 else black,verts=12)
 text('Control label','AUX',(8.30,y,2.46),.035,black,(0,0,math.pi/2))
for y in (-.84,.84):
 cyl('Seat pedestal',(7.23,y,1.95),.10,.52,steel)
 cube('Seat cushion',(7.23,y,2.17),(.55,.53,.16),rubber,.06)
 cube('Seat back',(6.98,y,2.49),(.13,.53,.56),rubber,.07)
for y in (-1.35,1.35):rod('Interior grab rail',(7.1,y,2.5),(7.1,y,3.6),.025,silver)
cube('Cab ceiling light',(7.4,0,3.91),(.45,.20,.025),light)
for o in set(bpy.data.objects)-before:
 for c in list(o.users_collection):c.objects.unlink(o)
 cab.objects.link(o)
# named camera and pivots for integration
bpy.ops.object.empty_add(location=(7.36,-.78,2.98));bpy.context.object.name='DriverViewpoint'
# export before studio environment
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT,'WAG9_S8.fbx'),use_selection=True,object_types={'MESH','EMPTY'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z')
mesh=[o for o in bpy.context.scene.objects if o.type=='MESH'];tri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in mesh)
json.dump({'mesh_objects':len(mesh),'base_triangles':tri,'materials':len(bpy.data.materials),'dimensions_m':{'body_length':18.6,'width':3.12,'wheel_diameter':1.09},'status':'provisional visual asset; game bindings and Android profiling pending'},open(os.path.join(OUT,'mesh_report.json'),'w'),indent=2)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1400;scene.render.resolution_y=840;scene.render.resolution_percentage=100
scene.world.color=(.25,.25,.25)
def area(n,loc,power,size):
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.name=n;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,2))-o.location).to_track_quat('-Z','Y').to_euler()
area('Studio key',(4,-9,14),2500,10);area('Studio fill',(-8,4,10),2200,10)
bpy.ops.object.camera_add(location=(22,-24,13));cam=bpy.context.object;cam.name='Exterior review camera';cam.rotation_euler=(Vector((0,0,2.3))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=24;scene.camera=cam
cube('Studio ground',(0,0,-.13),(200,200,.15),mat('Ground',(.15,.18,.20)),0)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'WAG9_S8.blend'))
scene.render.filepath=os.path.join(OUT,'WAG9_exterior.png');bpy.ops.render.render(write_still=True)
cam.location=(7.05,-.35,3.02);cam.data.type='PERSP';cam.data.lens=19;cam.rotation_euler=(Vector((8.6,0,2.68))-cam.location).to_track_quat('-Z','Y').to_euler()
area('Cab review light',(7.3,0,3.75),90,1.0)
scene.render.filepath=os.path.join(OUT,'WAG9_cab.png');bpy.ops.render.render(write_still=True)
