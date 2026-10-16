# Reference-led refinement, generated geometry only; photographs are review references.
def remove_prefix(prefixes):
 for o in list(bpy.data.objects):
  if o.name.startswith(prefixes):bpy.data.objects.remove(o,do_unlink=True)
def face(n,verts,ma):
 me=bpy.data.meshes.new(n);me.from_pydata(verts,[],[list(range(len(verts)))]);me.update();o=bpy.data.objects.new(n,me);bpy.context.collection.objects.link(o);o.data.materials.append(ma);return o
def cabface(n,sign,coords,ma,offset=0):
 return face(n,[(sign*(9.35-max(0,z-2.7)*.28+offset),y,z) for y,z in coords],ma)
remove_prefix(('Cab nose','Cab fascia','Windscreen','Cab lower side','Cab window header','Cab window rear pillar','Cab side sill','Roof','Front yellow','Front number','Twin headlight','Vent backing','Vent slat','Class lettering','Number','Pilot','Pressure','Gauge','Driver display','Display readout','Instrument fascia'))
# continuous roof crown and bevel shoulders
cube('Roof crown',(0,0,4.05),(17.85,2.38,.18),ivory,.08)
for y in (-1,1):
 o=cube('Roof shoulder',(0,y*1.36,3.98),(17.9,.38,.17),ivory,.07);o.rotation_euler[0]=y*-.35
for sign in (-1,1):
 cabface('Sloping cab lower',sign,[(-1.3,1.46),(1.3,1.46),(1.3,2.89),(-1.3,2.89)],green)
 cabface('Sloping cab header',sign,[(-1.3,3.64),(1.3,3.64),(1.15,4.02),(-1.15,4.02)],green)
 for y in (-1,1):
  face('Cab chamfer',[(sign*9.35,y*1.3,1.46),(sign*8.9,y*1.56,1.46),(sign*8.72,y*1.56,3.85),(sign*8.98,y*1.3,4.02)],green)
  cube('Cab lower side',(sign*7.88,y*1.54,2.18),(2.0,.10,1.44),green)
  cube('Cab window header',(sign*7.9,y*1.54,3.8),(1.85,.1,.30),green)
  cube('Cab rear pillar',(sign*7.08,y*1.54,3.27),(.2,.1,.8),green)
  for yy in (y*.055,y*1.29):cabface('Windscreen pillar',sign,[(yy-.045,2.88),(yy+.045,2.88),(yy+.045,3.68),(yy-.045,3.68)],green,.005)
  cabface('Windscreen',sign,[(y*.12,2.95),(y*1.20,2.95),(y*1.20,3.61),(y*.12,3.61)],glass,.012)
  # protective front window bars, slim enough for driving view
  for i in range(9):
   yy=y*(.18+i*.118);rod('Window protection bar',(sign*9.33,yy,2.95),(sign*9.14,yy,3.61),.009,steel)
  for z in (2.94,3.62):rod('Window protection frame',(sign*(9.39-(z-2.7)*.28),y*.08,z),(sign*(9.39-(z-2.7)*.28),y*1.24,z),.017,steel)
  cabface('Chevron',sign,[(0,1.67),(y*.78,2.47),(y*1.30,2.47),(y*1.30,2.70),(y*.56,2.70)],yellow,.018)
  text('Front identification','WAG-9' if y<0 else '31082',(sign*9.38,y*.85,1.83),.16,white,(math.pi/2,0,sign*math.pi/2))
 # low twin central headlights
 cube('Headlight recess',(sign*9.38,0,2.43),(.06,.58,.32),steel,.015)
 for y in (-.145,.145):
  cyl('Twin headlight housing',(sign*9.43,y,2.43),.125,.09,silver,'X');cyl('Twin headlight glass',(sign*9.485,y,2.43),.09,.015,light,'X')
 for y in (-1.2,1.2):rod('Front handrail',(sign*9.41,y,1.75),(sign*9.30,y,2.84),.015,silver)
 rod('Front handrail',(sign*9.30,-1.2,2.84),(sign*9.30,1.2,2.84),.015,silver)
 for y in [-1.25+i*.25 for i in range(11)]:rod('Pilot grille',(sign*9.48,y,.34),(sign*9.2,y,.92),.027,steel)
 for z in (.38,.65,.9):rod('Pilot beam',(sign*9.40,-1.36,z),(sign*9.40,1.36,z),.043,steel)
 for y in (-.6,.6):cyl('Roof horn',(sign*8.6,y,4.22),.075,.32,steel,'X')
for y in (-1.58,1.58):
 for x in (-4.8,4.8):
  cube('Vent frame',(x,y,3.28),(1.1,.04,1.08),silver,.02)
  cube('Vent backing',(x,y*1.02,3.28),(.98,.025,.96),black,.01)
  for i in range(16):cube('Vent slat',(x-.44+i*.058,y*1.025,3.28),(.018,.024,.91),steel,.001)
 text('Side railway','INDIAN RAILWAYS',(0,y*1.014,3.27),.39,white,(math.pi/2,0,0 if y<0 else math.pi))
 text('Side number','31082',(0,y*1.014,1.95),.36,white,(math.pi/2,0,0 if y<0 else math.pi))
 # move waist stripe outside body instead of accidentally hidden within it
for o in bpy.data.objects:
 if o.name.startswith('Yellow waist'):o.location.y=math.copysign(1.601,o.location.y);o.dimensions.z=.29
# bogie deep cast frame, axle housings, brake rigging and coil springs
for bx in (-6,6):
 for y in (-1.22,1.22):
  cube('Bogie upper beam',(bx,y,1.02),(4.7,.22,.19),steel,.035)
  for dx in (-1.85,0,1.85):
   cube('Axle box casting',(bx+dx,y,.59),(.37,.32,.35),steel,.045)
   cyl('Axle bearing cap',(bx+dx,y*1.15,.59),.12,.05,silver,'Y',16)
   for sx in (-.31,.31):
    cube('Bogie spring seat',(bx+dx+sx,y,.65),(.26,.30,.09),steel)
    pts=[(bx+dx+sx+.10*math.cos(i*.48),y+.10*math.sin(i*.48),.70+i*.004) for i in range(80)]
    verts=[];faces=[]
    for i,pt in enumerate(pts):
     for j in range(6):
      a=j*math.tau/6;verts.append((pt[0]+.022*math.cos(a),pt[1]+.022*math.sin(a),pt[2]+.022*math.sin(a)))
    for i in range(len(pts)-1):
     for j in range(6):faces.append((i*6+j,i*6+(j+1)%6,(i+1)*6+(j+1)%6,(i+1)*6+j))
    me=bpy.data.meshes.new('Primary coil');me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new('Primary coil',me);bpy.context.collection.objects.link(o);o.data.materials.append(steel)
   rod('Bogie brake hanger',(bx+dx+.47,y,1.03),(bx+dx+.47,y,.43),.045,steel)
   cube('Bogie brake shoe',(bx+dx+.45,y*.83,.44),(.13,.25,.27),steel)
  for dx in (-.95,.95):
   cube('Bogie secondary spring seat',(bx+dx,y,1.16),(.55,.42,.14),steel)
   rod('Bogie inclined link',(bx+dx-.25,y,.57),(bx+dx+.22,y,1.08),.075,steel)
  rod('Bogie brake pull rod',(bx-2.1,y*1.1,.4),(bx+2.1,y*1.1,.4),.022,silver)
# interior panels based on WAG9 31132 reference: pressure bank + electrical meters.
inside_before=set(bpy.data.objects)
for o in bpy.data.objects:
 if o.name.startswith('Desk pedestal'):o.data.materials.clear();o.data.materials.append(ivory)
cube('Instrument fascia',(8.77,0,2.73),(.10,2.68,.66),ivory,.03)
cube('Pressure instrument panel',(8.70,-.82,2.76),(.025,.93,.59),black,.012)
for j,(y,z) in enumerate([(-1.04,2.91),(-.62,2.91),(-1.04,2.62),(-.62,2.62)]):
 cyl('Pressure gauge rim',(8.667,y,z),.128,.026,silver,'X',32);cyl('Pressure dial',(8.646,y,z),.115,.012,white if j==2 else black,'X',32)
 for i in range(25):
  ang=-2.35+i*4.7/24
  rod('Gauge graduation',(8.636,y+math.sin(ang)*.091,z+math.cos(ang)*.091),(8.636,y+math.sin(ang)*.103,z+math.cos(ang)*.103),.0025,black if j==2 else white)
 rod('Gauge needle',(8.626,y,z),(8.626,y-.06,z+.055),.005,red if j==2 else white)
 text('Gauge caption',['BC','MR','BP','FP'][j],(8.625,y,z-.049),.021,white if j!=2 else black,(math.pi/2,0,-math.pi/2))
cube('Electrical instrument panel',(8.70,.04,2.76),(.025,.73,.59),black,.012)
for y,label in [(-.17,'V'),(.02,'kV'),(.19,'A1'),(.34,'A2')]:
 cube('Electrical meter rim',(8.675,y,2.9),(.035,.15,.25),silver,.009);cube('Electrical meter face',(8.65,y,2.9),(.015,.125,.225),white,.004)
 rod('Electrical pointer',(8.637,y-.04,2.84),(8.637,y+.025,2.96),.004,black)
 text('Electrical caption',label,(8.63,y,2.81),.03,black,(math.pi/2,0,-math.pi/2))
for i in range(5):
 y=-.23+i*.13;cube('Rocker switch',(8.65,y,2.58),(.055,.045,.075),steel,.005)
cube('Diagnostic bezel',(8.69,.84,2.82),(.04,.61,.42),black,.013)
cube('Diagnostic screen',(8.66,.84,2.85),(.015,.48,.28),mat('LCD dark',(.025,.075,.09)),.005)
for i,s in enumerate(['LOCO 31082','LINE 25.0 kV','TRACTION READY','SPEED 000']):text('Diagnostic line',s,(8.644,.84,2.94-i*.048),.032,screen,(math.pi/2,0,-math.pi/2))
for i in range(6):cyl('Diagnostic key',(8.648,.61+i*.09,2.64),.018,.02,ivory,'X',12)
for i in range(4):
 cyl('Status lamp',(8.65,.59+i*.15,2.51),.027,.025,red if i==0 else yellow,'X',16)
for o in set(bpy.data.objects)-inside_before:
 for c in list(o.users_collection):c.objects.unlink(o)
 cab.objects.link(o)
# Render surfaces remain see-through; runtime glass parameters are separately declared.
glass.diffuse_color=(.12,.23,.25,.14)
gp=glass.node_tree.nodes.get('Principled BSDF');gp.inputs['Base Color'].default_value=(.5,.65,.68,1);gp.inputs['Alpha'].default_value=.14;gp.inputs['Metallic'].default_value=0;gp.inputs['Transmission Weight'].default_value=.85
