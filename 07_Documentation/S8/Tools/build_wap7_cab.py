"""Author a reference-led traditional WAP7 cab; no source photograph pixels shipped.
Blender coordinates: +X forward, +Y driver's left, +Z up. FBX converts handedness.
"""
import bpy, math, json, random, sys
from pathlib import Path
from mathutils import Vector, Matrix
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'04_Assets/S8/Cabs/WAP7'
(OUT/'Parts').mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
random.seed(7)
groups={}; mats={}; definitions=[]; moving={}
def material(name,color,rough=.6,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
 definition=dict(name=name,base_color=[*color,1],roughness=rough,metallic=metal)
 if name in ['CabIvory','DeskEnamel','PanelCharcoal','SeatBlue','Floor','Rubber']:
  texname='Fabric' if name=='SeatBlue' else 'Rubber' if name in ['Floor','Rubber'] else 'Enamel';definition['detail_texture']='Textures/'+texname+'.png'
  image=bpy.data.images.load(str(OUT/definition['detail_texture']),check_existing=True);image.colorspace_settings.name='Non-Color'
  sample=m.node_tree.nodes.new('ShaderNodeTexImage');sample.image=image
  multiply=m.node_tree.nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1;multiply.inputs[2].default_value=(*color,1);m.node_tree.links.new(sample.outputs['Color'],multiply.inputs[1]);m.node_tree.links.new(multiply.outputs[0],p.inputs['Base Color'])
  roughnode=m.node_tree.nodes.new('ShaderNodeMath');roughnode.operation='MULTIPLY';roughnode.inputs[1].default_value=rough;m.node_tree.links.new(sample.outputs['Color'],roughnode.inputs[0]);m.node_tree.links.new(roughnode.outputs[0],p.inputs['Roughness'])
 mats[name]=m;definitions.append(definition);return name
material('CabIvory',(.54,.53,.43));material('DeskEnamel',(.52,.51,.43),.67)
material('PanelCharcoal',(.013,.017,.015),.76);material('Rubber',(.012,.014,.014),.85)
material('Steel',(.16,.17,.16),.42,.75);material('Lettering',(.72,.76,.69),.7)
material('DialIvory',(.51,.53,.46),.72);material('SeatBlue',(.07,.17,.23),.91)
material('Red',(.45,.015,.009),.4);material('Amber',(.7,.22,.012),.4)
material('Green',(.035,.28,.085),.4);material('Brass',(.40,.27,.07),.35,.65)
material('DisplayGlass',(.009,.027,.025),.24);material('Floor',(.07,.08,.075),.95)
material('WindowGlass',(.18,.25,.22),.12);definitions[-1]['base_color'][3]=.045
mats['WindowGlass'].node_tree.nodes.get('Principled BSDF').inputs['Alpha'].default_value=.045
def add(o,g,m):
 o.data.materials.append(mats[m]);groups.setdefault(g,[]).append(o);return o
def box(name,loc,size,m='DeskEnamel',g='Desk',bevel=.003):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Manufactured edge','BEVEL');mod.width=bevel;mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
  mod=o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');bpy.ops.object.modifier_apply(modifier=mod.name)
 return add(o,g,m)
def cylinder(name,loc,radius,depth,m='Steel',g='Equipment',axis=(0,0,1),vertices=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc);o=bpy.context.object;o.name=name
 o.rotation_euler=Vector(axis).to_track_quat('Z','Y').to_euler()
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return add(o,g,m)
def rod(name,a,b,r,m='Steel',g='Equipment',vertices=12):
 a,b=Vector(a),Vector(b);return cylinder(name,(a+b)/2,r,(b-a).length,m,g,b-a,vertices)
def text(body,loc,size=.012,m='Lettering',g='Labels',rot=(math.pi/2,0,-math.pi/2)):
 bpy.ops.object.text_add(location=loc,rotation=rot);o=bpy.context.object;o.data.body=body;o.data.size=size;o.data.align_x='CENTER';o.data.align_y='CENTER';o.data.extrude=0;o.data.resolution_u=3
 bpy.ops.object.convert(target='MESH');return add(bpy.context.object,g,m)
def front_text(body,x,y,z,size=.012,m='Lettering'):return text(body,(x,y,z),size,m)
def sphere(name,loc,r,m='Rubber',g='Equipment'):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=r,location=loc);o=bpy.context.object;o.name=name
 for p in o.data.polygons:p.use_smooth=True
 return add(o,g,m)
def face_disc(name,x,y,z,r,mat='DialIvory',group='Instruments'):
 return cylinder(name,(x,y,z),r,.006,mat,group,(-1,0,0),48)
def gauge(y,z,r,label,maxvalue=10,role=None):
 x=9.105
 dark=label in ['MR','BP','ER'];ink='Lettering' if dark else 'PanelCharcoal'
 box('Square gauge mounting plate',(x+.004,y,z),(.007,r*2.50,r*2.50),'PanelCharcoal','Instruments',.002);face_disc('Gauge body',x-.003,y,z,r*1.07,'Rubber');face_disc('Machined bezel',x-.008,y,z,r*1.035,'Steel');face_disc('Dial',x-.014,y,z,r,'PanelCharcoal' if dark else 'DialIvory')
 for i in range(41):
  a=math.radians(-130+i*6.5);p=Vector((x-.021,y-math.sin(a)*r*.86,z+math.cos(a)*r*.86));q=Vector((x-.021,y-math.sin(a)*r*(.70 if i%5==0 else .79),z+math.cos(a)*r*(.70 if i%5==0 else .79)))
  rod('Dial graduation',p,q,.0008,ink,'Instruments',6)
  if i%10==0:front_text(str(round(i/40*maxvalue)),x-.024,y-math.sin(a)*r*.55,z+math.cos(a)*r*.55,.009 if r<.075 else .013,ink)
 front_text(label,x-.025,y,z-r*.34,.008 if r<.075 else .012,ink)
 if role:
  group=role;rod('Moving pointer',(x-.029,y,z),(x-.029,y,z+r*.65),.0018,'Red',group,8);face_disc('Needle spindle',x-.032,y,z,.005,'Steel',group)
  moving[group]=dict(role=role,pivot=(x-.029,y,z),rotation_axis=[-1,0,0],rotation_range_deg=[-130,130])
 else:
  # A pressure system is absent from S3/S7: parked dial, never synthesize pressure.
  a=math.radians(-130);rod('Unpowered pointer',(x-.03,y,z),(x-.03,y-math.sin(a)*r*.66,z+math.cos(a)*r*.66),.0015,ink,'Instruments',8)
 for dy,dz in [(-1,-1),(-1,1),(1,-1),(1,1)]:cylinder('Gauge fastener',(x-.007,y+dy*r*1.04,z+dz*r*1.04),.0025,.003,'Steel','Instruments',(-1,0,0),8)
# Enclosure. Windows are genuine openings, with seals and restrained glazing edges.
box('Cab floor',(8.53,0,1.79),(2.56,2.78,.075),'Floor','Shell')
box('Roof lining',(8.48,0,4.07),(2.54,2.79,.08),'CabIvory','Shell',.04)
box('Rear bulkhead',(7.22,0,2.94),(.08,2.79,2.28),'CabIvory','Shell',.025)
box('Rear access door',(7.28,-.22,2.88),(.03,.72,1.99),'DeskEnamel','Shell')
for z in [2.05,3.65]:box('Door hinge',(7.305,.13,z),(.027,.018,.09),'Steel','Equipment',.003)
rod('Door handle',(7.33,-.49,2.70),(7.33,-.49,2.88),.012)
for side in [-1,1]:
 y=side*1.405
 box('Lower side lining',(8.48,y,2.36),(2.46,.06,1.05),'CabIvory','Shell',.025)
 box('Window upper rail',(8.47,y,3.95),(2.47,.075,.18),'CabIvory','Shell')
 for x in [7.31,8.00,9.65]:box('Window jamb',(x,y,3.40),(.075,.075,1.0),'CabIvory','Shell')
 for z in [2.9,3.86]:rod('Side window seal',(8.03,y-side*.035,z),(9.61,y-side*.035,z),.014,'Rubber','Shell')
 rod('Sliding window divider',(8.73,y-side*.035,2.90),(8.73,y-side*.035,3.86),.013,'Steel','Shell')
 rod('Window handle',(8.77,y-side*.055,3.09),(8.77,y-side*.055,3.24),.007,'Rubber')
# Front sill, twin windscreens, centre pillar and sloped outer posts.
box('Front lower panel',(9.68,0,2.46),(.075,2.78,1.19),'CabIvory','Shell')
box('Front header',(9.52,0,4.00),(.17,2.76,.14),'CabIvory','Shell',.03)
rod('Centre windscreen pillar',(9.70,0,3.02),(9.52,0,3.99),.047,'CabIvory','Shell')
for sign in [-1,1]:
 y=sign*1.34
 rod('Outer screen pillar',(9.7,y,3.02),(9.52,y,3.98),.065,'CabIvory','Shell')
 for yy in [sign*.09,sign*1.25]:rod('Windscreen rubber',(9.68,yy,3.09),(9.53,yy,3.90),.014,'Rubber','Shell')
 for z,x in [(3.09,9.68),(3.90,9.53)]:rod('Windscreen rubber',(x,sign*.09,z),(x,sign*1.25,z),.014,'Rubber','Shell')
 box('Sun visor',(9.39,sign*.66,3.86),(.025,1.01,.20),'PanelCharcoal','Equipment')
# Single-surface clear panes: faint tint, no opaque substitute for glazing.
def pane(name,vertices):
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],[(0,1,2,3)]);mesh.update()
 ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob);add(ob,'Glazing','WindowGlass')
for sign in [-1,1]:
 pane('Front clear pane',[(9.695,sign*.10,3.1),(9.695,sign*1.24,3.1),(9.55,sign*1.24,3.89),(9.55,sign*.10,3.89)])
 pane('Side clear pane',[(8.03,sign*1.40,2.92),(9.60,sign*1.40,2.92),(9.60,sign*1.40,3.85),(8.03,sign*1.40,3.85)])
# Desk and stepped brake pedestal matching the photograph's silhouette.
box('Desk fascia',(8.98,0,2.64),(.08,2.66,.15),'DeskEnamel','Desk',.005)
box('Desktop',(9.05,0,2.73),(.87,2.67,.045),'DeskEnamel','Desk',.006)
for y in [-1.17,1.17]:box('Desk side cabinet',(8.99,y,2.18),(.79,.24,.84),'DeskEnamel','Desk',.005)
box('Lower left brake pedestal',(8.33,1.15,2.15),(.49,.44,.72),'DeskEnamel','Desk',.025)
box('Upper brake pedestal',(8.70,1.15,2.54),(.37,.45,.38),'DeskEnamel','Desk',.025)
for y in [1.12,.48,-.43]:
 width={1.12:.43,.48:.79,-.43:1.00}[y]
 box('Removable instrument panel',(9.145,y,2.99),(.025,width,.40),'PanelCharcoal','Desk',.002)
 for yy in [y-width/2+.02,y+width/2-.02]:
  for z in [2.805,3.175]:cylinder('Panel captive screw',(9.103,yy,z),.003,.003,'Steel','Instruments',(-1,0,0),8)
# Right switch bank shares a thin metal continuation of the diagnostic fascia.
box('Right switch panel',(9.145,-1.115,2.99),(.025,.36,.40),'PanelCharcoal','Desk',.002)
# Left four pressure instruments and outboard auxiliary dial.
for y,z,lab in [(1.23,3.09,'MR'),(1.035,3.09,'BP'),(1.23,2.90,'BC'),(1.035,2.90,'ER')]:gauge(y,z,.064,lab)
gauge(.12,3.58,.077,'km/h',180,'gauge_speed')
box('Speed recorder housing',(9.22,.12,3.51),(.12,.215,.36),'DeskEnamel','Equipment',.005)
# Re-add speed face in front of housing; keypad under dial.
for row in range(3):
 for col in range(4):box('Recorder key',(9.07,.20-col*.045,3.40-row*.025),(.008,.032,.016),'Rubber','Instruments',.002)
# Electrical meter stack, indicators and labelled panel-A controls.
for i,label in enumerate(['kV','A','V']):
 y=.81-i*.105;box('Electrical meter bezel',(9.092,y,3.105),(.022,.071,.132),'Steel','Instruments',.004);box('Electrical meter face',(9.070,y,3.105),(.006,.058,.111),'DialIvory','Instruments',.002);front_text(label,9.063,y,3.064,.014,'PanelCharcoal')
 for tick in range(6):rod('Meter marks',(9.061,y-.028+tick*.011,3.13),(9.061,y-.028+tick*.011,3.14),.0007,'PanelCharcoal','Instruments',6)
 rod('Meter parked needle',(9.059,y,3.10),(9.059,y-.025,3.13),.001,'PanelCharcoal','Instruments',6)
for i,label in enumerate(['LSDJ','LSHO','LSP','LSAF','LSVW']):
 y=.40-i*.075;face_disc('Annunciator rim',9.09,y,3.13,.014,'PanelCharcoal');face_disc('Annunciator lens',9.081,y,3.13,.011,'Red' if i==0 else 'Amber');front_text(label,9.074,y,3.087,.010)
for i,label in enumerate(['BLCP','BLDJ','ZPT','ZTEL','ZBAN']):
 y=.70-i*.117;cylinder('Selector bezel',(9.09,y,2.88),.012,.005,'Steel','Instruments',(-1,0,0),24);box('Selector toggle',(9.062,y,2.90),(.018,.009,.029),'Rubber','Instruments',.003);front_text(label,9.065,y,2.827,.010)
for i,(label,col) in enumerate([('BPVR','Amber'),('BPCS','Green'),('BPPB','Red')]):
 y=.40-i*.12;face_disc('Pushbutton rim',9.085,y,2.98,.016,'PanelCharcoal');face_disc('Pushbutton',9.072,y,2.98,.012,col);front_text(label,9.059,y,2.946,.009)
face_disc('Emergency surround',9.09,-.02,2.90,.030,'Brass');face_disc('Emergency mushroom',9.055,-.02,2.90,.024,'Red','switch_emergency');moving['switch_emergency']=dict(role='switch_emergency',pivot=(9.055,-.02,2.90),rotation_axis=[1,0,0],rotation_range_deg=[0,0])
front_text('EMERGENCY',9.035,-.02,2.84,.010)
# Panel-A key and small labelled selector bank: density follows the reference.
face_disc('Cab key switch',9.078,.85,2.934,.017,'Steel')
rod('Inserted cab key',(9.065,.85,2.934),(9.03,.85,2.934),.003,'Steel','Instruments',8)
for i,label in enumerate(['ZPT','BLDJ','BLCP','BLHO','ZTEL','ZBAN','BPVR']):
 yy=.76-i*.100;zz=2.945
 face_disc('Small selector collar',9.078,yy,zz,.009,'Steel')
 box('Small black selector',(9.055,yy,zz+.008),(.018,.010,.027),'Rubber','Instruments',.002)
 front_text(label,9.038,yy,zz-.027,.007)
for i,label in enumerate(['OFF','ON','I','II','0','1','2']):front_text(label,9.044,.76-i*.100,2.998,.006)
# Small white equipment inscriptions and screw heads break up the broad black panels.
for yy in [.91,.06,-.06,-.92]:
 for zz in [2.805,3.17]:cylinder('Panel bolt',(9.092,yy,zz),.0027,.004,'Steel','Instruments',(-1,0,0),8)
front_text('PANEL A',9.072,.47,3.176,.008)
# Diagnostic terminal, recessed glass and membrane keypad.
box('Diagnostic outer bezel',(9.095,-.50,3.0),(.012,.54,.32),'PanelCharcoal','Instruments',.001)
box('Diagnostic screen',(9.080,-.43,3.07),(.003,.36,.075),'DisplayGlass','Instruments',.001)
front_text('DIAGNOSTIC DISPLAY',9.052,-.50,3.165,.013)
for i in range(6):box('Terminal key',(9.055,-.285-i*.055,2.936),(.008,.020,.014),'DeskEnamel','Instruments',.003)
# Driver switches to the right, mapped only where existing functions exist.
for i,(label,role,col) in enumerate([('HEADLIGHT','switch_lamps','Green'),('WIPER','switch_wipers','Rubber'),('CAB LIGHT','switch_cablight','Amber')]):
 y=-.91-i*.105;face_disc('Switch mounting',9.09,y,3.08,.017,'PanelCharcoal');box('Cab selector',(9.060,y,3.08),(.018,.010,.031),col,role,.004);front_text(label,9.045,y,3.025,.009)
 moving[role]=dict(role=role,pivot=(9.06,y,3.08),rotation_axis=[1,0,0],rotation_range_deg=[-25,25])
for row in range(2):
 for col in range(3):
  y=-.77-col*.08;z=2.91-row*.08;face_disc('Spare switch bezel',9.086,y,z,.018,'Steel');box('Toggle',(9.06,y,z),(.025,.010,.033),'Rubber','Instruments',.003)
# Reference controller silhouettes: rectangular master gate, rotary brake, small bellows control.
x,y,z=8.87,-.22,2.767
box('Master controller gate',(x,y,z),(.29,.175,.008),'PanelCharcoal','Equipment',.002)
box('Controller slot',(x,y,z+.006),(.21,.020,.004),'Rubber','Equipment',.001)
for side in [-1,1]:
 for end in [-1,1]:cylinder('Gate captive screw',(x+end*.125,y+side*.07,z+.006),.003,.004,'Steel','Equipment',vertices=8)
for i in range(7):
 text(str(i),(x-.10+i*.033,y+.045,z+.009),.010,'Lettering',rot=(0,0,math.pi/2))
rod('Master controller shaft',(x,y,z+.006),(x+.025,y,z+.17),.006,'Steel','throttle')
sphere('Master controller knob',(x+.025,y,z+.17),.024,'Rubber','throttle')
moving['throttle']=dict(role='throttle',pivot=(x,y,z+.006),rotation_axis=[0,1,0],rotation_range_deg=[-24,24])
# Driver train brake on the left raised pedestal.
x,y,z=8.57,1.15,2.777
box('Brake engraved plate',(x,y,z),(.29,.28,.008),'PanelCharcoal','Equipment',.018)
cylinder('Brake valve flange',(x,y,z+.016),.065,.021,'Steel','Equipment',vertices=48)
cylinder('Brake valve collar',(x,y,z+.043),.041,.042,'Steel','Equipment',vertices=40)
rod('Brake polished lever',(x,y,z+.061),(x+.09,y-.055,z+.145),.008,'Steel','train_brake')
sphere('Brake ball grip',(x+.09,y-.055,z+.145),.023,'Rubber','train_brake')
moving['train_brake']=dict(role='train_brake',pivot=(x,y,z+.061),rotation_axis=[0,0,1],rotation_range_deg=[-30,40])
for i,caption in enumerate(['REL','RUN','LAP','APP','EMG']):
 a=math.radians(-65+i*30);text(caption,(x+math.cos(a)*.115,y+math.sin(a)*.115,z+.007),.010,'Lettering',rot=(0,0,math.pi/2))
# Compact bellows mounted direction control, position inferred from the reference silhouette.
x,y,z=8.94,.81,2.76
cylinder('Small control brass flange',(x,y,z),.042,.009,'Brass','Equipment')
for i in range(5):cylinder('Small control bellows',(x,y,z+.012+i*.007),.033-i*.004,.008,'Rubber','Equipment',vertices=24)
rod('Direction spindle',(x,y,z+.04),(x+.025,y,z+.115),.005,'Steel','reverser')
sphere('Direction ball grip',(x+.025,y,z+.115),.015,'Rubber','reverser')
moving['reverser']=dict(role='reverser',pivot=(x,y,z+.04),rotation_axis=[0,1,0],rotation_range_deg=[-18,18])
# Lower independent brake is reference geometry; no new independent-brake physics.
cylinder('Independent brake pedestal',(8.30,1.15,2.54),.075,.035,'Brass')
rod('Independent brake handle',(8.30,1.15,2.57),(8.20,1.15,2.65),.018,'Rubber')
# Driver and assistant seats: cushions, piping, patterned stitching, arms and sprung bases.
for y in [.70,-.76]:
 cylinder('Seat pedestal',(7.91,y,2.07),.075,.49,'Steel');cylinder('Seat floor foot',(7.91,y,1.84),.22,.045,'PanelCharcoal')
 box('Seat cushion',(7.93,y,2.34),(.49,.48,.115),'SeatBlue','Equipment',.055)
 back=box('Seat back',(7.66,y,2.72),(.115,.47,.66),'SeatBlue','Equipment',.06);back.rotation_euler.y=-.10
 for side in [-1,1]:
  rod('Seat arm support',(7.83,y+side*.265,2.37),(7.83,y+side*.265,2.62),.014)
  box('Armrest',(7.92,y+side*.265,2.63),(.33,.065,.046),'Rubber','Equipment',.02)
 
 
# Footrest, pedals, conduit, desk access panels and visible ventilation slots.
for y in [.61,.86]:box('Foot pedal',(8.75,y,1.93),(.20,.14,.055),'Rubber','Equipment',.015)
rod('Footrest',(8.85,.24,1.95),(8.85,1.00,1.95),.022)
for y in [-1.12,1.12]:
 box('Cabinet access plate',(8.52,y,2.20),(.014,.24,.38),'DeskEnamel','Equipment',.007)
 for i in range(10):box('Cabinet vent',(8.509,y,2.06+i*.022),(.012,.19,.006),'PanelCharcoal','Equipment',.001)
for i in range(20):box('Floor anti-slip rib',(7.45+i*.075,0,1.835),(.009,2.3,.003),'Rubber','Equipment',.001)
# Circular cab fan with wire guard and blades, visible in reference.
fan=Vector((9.35,1.19,3.77))
for r in [.06,.105,.145]:
 bpy.ops.mesh.primitive_torus_add(major_radius=r,minor_radius=.0025,major_segments=48,minor_segments=6,location=fan,rotation=(0,math.pi/2,0));add(bpy.context.object,'Equipment','Steel')
for i in range(16):
 a=i*math.tau/16;rod('Fan guard spoke',fan,fan+Vector((0,math.sin(a)*.145,math.cos(a)*.145)),.0018,'Steel','Equipment',6)
for i in range(3):
 a=i*math.tau/3;o=box('Fan blade',fan+Vector((.015,math.sin(a)*.069,math.cos(a)*.069)),(.008,.047,.115),'PanelCharcoal','Equipment',.018);o.rotation_euler.x=-a
cylinder('Fan hub',fan,.025,.03,'Steel','Equipment',(-1,0,0))
box('Ceiling lamp',(8.33,0,4.012),(.42,.15,.027),'DialIvory','Equipment',.016)
# External protective grilles and interior-visible windscreen wiper.
for sign in [-1,1]:
 for i in range(12):
  yy=sign*(.13+i*.098);rod('Windscreen grille',(9.72,yy,3.12),(9.58,yy,3.89),.0045,'Steel','Shell',8)
 for z,xx in [(3.15,9.715),(3.86,9.587)]:rod('Grille cross rail',(xx,sign*.10,z),(xx,sign*1.27,z),.006,'Steel','Shell',8)
 pivot=Vector((9.696,sign*.45,3.13));end=Vector((9.61,sign*.78,3.65));g='wiper_cab_'+str(sign)
 rod('Wiper arm',pivot,end,.006,'PanelCharcoal',g,10);rod('Wiper rubber blade',end+Vector((0,0,-.17)),end+Vector((-.055,0,.16)),.008,'Rubber',g,8)
 moving[g]=dict(role='wiper',pivot=tuple(pivot),rotation_axis=[1,0,0],rotation_range_deg=[-30,30])
# Continuous folded console rim and rounded windscreen gasket contours.
box('Console top folded rim',(9.245,.02,3.202),(.038,2.66,.012),'CabIvory','Desk',.002)
box('Console bottom folded rim',(9.12,.02,2.785),(.025,2.66,.009),'CabIvory','Desk',.001)
for sign in [-1,1]:
 points=[]
 for cy,cz,angle in [(.23,3.22,180),(1.15,3.22,270),(1.15,3.79,0),(.23,3.79,90)]:
  for j in range(9):
   a=math.radians(angle+j*90/8);yy=cy+.12*math.cos(a);zz=cz+.12*math.sin(a);points.append((9.70-(zz-3.02)*.18,sign*yy,zz))
 for i in range(len(points)):rod('Rounded windscreen gasket',points[i],points[(i+1)%len(points)],.010,'Rubber','Shell',8)
# Tilt the instrument bank back and give the flanking panels a subtle wrap.
for group,objects in groups.items():
 for ob in objects:
  inv=ob.matrix_world.inverted()
  for v in ob.data.vertices:
   w=ob.matrix_world@v.co
   if 9.015<w.x<9.20 and 2.77<w.z<3.235:
    w.x+=(w.z-2.78)*.24
    if w.y>.91:w.x-=.10*(w.y-.91)
    elif w.y<-.04:w.x+=.10*(w.y+.04)
    v.co=inv@w
# Small paint chips concentrate at the working edge; sparse grime at panel feet.
for i in range(55):
 y=random.uniform(-.72,1.29);x=random.choice([8.625,9.18])+random.uniform(-.008,.008)
 box('Sparse enamel chip',(x,y,2.754),(random.uniform(.002,.006),random.uniform(.004,.015),.0005),'Steel','Wear',0)
for i in range(35):
 x=random.uniform(8.68,9.2);y=random.uniform(-.8,1.30)
 box('Fine surface scratch',(x,y,2.754),(random.uniform(.006,.025),.00045,.0002),'CabIvory','Wear',0)
parts=[]
for group,objects in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_WAP7_Cab_'+group
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 meta=moving.get(group,{});pivot=Vector(meta.get('pivot',(0,0,0)))
 bpy.context.scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');o.location=(0,0,0)
 filename=o.name+'.fbx'
 bpy.ops.export_scene.fbx(filepath=str(OUT/'Parts'/filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,mesh_smooth_type='FACE',path_mode='STRIP')
 o.location=pivot;o.data.calc_loop_triangles()
 parts.append(dict(name=group,role=meta.get('role','cab_static'),fbx='Parts/'+filename,location_cm=[pivot.x*100,-pivot.y*100,pivot.z*100],rotation_deg=[0,0,0],scale=[1,1,1],cab_only=True,rotation_axis=meta.get('rotation_axis',[1,0,0]),rotation_range_deg=meta.get('rotation_range_deg',[0,0]),triangles=len(o.data.loop_triangles)))
manifest=dict(schema_version=1,family='WAP7',materials=definitions,parts=parts,cab_available=True,driver_camera=dict(location_cm=[780,-65,335],rotation_deg=[-12,0,0],fov=98),instrument_display=dict(location_cm=[907,43,307],rotation_deg=[0,180,0],size_cm=2.4),cab_light_location_cm=[835,0,390],triangles=sum(x['triangles'] for x in parts),references=['https://irfca.org/gallery/Locos/Electric/wap5_7/insidewap7/IMG_0092.JPG.html','https://irfca.org/gallery/Locos/Electric/wap5_7/insidewap7/Detailed%2Bview-Panel-A-WAP7%2Bloco.JPG.html'],limitations=['Traditional desk reference, not verified specific to exterior fleet number.','Cab dimensions inferred to fit source exterior; no measured engineering drawing.','Pressure/electrical dial channels are parked; S3/S7 does not simulate these quantities. Small bellows control identity is not legible; direction mapping is a gameplay adaptation.','Rear bulkhead, seat mechanism, small labels and assistant side partly inferred.'])
(OUT/'assembly.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_Cab.blend'))
if '--no-render' in sys.argv:
 print('CAB_BUILT',manifest['triangles'],len(parts));raise SystemExit(0)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Daylight');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.55,.65,.8,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.5
for loc,energy,size in [((9.9,0,3.6),180,2.4),((8.3,0,3.92),45,1.4),((8.8,1.9,3.6),100,1.2)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=energy;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((8.7,0,2.7))-l.location).to_track_quat('-Z','Y').to_euler()
for name,loc,target in [('driver',(7.8,.65,3.35),(9.2,.65,3.052)),('overview',(7.35,-1.12,3.65),(9.10,.30,2.85))]:
 bpy.ops.object.camera_add(location=loc);c=bpy.context.object;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();c.data.lens=21;c.data.clip_start=.02;scene.camera=c;scene.render.filepath=str(OUT/('WAP7_Cab_'+name+'.png'));bpy.ops.render.render(write_still=True)
print('CAB_BUILT',manifest['triangles'],len(parts))

