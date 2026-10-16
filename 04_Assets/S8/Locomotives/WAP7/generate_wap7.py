import bpy, math, os, json, bmesh
from mathutils import noise
from mathutils import Vector
O=os.path.dirname(os.path.abspath(__file__))
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
sc=bpy.context.scene; sc.unit_settings.system='METRIC'; sc.unit_settings.scale_length=.01
parts={}; current='Exterior'; pivots={}; roles={}
def group(n,role,p=(0,0,0)):
 global current
 current=n; parts.setdefault(n,[]); pivots[n]=Vector(p)*100; roles[n]=role

def mat(n,c,metal=0,rough=.5,emit=0):
 m=bpy.data.materials.new(n); m.diffuse_color=(*c,1); m.use_nodes=True; p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*c,1); p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if emit:p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=emit
 return m
ivory=mat('M_WAP7_Ivory',(.78,.77,.69)); red=mat('M_WAP7_Vermilion',(.55,.065,.025)); dark=mat('M_Common_Charcoal',(.025,.035,.043),.35); steel=mat('M_Common_Steel',(.26,.29,.31),.8,.32); grey=mat('M_Common_Bogie',(.12,.145,.155),.4); glass=mat('M_Common_Glass',(.07,.17,.19),.3,.17); green=mat('M_Cab_Desk',(.39,.42,.41)); black=mat('M_Cab_Instrument',(.008,.014,.018)); white=mat('M_Common_Lettering',(.92,.92,.82)); amber=mat('M_Lamp_Amber',(1,.51,.12),0,.3,2); lamp=mat('M_Lamp_White',(.85,.94,1),0,.2,3); screen=mat('M_Cab_LCD',(.035,.19,.23),0,.35,.6); brown=mat('M_Common_Insulator',(.21,.09,.05),.2)
def add(o,n,m):
 o.name='SM_WAP7_01_'+n; o.data.materials.append(m); parts.setdefault(current,[]).append(o);return o
def box(n,p,d,m,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)*100);o=bpy.context.object;o.dimensions=Vector(d)*100;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);add(o,n,m)
 if bevel:
  b=o.modifiers.new('Edge highlights','BEVEL');b.width=bevel*100;b.segments=2;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=b.name)
 return o
def cyl(n,p,r,dep,m,axis='Z',v=20):
 bpy.ops.mesh.primitive_cylinder_add(vertices=v,radius=r*100,depth=dep*100,location=Vector(p)*100);o=bpy.context.object
 if axis=='Y':o.rotation_euler.x=math.pi/2
 if axis=='X':o.rotation_euler.y=math.pi/2
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);return add(o,n,m)
def rod(n,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(n,(a+b)/2,r,(b-a).length,m,v=10);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def text(n,s,p,size,m,rot=(0,0,0)):
 c=bpy.data.curves.new(n,'FONT');c.body=s;c.size=size*100;c.align_x='CENTER';c.extrude=0;c.resolution_u=2;o=bpy.data.objects.new(n,c);sc.collection.objects.link(o);o.location=Vector(p)*100;o.rotation_euler=rot;c.materials.append(m);parts.setdefault(current,[]).append(o);return o
glass.diffuse_color=(.12,.24,.26,.12)
glass.node_tree.nodes.get('Principled BSDF').inputs['Alpha'].default_value=.12
# chassis and machine room: cab areas are open, with separate real wall openings
group('Exterior','exterior')
box('Underframe',(0,0,1.3),(18.9,2.95,.42),dark,.045)
box('MachineRoom',(0,0,2.57),(13.9,3.1,2.12),ivory,.075)
box('Roof',(0,0,3.84),(17.9,3.04,.25),grey,.11)
for s in [-1,1]:
 box('RedBand',(0,s*1.557,2.49),(14,.018,.22),red)
 for x in [-5.7,-.7,5.7]:
  box('SideGrille',(x,s*1.575,3.07),(1.25,.035,.88),dark,.015)
  for k in range(14):box('Louver',(x,s*1.6,2.67+k*.064),(1.29,.025,.025),grey)
 text('SideClass','INDIAN RAILWAYS',(2.0,s*1.577,3.02),.35,dark,(math.pi/2,0,math.pi if s==1 else 0))
 text('Railways','30201   WAP-7',(0,s*1.577,2.03),.28,dark,(math.pi/2,0,math.pi if s==1 else 0))
 for x in [-8.25,8.25]:
  e=1 if x>0 else -1
  box('SideLowerCab',(x,s*1.54,2.08),(2.5,.08,1.15),ivory,.025)
  box('CabSideHeader',(x,s*1.54,3.63),(2.5,.08,.27),ivory,.035)
  for xx in [e*7.04,e*7.85,e*8.15,e*9.27]:box('CabSidePillar',(xx,s*1.54,3.03),(.15,.08,1.12),ivory,.025)
  
  for xx in [e*8.22,e*9.18]:box('SideWindowSeal',(xx,s*1.554,3.14),(.065,.03,.76),dark,.018)
  for zz in [2.78,3.50]:box('SideWindowSeal',(e*8.7,s*1.554,zz),(1.02,.03,.06),dark,.018)
  box('SideWindowGlass',(e*8.7,s*1.58,3.14),(.89,.016,.63),glass,.06)
  box('CabDoor',(e*7.5,s*1.596,2.57),(.66,.035,1.98),grey,.04)
  box('CabDoorPaint',(e*7.5,s*1.62,2.57),(.6,.022,1.91),ivory,.025)
  
  for xx in [e*7.30,e*7.70]:box('DoorWindowSeal',(xx,s*1.64,3.17),(.045,.02,.6),dark,.015)
  for zz in [2.89,3.45]:box('DoorWindowSeal',(e*7.5,s*1.64,zz),(.43,.02,.045),dark,.015)
  box('DoorGlass',(e*7.5,s*1.66,3.17),(.35,.012,.5),glass,.04)
  box('SideStripe',(x,s*1.65,2.49),(2.5,.012,.22),red)
  for z in [1.77,2.69]:box('DoorHinge',(e*7.2,s*1.67,z),(.08,.035,.12),steel,.012)
  rod('DoorHandle',(e*7.71,s*1.69,2.65),(e*7.71,s*1.69,2.81),.018,steel)
  for z in [.72,.98,1.24]:box('AccessStep',(e*7.5,s*1.55,z),(.52,.36,.055),steel)
  for xx in [e*7.06,e*7.94]:rod('GrabRail',(xx,s*1.7,1.6),(xx,s*1.7,3.0),.018,steel)
# end faces with twin inset screens and central sloping brow
for end in [-1,1]:
 x=end*9.43
 box('Nose',(x,0,2.05),(.17,3.04,1.13),ivory,.06)
 box('EndBand',(x+end*.09,0,2.49),(.02,3.02,.22),red)
 box('Brow',(x-end*.09,0,3.66),(.25,3.06,.25),ivory,.05)
 for y in [-1.43,0,1.43]:box('WindshieldPillar',(x, y,3.14),(.15,.13,1.0),ivory,.025)
 for y in [-.72,.72]:
  for yy in [y-.615,y+.615]:box('WindowRubber',(x+.005*end,yy,3.17),(.035,.04,.82),dark,.012)
  for zz in [2.78,3.56]:box('WindowRubber',(x+.005*end,y,zz),(.035,1.27,.04),dark,.012)
  box('Windshield',(x+.028*end,y,3.17),(.02,1.16,.71),glass,.025)
  # protective bars visible on many classic WAP7 units
  for z in [2.91,3.1,3.29,3.47]:rod('WindowGuard',(x+end*.11,y-.58,z),(x+end*.11,y+.58,z),.012,steel)
  group('Wiper_'+str(end)+'_'+str(y),'wiper',(x+end*.14,y,2.86))
  rod('WiperArm',(x+end*.14,y,2.86),(x+end*.14,y+.26,3.35),.012,dark)
  rod('WiperBlade',(x+end*.15,y+.12,3.17),(x+end*.15,y+.39,3.48),.023,dark)
  group('Exterior','exterior')
 for y in [-.18,.18]:
  cyl('HeadlightHousing',(x+end*.13,y,2.19),.15,.12,dark,'X');cyl('HeadlightLens',(x+end*.2,y,2.19),.114,.012,lamp,'X')
 for y in [-1.18,1.18]:
  cyl('MarkerHousing',(x+end*.1,y,2.31),.1,.1,dark,'X');cyl('MarkerLens',(x+end*.16,y,2.31),.067,.012,red,'X')
  cyl('BufferStem',(end*9.79,y,1.06),.13,.58,steel,'X');box('Buffer',(end*10.21,y,1.06),(.14,.5,.38),dark,.07)
 box('Coupler',(end*10.07,0,.97),(.4,.29,.3),steel,.04)
 box('Pilot',(end*9.53,0,.62),(.14,2.9,.5),dark,.04)
 for y in [-1.2,-.8,-.4,0,.4,.8,1.2]:rod('PilotBrace',(end*9.62,y,.39),(end*9.66,y,.82),.025,steel)
 text('EndNumber','30201',(x+end*.1,0,1.72),.22,dark,(math.pi/2,0,end*math.pi/2))
 # flag at centre upper nose, geometry not image
 for zz,mm in [(2.7,red),(2.65,white),(2.6,green)]:box('Flag',(x+end*.108,0,zz),(.015,.34,.045),mm)
 for y in [-.54,.54]:
  rod('BrakeHose',(end*9.7,y,1.05),(end*9.94,y,.74),.034,dark)
# underbody transformer tanks, equipment, piping
box('Transformer',(0,0,.85),(3.3,1.7,.72),grey,.1)
for s in [-1,1]:
 cyl('AirReservoir',(0,s*1.12,.88),.27,2.45,grey,'X')
 for x in [-2.25,2.25]:box('EquipmentBox',(x,s*.88,.91),(.8,1.0,.67),grey,.06)
 rod('UnderframePipe',(-6,s*1.4,1.16),(6,s*1.4,1.16),.025,steel)
# two Co-Co bogies and six wheelsets, wheel radius .546m
for bi,bx in enumerate([-5.98,5.98]):
 group('Bogie_'+str(bi),'bogie',(bx,0,.75))
 box('BogieBolster',(bx,0,.92),(4.7,1.7,.35),grey,.07)
 for s in [-1,1]:
  box('BogieSideFrame',(bx,s*1.17,.82),(4.9,.23,.35),grey,.07)
  for dx in [-1.85,0,1.85]:
   box('AxleBox',(bx+dx,s*1.23,.55),(.48,.27,.33),grey,.04)
   for dd in [-.34,.34]:
    cyl('SpringCore',(bx+dx+dd,s*1.18,.91),.105,.36,dark)
    for z in [.78,.85,.92,.99,1.06]:cyl('SpringRib',(bx+dx+dd,s*1.18,z),.13,.024,steel,v=12)
   box('BrakeShoe',(bx+dx+.49,s*1.04,.47),(.12,.19,.4),dark,.025)
 for wi,dx in enumerate([-1.85,0,1.85]):
  group('Wheelset_'+str(bi)+'_'+str(wi),'wheelset',(bx+dx,0,.546))
  cyl('Axle',(bx+dx,0,.546),.105,2.26,steel,'Y')
  for s in [-1,1]:
   cyl('Wheel',(bx+dx,s*.91,.546),.546,.14,dark,'Y',32)
   cyl('Tread',(bx+dx,s*.91,.546),.548,.105,steel,'Y',32)
   cyl('WheelFace',(bx+dx,s*.986,.546),.45,.026,grey,'Y',24)
   cyl('WheelHub',(bx+dx,s*1.02,.546),.16,.11,steel,'Y',20)
# roof details and pantograph; lower/upper pivot animation can be runtime driven
group('Exterior','exterior')
for x in [-3,-1,1,3]:
 box('RoofVent',(x,0,4.02),(1.55,1.5,.15),dark,.04)
 for y in [-.56,-.28,0,.28,.56]:box('RoofSlat',(x,y,4.11),(1.45,.07,.03),grey)
for x in [-5.7,5.7]:
 for y in [-.7,.7]:
  for z in [4.02,4.1,4.18]:cyl('PantoInsulator',(x,y,z),.14,.06,brown,v=12)
 box('PantoBase',(x,0,4.25),(1.8,1.65,.08),steel)
 group('PantoLower_'+str(x),'pantograph_lower',(x-.72,0,4.3))
 for y in [-.52,.52]:rod('PantoLower',(x-.72,y,4.3),(x+.65,y,5.03),.035,red)
 rod('PantoTie',(x+.65,-.52,5.03),(x+.65,.52,5.03),.032,steel)
 group('PantoUpper_'+str(x),'pantograph_upper',(x+.65,0,5.03))
 for y in [-.52,.52]:rod('PantoUpper',(x+.65,y,5.03),(x-.55,y,5.64),.025,steel)
 group('PantoShoe_'+str(x),'pantograph_shoe',(x-.55,0,5.66))
 box('PantoShoe',(x-.55,0,5.66),(.12,1.95,.055),dark,.025)
 for y in [-1,1]:rod('PantoHorn',(x-.55,y*.95,5.66),(x-.55,y*1.17,5.47),.022,steel)
 group('Exterior','exterior')
for x in [-2.4,0,2.4]:
 for z in [4.03,4.12,4.21,4.3]:cyl('RoofInsulator',(x,1.04,z),.105,.065,brown,v=12)
rod('HTBus',(-5.7,1.04,4.4),(5.7,1.04,4.4),.021,steel)
# Cab 1 is a usable interior. Cab 2 contains equivalent shell and seating.
for end in [-1,1]:
 group('Cab_'+str(end),'cab')
 box('CabFloor',(end*8.22,0,1.6),(2.35,2.94,.08),dark)
 box('CabRearWall',(end*7.02,0,2.63),(.09,2.93,2.02),green)
 box('EquipmentDoor',(end*7.08,0,2.57),(.04,.7,1.73),grey,.025)
 box('DeskBase',(end*8.98,0,1.98),(.61,2.72,.69),green,.045)
 box('DeskTop',(end*8.9,0,2.35),(.86,2.74,.13),green,.04)
 # Three black panels matching the pressure / electrical / diagnostic arrangement.
 rot=(math.pi/2,0,-end*math.pi/2)
 for y,w in [(-.94,.62),(-.12,.94),(.93,.99)]:
  box('InstrumentPanel',(end*9.1,y,2.55),(.12,w,.43),black,.014)
  for yy in [y-w/2+.025,y+w/2-.025]:
   for z in [2.36,2.74]:cyl('PanelScrew',(end*9.025,yy,z),.011,.012,steel,'X',8)
 # Four compact pressure instruments grouped left, two by two.
 for i,(y,z) in enumerate([(-1.1,2.65),(-.79,2.65),(-1.1,2.44),(-.79,2.44)]):
  cyl('GaugeRim',(end*9.019,y,z),.092,.023,steel,'X',24)
  cyl('GaugeFace',(end*9.002,y,z),.079,.008,white if i==2 else dark,'X',24)
  for a in range(-130,131,26):
   aa=math.radians(a)
   rod('DialTick',(end*8.991,y+math.sin(aa)*.063,z+math.cos(aa)*.063),(end*8.991,y+math.sin(aa)*.072,z+math.cos(aa)*.072),.002,white if i!=2 else dark)
  group('Gauge_'+str(end)+'_'+str(i),'gauge_brake',(end*8.983,y,z));rod('Needle',(end*8.983,y,z),(end*8.983,y-.042,z+.039),.003,red);group('Cab_'+str(end),'cab')
  text('GaugeUnit','bar',(end*8.98,y,z-.037),.018,white if i!=2 else dark,rot)
 for i,y in enumerate([-.49,-.35,-.21]):
  box('MeterBezel',(end*9.018,y,2.63),(.035,.1,.18),steel,.008)
  box('MeterFace',(end*8.992,y,2.63),(.01,.077,.153),white)
  group('PowerNeedle_'+str(end)+'_'+str(i),'gauge_power',(end*8.98,y,2.57));rod('MeterNeedle',(end*8.98,y,2.57),(end*8.98,y+.018,2.65),.003,dark);group('Cab_'+str(end),'cab')
 for y in [.0,.12,.24]:cyl('Indicator',(end*9.015,y,2.66),.028,.02,red,'X',12)
 for y in [-.46,-.29,-.12,.05,.22]:
  cyl('SwitchBase',(end*9.012,y,2.44),.022,.018,steel,'X',12)
  rod('PanelSwitch',(end*8.99,y,2.44),(end*8.955,y,2.47),.007,dark)
  text('SwitchLabel','ON',(end*8.98,y,2.38),.017,white,rot)
 box('DiagnosticBezel',(end*9.017,.7,2.54),(.055,.51,.33),grey,.008)
 box('DiagnosticLCD',(end*8.978,.7,2.6),(.012,.4,.076),screen)
 text('DiagnosticText','SYSTEM READY',(end*8.965,.7,2.588),.025,white,rot)
 for y in [.56,.67,.78]:
  for z in [2.43,2.48]:box('DiagnosticKey',(end*8.965,y,z),(.012,.04,.025),dark,.003)
 for y in [1.07,1.24]:
  for z in [2.42,2.56,2.68]:cyl('RightPanelButton',(end*9.011,y,z),.022,.02,red if z==2.56 else steel,'X',12)
 # Speed recorder on windscreen pillar.
 box('SpeedRecorder',(end*8.96,0,3.1),(.12,.24,.36),black,.012)
 cyl('SpeedDial',(end*8.89,0,3.16),.098,.015,white,'X',24)
 group('SpeedNeedle_'+str(end),'gauge_speed',(end*8.874,0,3.16));rod('SpeedNeedle',(end*8.874,0,3.16),(end*8.874,-.071,3.19),.003,dark);group('Cab_'+str(end),'cab')
 for a in range(-120,121,24):
  aa=math.radians(a);rod('SpeedTick',(end*8.873,math.sin(aa)*.078,3.16+math.cos(aa)*.078),(end*8.873,math.sin(aa)*.089,3.16+math.cos(aa)*.089),.002,dark)
 text('SpeedUnit','km/h',(end*8.873,0,3.02),.025,white,rot)
 for y,label in [(-1.03,'AUTO BRAKE'),(-.65,'LOCO BRAKE'),(.85,'THROTTLE')]:
  box('ControlBase',(end*8.65,y,2.43),(.23,.2,.055),black,.008)
  text('ControlLabel',label,(end*8.5,y,2.43),.026,white,(0,0,-end*math.pi/2))
  role='throttle' if y>0 else ('train_brake' if y<-1 else 'reserved')
  group(role+'_'+str(end),role,(end*8.65,y,2.47))
  cyl('LeverCollar',(end*8.65,y,2.48),.07,.045,brown,v=16)
  rod('Lever',(end*8.65,y,2.5),(end*8.59,y,2.63),.012,steel)
  box('Grip',(end*8.59,y,2.64),(.055,.11,.044),dark,.014)
  group('Cab_'+str(end),'cab')
 for y in [-.36,-.2,-.04,.12,.28,.44]:
  cyl('DeskSwitch',(end*8.65,y,2.43),.026,.026,black,v=12)
  rod('DeskToggle',(end*8.65,y,2.44),(end*8.63,y,2.485),.007,steel)
 group('Reverser_'+str(end),'reverser',(end*8.55,.58,2.47));rod('ReverserLever',(end*8.55,.58,2.47),(end*8.55,.58,2.55),.012,steel);box('ReverserGrip',(end*8.55,.58,2.56),(.04,.09,.025),dark,.008);group('Cab_'+str(end),'cab')
 for y in [-.94,.93]:
  cyl('SeatPedestal',(end*7.95,y,1.94),.12,.65,steel)
  box('SeatCushion',(end*7.95,y,2.24),(.55,.54,.14),dark,.07)
  box('SeatBack',(end*7.68,y,2.57),(.13,.54,.62),dark,.07)
 box('CabLamp',(end*8.0,0,3.69),(.45,.24,.04),lamp,.025)
# Additional roof equipment, traction links and underbody details.
group('Exterior','exterior')
for e in [-1,1]:
 for y in [-.6,.6]:
  cyl('HornBody',(e*8.3,y,4.06),.085,.35,grey,'X',16)
  cyl('HornMouth',(e*8.5,y,4.06),.14,.055,dark,'X',16)
  rod('RoofGrab',(e*8.2,y,4.13),(e*7.7,y,4.13),.018,steel)
for bx in [-5.98,5.98]:
 for y in [-1.31,1.31]:
  for dx in [-1.85,0,1.85]:
   rod('TractionLink',(bx+dx-.55,y,.68),(bx+dx+.42,y,.5),.035,grey)
   cyl('BearingCap',(bx+dx,y,.55),.16,.075,steel,'Y',16)
  rod('BrakeRigging',(bx-2.25,y,.43),(bx+2.25,y,.43),.028,grey)
for x in [-1.2,-.6,0,.6,1.2]:box('TransformerRib',(x,-.9,.83),(.06,.1,.63),steel,.012)
# The front face leans back above the lower nose; corners taper to 2.75 m.
for name,obs in parts.items():
 if name!='Exterior' and not name.startswith('Wiper'):continue
 for ob in obs:
  if ob.type!='MESH':continue
  for v in ob.data.vertices:
   w=ob.matrix_world@v.co
   if abs(w.x)>870:
    t=min(1,max(0,(abs(w.x)-870)/73))
    w.x-=math.copysign(max(0,w.z-165)*.24*t,w.x)
    if abs(w.y)>132:w.y=math.copysign(abs(w.y)-(abs(w.y)-132)*t*.8,w.y)
    v.co=ob.matrix_world.inverted()@w
# Procedural surface variation for source render. FBX uses material base values.
for material in [ivory,grey,green,steel,red]:
 nt=material.node_tree;p=nt.nodes.get('Principled BSDF')
 noise=nt.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=35;noise.inputs['Detail'].default_value=3
 ramp=nt.nodes.new('ShaderNodeValToRGB');c=material.diffuse_color
 ramp.color_ramp.elements[0].position=.2;ramp.color_ramp.elements[0].color=(c[0]*.62,c[1]*.62,c[2]*.62,1)
 ramp.color_ramp.elements[1].position=.8;ramp.color_ramp.elements[1].color=tuple(c)
 nt.links.new(noise.outputs['Fac'],ramp.inputs[0]);nt.links.new(ramp.outputs[0],p.inputs['Base Color'])
 bump=nt.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.12;bump.inputs['Distance'].default_value=.025
 nt.links.new(noise.outputs['Fac'],bump.inputs['Height']);nt.links.new(bump.outputs['Normal'],p.inputs['Normal'])
# Orient the desk layout with pressure instruments to driver's left.
for name,obs in parts.items():
 if roles[name] not in ['cab','gauge_brake','gauge_speed','gauge_power','throttle','train_brake','reserved','reverser']:continue
 pivots[name].y=-pivots[name].y
 for ob in obs:
  if ob.type=='FONT':ob.location.y=-ob.location.y;continue
  for v in ob.data.vertices:
   w=ob.matrix_world@v.co;w.y=-w.y;v.co=ob.matrix_world.inverted()@w
# Bake a neutral wear multiplier into exportable vertex colors.
for name,obs in parts.items():
 for ob in obs:
  if ob.type!='MESH':continue
  bm=bmesh.new();bm.from_mesh(ob.data)
  long_edges=[e for e in bm.edges if e.calc_length()>200]
  if long_edges:bmesh.ops.subdivide_edges(bm,edges=long_edges,cuts=10,use_grid_fill=True)
  bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
  attr=ob.data.color_attributes.new(name='Wear',type='BYTE_COLOR',domain='CORNER')
  for poly in ob.data.polygons:
   matname=ob.data.materials[poly.material_index].name
   clean=any(k in matname for k in ['Letter','Lamp','LCD','Glass'])
   for li in poly.loop_indices:
    w=ob.matrix_world@ob.data.vertices[ob.data.loops[li].vertex_index].co
    n=noise.noise_vector(w*.035)[0];v=1.0 if clean else max(.60,min(1,.85+n*.16-max(0,150-w.z)*.0008))
    attr.data[li].color=(v,v,v,1)
# merge each semantic group, center mesh about runtime pivot; convert font to mesh
assembly=[]
for name,objs in parts.items():
 bpy.ops.object.select_all(action='DESELECT')
 for ob in objs:ob.select_set(True)
 bpy.context.view_layer.objects.active=objs[0];bpy.ops.object.convert(target='MESH');bpy.ops.object.join();ob=bpy.context.object;ob.name='SM_WAP7_01_'+name+'_LOD0'
 sc.cursor.location=pivots[name];bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 ob['semantic_role']=roles[name]
 assembly.append({'name':name,'role':roles[name],'fbx':'Parts/'+ob.name+'.fbx','location_cm':list(ob.location),'rotation_deg':[0,0,0],'scale':[1,1,1],'pivot_cm':list(ob.location),'object':ob.name})
for row in assembly:
 role=row['role'];row['rotation_axis']=[1,0,0] if role.startswith('gauge') or role=='wiper' else [0,1,0]
 row['rotation_range_deg']=[-120,120] if role.startswith('gauge') else [-25,25]
 if role.startswith('pantograph'):
  x=float(row['name'].split('_')[-1]);row['linkage']={'base_cm':[(x-.72)*100,0,430],'elbow_cm':[(x+.65)*100,0,503],'contact_cm':[(x-.55)*100,0,566],'rotation_axis':[0,1,0]}
# Articulated pantograph keyframes solve two rigid links; no scaled geometry.
for x in [-5.7,5.7]:
 lower=bpy.data.objects['SM_WAP7_01_PantoLower_'+str(x)+'_LOD0']
 upper=bpy.data.objects['SM_WAP7_01_PantoUpper_'+str(x)+'_LOD0']
 shoe=bpy.data.objects['SM_WAP7_01_PantoShoe_'+str(x)+'_LOD0']
 base=Vector((x-.72,0,4.3))*100
 a0=math.atan2(.73,1.37);b0=math.atan2(.61,-1.2);l1=math.hypot(.73,1.37)*100;l2=math.hypot(.61,1.2)*100
 for frame in range(1,31):
  t=(frame-1)/29;aa=.06+(a0-.06)*t;bb=math.pi-.06+(b0-math.pi+.06)*t
  elbow=base+Vector((math.cos(aa)*l1,0,math.sin(aa)*l1))
  top=elbow+Vector((math.cos(bb)*l2,0,math.sin(bb)*l2))
  lower.rotation_euler.y=a0-aa;lower.keyframe_insert(data_path='rotation_euler',frame=frame)
  upper.location=elbow;upper.rotation_euler.y=b0-bb;upper.keyframe_insert(data_path='location',frame=frame);upper.keyframe_insert(data_path='rotation_euler',frame=frame)
  shoe.location=top+Vector((0,0,2));shoe.keyframe_insert(data_path='location',frame=frame)
sc.frame_set(30)
os.makedirs(os.path.join(O,'Parts'),exist_ok=True)
for p in assembly:
 ob=bpy.data.objects[p['object']];bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);loc=ob.location.copy();ob.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=os.path.join(O,p['fbx']),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
 ob.location=loc
bpy.ops.object.select_all(action='SELECT');bpy.ops.export_scene.fbx(filepath=os.path.join(O,'SM_WAP7_01.fbx'),use_selection=True,axis_forward='X',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
tri=sum(sum(len(f.vertices)-2 for f in ob.data.polygons) for ob in bpy.data.objects if ob.type=='MESH')
with open(os.path.join(O,'assembly.json'),'w') as f:json.dump({'schema_version':1,'vertex_color_semantics':'Wear RGB multiplies base color; alpha 1; import vertex colors REPLACE','fbx_import_settings':{'convert_scene':True,'force_front_x_axis':True,'convert_scene_unit':True},'family':'WAP7','units':'centimetres','axis':'+X forward +Y right +Z up','parts':assembly,'driver_camera':{'location_cm':[806,-83,303],'rotation_deg':[-12,0,0]},'triangles_lod0':tri,'materials':[{'name':m.name,'base_color':list(m.diffuse_color),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value} for m in bpy.data.materials if m.use_nodes and m.node_tree.nodes.get('Principled BSDF')],'status':'PROVISIONAL: Android profiling and authentic cab variant review pending'},f,indent=2)
# presentation-only staging, excluded from export
parts={};group('STAGE','stage')
box('Stage',(0,0,-.2),(200,200,.25),mat('Stage',(.06,.085,.105)))
for y in [-.838,.838]:box('Rail',(0,y,-.025),(27,.07,.11),steel)
for x in range(-13,14):box('Sleeper',(x,0,-.11),(.22,2.5,.13),grey)
def camera(n,p,target,lens=48):
 bpy.ops.object.camera_add(location=Vector(p)*100);o=bpy.context.object;o.name=n;o.rotation_euler=(Vector(target)*100-o.location).to_track_quat('-Z','Y').to_euler();o.data.lens=lens;o.data.clip_end=100000;sc.camera=o;return o
for n,p,power,size in [('Key',(8,-10,15),180000000,1000),('Fill',(-5,8,10),100000000,900)]:
 bpy.ops.object.light_add(type='AREA',location=Vector(p)*100);ob=bpy.context.object;ob.name=n;ob.data.energy=power;ob.data.shape='DISK';ob.data.size=size;ob.rotation_euler=(-ob.location).to_track_quat('-Z','Y').to_euler()
sc.world.color=(.22,.22,.22);sc.render.engine='CYCLES';sc.cycles.samples=32;sc.render.resolution_x=1600;sc.render.resolution_y=1000;sc.render.resolution_percentage=100
camera('ExteriorReview',(24,-29,8.5),(0,0,2.1),48)
sc.render.filepath=os.path.join(O,'WAP7_exterior.png');bpy.ops.render.render(write_still=True)
# cab review, interior lighting without hiding production shell
bpy.ops.object.light_add(type='AREA',location=(810,0,360));bpy.context.object.data.energy=150000;bpy.context.object.data.size=170
camera('CabReview',(7.4,0,3.03),(9.02,0,2.72),20)
sc.render.filepath=os.path.join(O,'WAP7_cab.png');bpy.ops.render.render(write_still=True)
camera('DriverCamera',(8.06,-.83,3.03),(13,-.83,2.55),24)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(O,'WAP7.blend'))
print('WAP7_COMPLETED',tri,len(assembly))
