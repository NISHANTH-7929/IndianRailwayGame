"""Egmore heritage study: independently authored metric modules, not a survey.
Uses the existing source helpers; no photographs are read as texture pixels.
"""
from pathlib import Path
helper=Path(__file__).with_name('build_station_kit.py')
setup=helper.read_text().split('box((2.5,2.5,.37)')[0]
exec(compile(setup.replace("S8/StationKit","S8/EgmoreKit"),str(helper),'exec'))
surface('EgmoreRed',[.43,.065,.046],.83,kind='stone')
surface('EgmoreLime',[.79,.76,.65],.83,kind='stone')
surface('EgmoreBlue',[.085,.31,.46],.54,.25,'metal')
surface('EgmoreStone',[.115,.105,.092],.97,kind='aggregate')

def strip(points,width,depth,mat):
    # Shared cross-sections avoid dozens of overlapping bevelled boxes per arch.
    verts=[]
    for i,p in enumerate(points):
        tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])
        normal=Vector((-tangent.z,0,tangent.x)).normalized()*width/2
        for side,dy in [(-1,-depth/2),(1,-depth/2),(1,depth/2),(-1,depth/2)]:
            q=Vector(p)+normal*side+Vector((0,dy,0));verts.append(tuple(q))
    faces=[(3,2,1,0)]
    for i in range(len(points)-1):
        for j in range(4):faces.append((i*4+j,i*4+(j+1)%4,(i+1)*4+(j+1)%4,(i+1)*4+j))
    end=(len(points)-1)*4;faces.append(tuple(range(end,end+4)))
    mesh=bpy.data.meshes.new('continuous moulding');mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new('continuous moulding',mesh);scene.collection.objects.link(o);finish_object(o,mat)

def arch(cx,y,z,width,height,infill='window'):
    # Recessed opening, cream jambs, pointed upper arch and small cusp details.
    radius=width/2;spring=z+height-radius
    # Recessed joinery gives openings depth and readable construction at eye level.
    # Portico arches are actually open; never cover them with black cards.
    if infill:
        box((cx,y+.23,z+(height-radius)/2),(width,.08,height-radius),'Glass' if infill=='window' else 'Wood',0)
        for xx in [cx-radius+.045,cx,cx+radius-.045]:
            box((xx,y+.16,z+(height-radius)/2),(.065,.09,height-radius),'EgmoreLime',.004)
        for zz in [z+.06,z+(height-radius)*.46,spring-.05]:
            box((cx,y+.16,zz),(width,.1,.09),'EgmoreLime',.004)
        if infill=='door':
            for zz in np.arange(z+.2,spring-.1,.16):
                for xx in [cx-width*.25,cx+width*.25]:
                    box((xx,y+.13,zz),(width*.43,.09,.055),'Wood',.004)
        box((cx,y+.04,z-.035),(width+.22,.43,.11),'EgmoreLime',.008)
    # Five-lobed intrados seen in the heritage arcade; avoid generic round arches.
    curve=[]
    for t in np.linspace(0,math.pi,49):
        r=radius*(1-.105*abs(math.sin(t*5)))
        curve.append((cx+r*math.cos(t),y-.03,spring+r*math.sin(t)))
    verts=[(cx-radius,y+.03,spring),(cx+radius,y+.03,spring)]
    verts.extend((x,y+.03,zz) for x,_,zz in curve)
    mesh=bpy.data.meshes.new('arch recess');mesh.from_pydata(verts,[],[tuple(range(len(verts)))]);mesh.update()
    if infill:
        o=bpy.data.objects.new('arched transom',mesh);scene.collection.objects.link(o);finish_object(o,'Glass')
    for x in [cx-radius,cx+radius]:
        cylinder((x,y-.025,z+(height-radius)/2),.085,height-radius,'EgmoreLime',12)
        for zz,w in [(z,.22),(spring-.09,.24),(spring,.29)]:box((x,y-.025,zz),(w,.24,.12),'EgmoreLime',.008)
    strip(curve,.095,.17,'EgmoreLime')
    for t in np.linspace(.20,math.pi-.20,7):
        x=cx+radius*math.cos(t);zz=spring+radius*math.sin(t)
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8,ring_count=4,radius=.10,location=(x,y-.025,zz-.05))
        o=bpy.context.object;o.scale=(.9,1,.65);finish_object(o,'EgmoreLime')

def cornice(z,width=6):
    for zz,depth,h in [(z,.52,.20),(z+.18,.68,.16),(z+.32,.80,.10)]:box((0,-.12,zz),(width,depth,h),'EgmoreLime',.015)
    for x in np.arange(-width/2+.25,width/2,.5):
        box((x,-.21,z-.2),(.16,.34,.32),'EgmoreLime')

def parapet(width,z):
    box((0,0,z),(width,.3,.45),'EgmoreRed')
    for x in np.arange(-width/2+.15,width/2,.30):
        box((x,-.1,z+.29),(.12,.24,.23),'EgmoreLime',.025)
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8,ring_count=4,radius=.085,location=(x,-.1,z+.43))
        finish_object(bpy.context.object,'EgmoreLime')

def platform_face(build, offset):
    """Turn newly authored joinery towards the platform without reflected normals.

    Rear elevations are estimated study details, not a measured reconstruction.
    A rigid rotation preserves winding, UVs and the existing export convention.
    """
    before=set(scene.objects)
    build()
    transform=Matrix.Translation((0,offset,0)) @ Matrix.Rotation(math.pi,4,'Z')
    for obj in set(scene.objects)-before:
        obj.matrix_world=transform @ obj.matrix_world

def arcade_rear():
    for x in [-1.55,0,1.55]:arch(x,-.24,5.15,1.34,2.9)
    for x in [-1.5,1.5]:arch(x,-.25,.85,2.4,3.25,'door')
    for z in [4.55,8.8]:cornice(z)
    box((0,-.15,.55),(6,.35,.35),'EgmoreLime')

def tower_rear():
    arch(0,-.56,.85,2.55,3.5,'door')
    arch(0,-.56,6.6,2.55,3.7)
    for z in [4.6,11.8]:cornice(z,4.8)
    for x in [-1.85,1.85]:box((x,-.54,6),(.14,.12,12),'EgmoreLime')

# Six-metre facade bay. Back shell is inset so openings read as deep apertures.
box((0,.5,4.9),(6,.3,9.8),'EgmoreRed')
for x in [-2.85,2.85]:
    box((x,0,4.8),(.30,.65,9.6),'EgmoreRed')
    box((x,-.36,4.8),(.075,.045,9.6),'EgmoreLime',.003)
for z in [.25,4.4,8.65]:box((0,.12,z),(6,.55,.45),'EgmoreRed')
for x in [-1.55,0,1.55]:arch(x,-.24,5.15,1.34,2.9)
for x in [-1.5,1.5]:arch(x,-.25,.45,2.4,3.65,'door')
for z in [4.55,8.8]:cornice(z)
parapet(6,9.55)
for x in [-2.25,-.75,.75,2.25]:
    box((x,-.2,8.38),(.56,.12,.32),'EgmoreLime')
    # Raised quatrefoil on the cream frieze panels, separately modelled relief.
    for angle in np.linspace(0,math.tau,5)[:-1]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8,ring_count=4,radius=.065,location=(x+.065*math.cos(angle),-.278,8.38+.065*math.sin(angle)))
        o=bpy.context.object;o.scale=(1,.35,1);finish_object(o,'EgmoreRed')
platform_face(arcade_rear,.70)
export('EgmoreArcade6m')

# Tower body and a separate rotational dome allow proper shared instancing.
box((0,3.35,6),(4,.30,12),'EgmoreRed')
for x in [-1.65,1.65]:box((x,1.5,6),(.70,4,12),'EgmoreRed')
for z,h in [(.15,.30),(5.35,2.40),(11.20,1.60)]:
    box((0,-.35,z),(2.60,.30,h),'EgmoreRed')
box((0,1.5,6.35),(4,4,.18),'EgmoreLime')
for x in [-1.85,1.85]:box((x,-.54,6),(.14,.12,12),'EgmoreLime')
arch(0,-.56,.35,2.55,4.0);arch(0,-.56,6.6,2.55,3.7)
for z in [4.6,11.8]:cornice(z,4.8)
for x in [-1.8,1.8]:box((x,-.68,10.8),(.3,.15,.45),'EgmoreLime')
platform_face(tower_rear,3.30)
# Side elevations: shallow blind niches and continuous trim returns. These are
# estimated study details, not invented through-openings in the solid tower.
def tower_side():
    arch(0,-.16,6.6,1.65,3.7,'window')
    for xx in [-1.72,1.72]:box((xx,-.13,6),(.12,.15,12),'EgmoreLime')
    for zz in [4.6,11.8]:cornice(zz,4.0)
    box((0,-.08,.3),(4,.30,.35),'EgmoreLime')
    for xx in [-.6,0,.6]:box((xx,-.15,10.85),(.24,.12,.28),'EgmoreLime')
for side in [-1,1]:
    before=set(scene.objects);tower_side()
    transform=Matrix.Translation((side*2,1.5,0)) @ Matrix.Rotation(side*math.pi/2,4,'Z')
    for obj in set(scene.objects)-before:obj.matrix_world=transform @ obj.matrix_world
export('EgmoreTower4m')

profile=[(0,0),(1.82,0),(1.85,.20),(1.80,.55),(1.64,1.15),(1.36,1.65),(.98,2.05),(.48,2.36),(.19,2.54),(.12,2.7),(0,2.72)]
verts=[];n=48
for r,z in profile:
    verts.extend((r*math.cos(2*math.pi*i/n),r*math.sin(2*math.pi*i/n),z) for i in range(n))
faces=[]
for j in range(len(profile)-1):
    for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
mesh=bpy.data.meshes.new('cream dome');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('cream dome',mesh);scene.collection.objects.link(o);finish_object(o,'EgmoreLime')
for p in mesh.polygons:p.use_smooth=True
cylinder((0,0,2.9),.055,.5,'EgmoreLime')
export('EgmoreDome')

# Projecting entrance pavilion with curved parapet and a usable open portico.
for x in [-4.2,4.2]:
    box((x,1.0,5.0),(.6,5.0,10),'EgmoreRed')
    box((x,-1.55,5.0),(.18,.10,10),'EgmoreLime')
for z in [.15,4.7,9.3]:
    box((0,1,z),(9,5.4,.26),'EgmoreLime')
for x in [-2.8,0,2.8]:
    arch(x,-1.50,5.1,2.3,3.6,None)
for x in [-2.8,0,2.8]:
    for xx in [x-1.15,x+1.15]:
        box((xx,-1.5,2.25),(.3,.4,4.4),'EgmoreLime')
        for z,w in [(.35,.5),(3.8,.42),(4.2,.65)]:box((xx,-1.5,z),(w,.55,.22),'EgmoreLime')
for x in np.arange(-4.1,4.2,.4):
    box((x,-1.65,5.3),(.09,.12,.7),'EgmoreLime')
box((0,-1.65,5.7),(8.6,.28,.16),'EgmoreLime')
outline=[(-4.5,9.55),(-3.9,9.65),(-3.3,9.9),(-2.7,10.3),(-2.2,10.9),(-1.7,11.2),(-1.1,11.35),(0,11.5),(1.1,11.35),(1.7,11.2),(2.2,10.9),(2.7,10.3),(3.3,9.9),(3.9,9.65),(4.5,9.55)]
verts=[(x,-1.55,z) for x,z in outline]+[(4.5,-1.55,9.3),(-4.5,-1.55,9.3)]
mesh=bpy.data.meshes.new('entrance curved parapet');mesh.from_pydata(verts,[],[tuple(range(len(verts)))]);mesh.update()
o=bpy.data.objects.new('entrance curved parapet',mesh);scene.collection.objects.link(o);finish_object(o,'EgmoreLime')
mod=o.modifiers.new('parapet depth','SOLIDIFY');mod.thickness=.25;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
for dz in [0,.13]:strip([(x,-1.70,z+dz) for x,z in outline],.13,.15,'EgmoreLime')
text('CHENNAI EGMORE',(0,-1.88,9.88),.40,'Black')
# Small raised scrolls break up the blank pediment without borrowing photo pixels.
for cx in [-2.8,-1.4,1.4,2.8]:
    points=[]
    for t in np.linspace(0,math.tau*1.35,40):
        r=.25*(1-t/(math.tau*1.6))
        points.append((cx+r*math.cos(t),-1.88,10.42+r*math.sin(t)))
    strip(points,.035,.04,'EgmoreLime')
for x in [-4.25,4.25]:
    box((x,-1.45,10.3),(.45,.45,1.5),'EgmoreLime')
    for dx in [-.22,.22]:
        for dy in [-.22,.22]:cylinder((x+dx,-1.45+dy,11.25),.035,.6,'EgmoreLime',8)
    bpy.ops.mesh.primitive_cone_add(vertices=8,radius1=.47,radius2=.04,depth=.38,location=(x,-1.45,11.70));finish_object(bpy.context.object,'EgmoreLime')
export('EgmoreEntrance9m')

# Shallow continuous corrugated roof, not separated strips with visible gaps.
for x in [.2,5.8]:
    for y in [.5,7.5]:
        box((x,y,.17),(.60,.60,.34),'Black');box((x,y,2.55),(.18,.18,4.8),'EgmoreLime')
    beam((x,.15,4.65),(x,7.85,4.65),.12,'Steel')
    beam((x,.15,4.65),(x,4,5.5),.12,'Steel');beam((x,4,5.5),(x,7.85,4.65),.12,'Steel')
    for j in range(8):
        yy=.15+j*.9625;top=5.5-abs(yy-4)*.221
        beam((x,yy,4.65),(x,yy,top),.055,'Steel')
        beam((x,yy,4.65),(x,min(yy+.9625,7.85),5.5-abs(min(yy+.9625,7.85)-4)*.221),.05,'Steel')
for y in [.1,1.4,2.7,4,5.3,6.6,7.9]:box((3,y,5.53-abs(y-4)*.221),(6,.075,.12),'Steel')
nx=240;verts=[]
for y in [-.05,4,8.05]:
    for i in range(nx+1):
        x=6*i/nx;verts.append((x,y,5.58-abs(y-4)*.221+.012*math.cos(x*math.tau/.10)))
faces=[]
for j in range(2):
    for i in range(nx):a=j*(nx+1)+i;faces.append((a,a+1,a+nx+2,a+nx+1))
mesh=bpy.data.meshes.new('continuous roof');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('continuous roof',mesh);scene.collection.objects.link(o);finish_object(o,'EgmoreBlue')
mod=o.modifiers.new('sheet thickness','SOLIDIFY');mod.thickness=.008;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
for y in [-.05,8.05]:
    box((3,y,4.62),(6,.16,.2),'EgmoreBlue');box((3,y,4.48),(6,.18,.06),'Galvanized')
for x in [.2,5.8]:
    beam((x,.25,0),(x,.25,4.6),.075,'Galvanized')
for x in [1.5,4.5]:box((x,4,4.6),(1.2,.10,.08),'Cream')
export('EgmoreCanopy6m')

# Platform furniture: metric estimates, separate modules for shared instancing.
for x in [-.65,.65]:
    box((x,0,.27),(.12,.48,.54),'Galvanized')
    box((x,0,.04),(.28,.55,.08),'Concrete')
for y in [-.21,-.07,.07,.21]:box((0,y,.49),(1.8,.10,.045),'Wood')
for z in [.68,.83, .98]:box((0,.25,z),(1.8,.055,.10),'Wood')
for x in [-.82,.82]:
    beam((x,-.22,.52),(x,-.22,.73),.045,'Steel')
    beam((x,-.22,.73),(x,.25,.73),.045,'Steel')
export('EgmorePlatformBench')

box((0,0,.65),(1.8,.7,1.3),'EgmoreLime')
box((0,-.38,.95),(1.65,.18,.20),'Galvanized')
for x in [-.55,0,.55]:
    beam((x,-.36,1.10),(x,-.55,1.10),.035,'Galvanized')
    beam((x,-.55,1.10),(x,-.55,1.01),.035,'Galvanized')
    box((x,-.42,.80),(.44,.22,.025),'Steel')
    cylinder((x,-.35,1.2),.045,.03,'Galvanized',12)
box((0,.1,1.5),(1.8,.12,.32),'EgmoreBlue')
text('DRINKING WATER',(0,.025,1.45),.12,'Cream')
export('EgmoreWaterPoint')

for x,mat in [(-.35,'EgmoreBlue'),(.35,'Steel')]:
    cylinder((x,0,.43),.24,.80,mat,24)
    cylinder((x,0,.86),.255,.08,'Galvanized',24)
    box((x,-.24,.72),(.21,.04,.13),'Black')
    box((x,-.246,.43),(.24,.02,.19),'Cream')
beam((-.35,0,.02),(.35,0,.02),.12,'Steel')
export('EgmoreLitterBins')

box((0,0,1.5),(1.25,.18,.80),'EgmoreBlue')
text('PLATFORM 1',(0,-.104,1.46),.14,'Cream')
for x in [-.5,.5]:beam((x,0,1.9),(x,0,2.65),.025,'Steel')
export('EgmorePlatformBoard')

box((1.5,0,-.10),(3,.38,.2),'Concrete')
for x in np.arange(.06,3,.12):box((x,0,.008),(.035,.34,.025),'Steel',.003)
for y in [-.18,.18]:box((1.5,y,.008),(3,.028,.03),'Steel',.003)
export('EgmoreDrain3m')

cylinder((0,0,.38),.032,.76,'Steel',12)
cylinder((0,0,0),.11,.08,'Galvanized',16)
cylinder((0,0,-.10),.13,.14,'Cream',16)
for angle in [0,math.tau/3,math.tau*2/3]:
    o=box((.30*math.cos(angle),.30*math.sin(angle),-.14),(.48,.13,.018),'Cream',.02)
    o.rotation_euler.z=angle
export('EgmoreCeilingFan')

# Ballast shoulder with a coarse stone layer, separate from rails and sleepers.
# Deterministic original geometry; dimensions are presentation estimates.
cross=[(-1.75,-.57),(-1.25,-.29),(1.25,-.29),(1.75,-.57)]
verts=[(x,y,z) for x in [0,3] for y,z in cross]
faces=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
faces=[tuple(reversed(face)) for face in faces]
mesh=bpy.data.meshes.new('ballast shoulder');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('ballast shoulder',mesh);scene.collection.objects.link(o);finish_object(o,'EgmoreStone')
stone_rng=random.Random(1783)
for i in range(520):
    x=stone_rng.uniform(.035,2.965);y=stone_rng.uniform(-1.64,1.64)
    z=-.29-max(0,abs(y)-1.25)*.56
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(x,y,z))
    o=bpy.context.object;o.scale=(stone_rng.uniform(.035,.070),stone_rng.uniform(.030,.060),stone_rng.uniform(.020,.043))
    o.rotation_euler=(stone_rng.random(),stone_rng.random(),stone_rng.random()*math.tau)
    finish_object(o,'EgmoreStone' if i%5 else 'Ballast')
export('EgmoreBallast3m')

assembly={'asset_id':'Station.Egmore.Heritage.01','version':1,'units':'metres','materials':definitions,'parts':parts,
 'source':'Independent reference-based geometry; Egmore photographs listed in EGMORE_REFERENCE_REGISTER.md',
 'status':'architectural study; estimated dimensions; not yet imported or validated for gameplay'}
(OUT/'assembly.json').write_text(json.dumps(assembly,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'EgmoreKit.blend'))

def instance(name,pos):
    o=bpy.data.objects['SM_Station_'+name].copy();o.data=o.data.copy();scene.collection.objects.link(o)
    o.hide_set(False);o.hide_render=False;o.location=pos;return o
for x in [-30,-24,-18,-12,12,18,24,30]:instance('EgmoreArcade6m',(x,0,0))
for x in [-7,7]:instance('EgmoreTower4m',(x,0,0));instance('EgmoreDome',(x,1.5,12.2))
instance('EgmoreEntrance9m',(0,0,0))
for x in range(-30,31,6):instance('EgmoreCanopy6m',(x,10,.84))
for x in [-24,-12,0,12,24]:
    instance('EgmorePlatformBench',(x,14,.84))
    instance('EgmoreCeilingFan',(x,14,5.80))
for x in [-18,18]:instance('EgmoreLitterBins',(x,16,.84))
instance('EgmoreWaterPoint',(-6,12,.84))
instance('EgmorePlatformBoard',(6,17,3.3))
box((0,0,-.2),(85,65,.3),'Concrete')
scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.resolution_x=1800;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Egmore daylight');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.48,.57,.68,1)
bpy.ops.object.light_add(type='SUN');bpy.context.object.data.energy=2;bpy.context.object.rotation_euler=(.5,-.6,-.5)
bpy.ops.object.camera_add(location=(25,-75,15));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,6))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=42;scene.camera=cam
e=ROOT/'07_Documentation/S8/Evidence/Egmore';e.mkdir(parents=True,exist_ok=True)
scene.render.filepath=str(e/'heritage-study.png');bpy.ops.render.render(write_still=True)
print('EGMORE_SOURCE_COMPLETE',len(parts),sum(p['triangles'] for p in parts))
