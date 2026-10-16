"""Staging-only Egmore representative scene, metric source and portable modules."""
import bpy, math, json, shutil, hashlib, random, bmesh
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[5]
OWN=ROOT/'04_Assets/S8/Workstreams/StationArt'
OUT=OWN/'v2.0.0'
assert not (OUT/'READY.lock').exists(), 'Immutable version'
for d in ('Textures','Modules','Previews','Evidence'): (OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
S=bpy.context.scene; S.unit_settings.system='METRIC'
library=bpy.data.collections.new('MODULE_LIBRARY');S.collection.children.link(library)
world=bpy.data.collections.new('PLACEMENTS');S.collection.children.link(world)
mods={}; placements=[]; source_receipts=[]

def load(path, names):
    source_receipts.append({'path':str(path.relative_to(ROOT)), 'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    with bpy.data.libraries.load(str(path),link=False) as (a,b):
        b.objects=[n for n in a.objects if n in names]
        b.materials=list(a.materials)
    for o in b.objects:
        if o:
            library.objects.link(o);o.hide_set(False);o.hide_render=False
            key=o.name.removeprefix('SM_Station_'); mods[key]=o
            o.name=key; o.location=(0,0,0)
load(ROOT/'04_Assets/S8/EgmoreKit/EgmoreKit.blend', ['SM_Station_'+n for n in ['EgmoreArcade6m','EgmoreTower4m','EgmoreDome','EgmoreEntrance9m','EgmoreCanopy6m','EgmorePlatformBench','EgmoreWaterPoint','EgmoreLitterBins','EgmoreDrain3m','EgmoreCeilingFan']])
load(ROOT/'04_Assets/S8/EgmoreSignage/EgmoreSignage.blend',['SM_Station_EgmoreNameboard4m'])
for im in bpy.data.images:
    if im.source=='FILE':
        source=Path(bpy.path.abspath(im.filepath))
        if not source.exists():
            matches=list((ROOT/'04_Assets/S8/EgmoreKit/Textures').glob(source.name))+list((ROOT/'04_Assets/S8/EgmoreSignage/Textures').glob(source.name))
            assert matches, source
            source=matches[0]
        target=OUT/'Textures'/source.name
        if not target.exists():shutil.copy2(source,target)
        im.filepath=str(target)
M={m.name:m for m in bpy.data.materials}
for alias,source in {'RedOxide':'EgmoreRed','Paving':'Concrete','Soil':'EgmoreStone','Roof':'EgmoreBlue'}.items():
    if alias not in M:M[alias]=M[source]
def material(name,color,rough=.7,metal=0):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    M[name]=m;return m
material('RailHead',(.35,.39,.42),.24,.85)
if 'Insulator' not in M:material('Insulator',(.18,.07,.03),.3)
material('RailRust',(.20,.095,.05),.81,.35)
material('LeafLight',(.12,.22,.055),.85);material('LeafDark',(.065,.14,.035),.9)
material('Bark',(.16,.095,.045),.95);material('Asphalt',(.105,.115,.12),.95)
material('UrbanOchre',(.52,.42,.29),.88);material('UrbanGrey',(.42,.43,.39),.9)
material('LampDiffuser',(.85,.83,.67),.35)
# Metric tile material uses original authored maps, not generated coordinates.
tile=material('PlatformTile',(.42,.30,.22),.8)
for suffix in ('BC','N','ORM'):
    src=ROOT/'04_Assets/S8/StationKit/Textures'/f'T_Paving_{suffix}.png'
    target=OUT/'Textures'/src.name
    if not target.exists():shutil.copy2(src,target)
    nd=tile.node_tree.nodes;lk=tile.node_tree.links;p=nd.get('Principled BSDF')
    t=nd.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(target),check_existing=True)
    if suffix=='BC':lk.new(t.outputs['Color'],p.inputs['Base Color'])
    elif suffix=='N':
        t.image.colorspace_settings.name='Non-Color';n=nd.new('ShaderNodeNormalMap');lk.new(t.outputs['Color'],n.inputs['Color']);lk.new(n.outputs['Normal'],p.inputs['Normal'])
    else:
        t.image.colorspace_settings.name='Non-Color';n=nd.new('ShaderNodeSeparateColor');lk.new(t.outputs['Color'],n.inputs['Color']);lk.new(n.outputs['Green'],p.inputs['Roughness'])
M['Paving']=tile

def mesh(name,verts,faces,mat):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new(name,me);S.collection.objects.link(o);o.data.materials.append(M[mat]);return o
def box(loc,dim,mat):
    x,y,z=loc;a,b,c=[v/2 for v in dim]
    return mesh('box',[(x+i*a,y+j*b,z+k*c) for i,j,k in [(-1,-1,-1),(-1,-1,1),(-1,1,-1),(-1,1,1),(1,-1,-1),(1,-1,1),(1,1,-1),(1,1,1)]],[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)],mat)
def beam(a,b,w,mat):
    v=Vector(b)-Vector(a);o=box((0,0,0),(w,w,v.length),mat);o.location=(Vector(a)+Vector(b))/2;o.rotation_euler=v.to_track_quat('Z','Y').to_euler();return o
def cylinder(loc,r,h,mat,n=10):
    bpy.ops.mesh.primitive_cylinder_add(vertices=n,radius=r,depth=h,location=loc);o=bpy.context.object;o.data.materials.append(M[mat]);return o
def module(name,objects):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object
    S.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.name=name
    # UVs in metres: 2m tile, independent of object dimensions and exporter.
    uv=o.data.uv_layers.new(name='UVMap') if not len(o.data.uv_layers) else o.data.uv_layers[0]
    o.data.uv_layers.active=uv
    for p in o.data.polygons:
        axis=max(range(3),key=lambda i:abs(p.normal[i]));ij=[i for i in range(3) if i!=axis]
        for li in p.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[ij[0]]/2,co[ij[1]]/2)
    for c in list(o.users_collection):c.objects.unlink(o)
    library.objects.link(o);mods[name]=o;return o
def place(key,loc,angle=0,scale=(1,1,1),tag=None):
    src=mods[key];o=src.copy();o.data=src.data;world.objects.link(o);o.hide_set(False);o.hide_render=False
    sid=tag or f'EG2.{key}.{sum(p["module"]==key for p in placements):04}'
    o.name=sid;o.location=loc;o.rotation_euler=(0,0,math.radians(angle));o.scale=scale;o['stable_id']=sid
    placements.append({'stableId':sid,'module':key,'positionM':list(loc),'rotationZDeg':angle,'scale':list(scale)})
    return o

# 6m long, 8m wide platform. Coping outer face exactly 1.675m from track centre.
objs=[box((3,4,.36),(6,8,.72),'RedOxide'),box((3,4,.765),(6,8,.09),'Paving')]
for y in (.16,7.84):objs.append(box((3,y,.82),(6,.32,.04),'Cream'))
for y in (.48,7.52):objs.append(box((3,y,.841),(6,.10,.002),'Yellow'))
module('Platform6x8m',objs)
# Proper head/web/foot section. Inside rail-head faces are 1.676m apart.
objs=[]
for y in (-.873,.873):
    objs += [box((3,y,-.0225),(6,.07,.045),'RailHead'),box((3,y,-.09),(6,.016,.09),'RailRust'),box((3,y,-.151),(6,.15,.032),'RailRust')]
for x in [i*.6+.3 for i in range(10)]:
    objs.append(box((x,0,-.25),(.24,2.75,.17),'Concrete'))
    for y in (-.98,-.76,.76,.98):objs.append(box((x,y,-.158),(.13,.07,.035),'RailRust'))
module('Track6m',objs)
module('Ballast6m',[mesh('ballast',[(x,y,z) for x in (0,6) for y,z in [(-1.95,-.56),(-1.43,-.335),(1.43,-.335),(1.95,-.56)]],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],'EgmoreStone')])
stones=[];stone_rng=random.Random(7301)
for i in range(48):
    x=stone_rng.uniform(.07,5.93);y=stone_rng.uniform(-1.82,1.82);z=-.32-max(0,abs(y)-1.43)*.43
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(x,y,z));o=bpy.context.object
    o.scale=(stone_rng.uniform(.035,.07),stone_rng.uniform(.03,.065),stone_rng.uniform(.025,.05));o.rotation_euler=(stone_rng.random(),stone_rng.random(),stone_rng.random()*6.28);o.data.materials.append(M['EgmoreStone' if i%3 else 'Concrete']);stones.append(o)
module('BallastScatter6m',stones)
module('GroundTile24m',[box((12,12,-.70),(24,24,.24),'Soil')])
module('RoadTile12m',[box((6,0,-.305),(12,9,.55),'Asphalt'),box((6,0,-.025),(3,.12,.01),'Cream')])
module('Forecourt6m',[box((3,0,-.23),(6,7,.7),'Paving'),box((3,-3.45,.14),(6,.1,.04),'Cream')])
# Closed 42-riser flight, 0.16m rise, 0.29m run, top joins bridge at z7.56.
prof=[(0,-.22),(12.18,6.50),(12.18,6.72)]
for i in reversed(range(42)):prof.extend([(i*.29,(i+1)*.16),(i*.29,i*.16)])
n=len(prof);v=[(x,y,z) for y in (-1.2,1.2) for x,z in prof]
objs=[mesh('stairs',v,[tuple(reversed(range(n))),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)],'Concrete')]
for y in (-1.23,1.23):
    objs.append(beam((0,y,1.0),(12.18,y,7.72),.045,'Galvanized'))
    for i in range(0,43,3):objs.append(beam((i*.29,y,i*.16),(i*.29,y,i*.16+1),.04,'Galvanized'))
module('BridgeStairs',objs)
objs=[box((0,0,-.15),(3.6,6,.3),'Concrete')]
for x in (-1.8,1.8):
    for z in (.13,1.2,2.7):objs.append(box((x,0,z),(.08,6,.08),'Cream'))
    for y in (-3,0,3):
        objs.append(beam((x,y,0),(x,y,2.75),.085,'Cream'))
    for y in (-3,-1.5,0,1.5):objs.append(beam((x,y,.15),(x,y+1.5,1.2),.045,'Cream'))
for x in (-.97,.97):
    o=box((x,0,2.98),(2.05,6,.05),'EgmoreBlue');o.rotation_euler.y=(.24 if x>0 else -.24);objs.append(o)
module('BridgeDeck6m',objs)
# Open western guardrail only where each stair actually joins the deck.
for centre in (-6,12,30):
    stair=min((-5.675,10.675,27.025),key=lambda y:abs(y-centre));lo=max(-3,stair-centre-1.25);hi=min(3,stair-centre+1.25)
    o=mods['BridgeDeck6m'].copy();o.data=o.data.copy();S.collection.objects.link(o)
    bm=bmesh.new();bm.from_mesh(o.data)
    # Original deck guardrail islands at western side; retain roof/deck.
    delete=[v for v in bm.verts if v.co.x < -1.70 and .04<v.co.z<1.35]
    bmesh.ops.delete(bm,geom=delete,context='VERTS');bm.to_mesh(o.data);bm.free()
    parts=[o]
    for a,b in [(-3,lo),(hi,3)]:
        if b-a>.05:
            for z in (.13,1.2):parts.append(beam((-1.8,a,z),(-1.8,b,z),.08,'Cream'))
            for y in (a,b):parts.append(beam((-1.8,y,0),(-1.8,y,1.2),.08,'Cream'))
    module('BridgeDeckEntry'+str(centre),parts)
module('BridgePier',[box((0,0,3.36),(.55,.55,6.72),'Concrete')])
# Lattice portal spanning four tracks, pole centres outside the corridor.
objs=[]
for y in (-11.0,33.0):
    objs.append(box((0,y,.0),(1,1,.4),'Concrete'))
    for x in (-.16,.16):objs.append(beam((x,y,0),(x,y,7.2),.08,'Galvanized'))
    for z in range(7):objs.append(beam((-.16,y,z),(.16,y,z+1),.045,'Galvanized'))
for z in (6.55,7.2):objs.append(beam((0,-11,z),(0,33,z),.09,'Galvanized'))
for i in range(22):objs.append(beam((0,-11+i*2,6.55),(0,-9+i*2,7.2),.045,'Galvanized'))
for y in (0,5,16.35,21.35):
    objs.append(beam((0,y,6.55),(0,y,5.5),.025,'Steel'))
    for z in (6.06,6.14,6.22,6.30):objs.append(cylinder((0,y,z),.085,.035,'Insulator'))
module('OHEPortal',objs)
module('Wire18m',[beam((0,0,5.5),(18,0,5.5),.012,'Steel'),beam((0,0,6.1),(9,0,5.8),.016,'Steel'),beam((9,0,5.8),(18,0,6.1),.016,'Steel'),beam((9,0,5.5),(9,0,5.8),.008,'Steel')])
module('Cabinet',[box((0,0,.15),(1,.9,.3),'Concrete'),box((0,0,1),(.8,.6,1.4),'Galvanized'),box((0,-.305,1),(.66,.025,1.22),'Cream'),box((.24,-.33,1),(.035,.035,.12),'Black')])
module('CableDuct6m',[box((3,0,.08),(6,.35,.16),'Concrete')]+[box((x,0,.169),(.012,.36,.018),'Black') for x in (0,1,2,3,4,5)])
objs=[box((0,0,.2),(.7,.7,.4),'Concrete'),cylinder((0,0,1.9),.085,3.4,'Galvanized'),box((0,0,3.75),(.50,.20,1.45),'Black')]
for z in (3.30,3.75,4.20):
    o=cylinder((0,-.14,z),.14,.08,'Glass',16);o.rotation_euler.x=math.pi/2;objs.append(o)
    objs.extend([box((0,-.28,z+.16),(.35,.45,.035),'Black'),box((-.17,-.24,z),(.035,.35,.28),'Black'),box((.17,-.24,z),(.035,.35,.28),'Black')])
for x in (-.24,.24):objs.append(beam((x,.27,.3),(x,.27,4.4),.035,'Galvanized'))
for z in [i*.28+.4 for i in range(14)]:objs.append(beam((-.24,.27,z),(.24,.27,z),.025,'Galvanized'))
module('SignalUnbound',objs)
objs=[box((0,0,.16),(.65,.65,.32),'Concrete'),cylinder((0,0,2.8),.065,5.3,'Galvanized'),beam((0,0,5.4),(0,-.7,5.7),.055,'Galvanized'),box((0,-.7,5.68),(.45,.3,.08),'LampDiffuser')]
module('YardLamp',objs)
objs=[box((3,0,.65),(6,.23,1.3),'RedOxide'),box((3,0,1.35),(6,.33,.12),'Cream')]
for x in (0,3,6):objs.append(box((x,0,.9),(.35,.35,1.8),'Cream'))
module('Boundary6m',objs)
objs=[cylinder((0,0,2),.18,4,'Bark')];rng=random.Random(163)
for i in range(9):
    a=i*2.4;end=(math.cos(a)*2,math.sin(a)*2,4.3+(i%3)*.6);objs.append(beam((0,0,2.8),end,.09,'Bark'))
for i in range(90):
    a=i*2.4;r=math.sqrt(rng.random())*2.7;z=5.8-r*.35+rng.uniform(-.65,.85)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=rng.uniform(.45,.9),location=(math.cos(a)*r,math.sin(a)*r,z));o=bpy.context.object;o.scale=(1,.8,.7);o.rotation_euler.z=a;o.data.materials.append(M['LeafDark' if i%3 else 'LeafLight']);objs.append(o)
module('TreeBroadleaf',objs)
for name,paint,h in [('UrbanA','UrbanOchre',8),('UrbanB','UrbanGrey',11)]:
    objs=[box((0,0,h/2),(10,7,h),paint),box((0,0,h),(10.4,7.4,.25),'Cream')]
    for x in (-3,0,3):
        for z in range(2,h,3):objs.extend([box((x,-3.53,z),(1.3,.08,1.4),'Glass'),box((x,-3.65,z+.8),(1.6,.4,.1),'Cream')])
    module(name,objs)

# Facade faces the street (-Y); platform rear elevations face +Y.
for x in range(-57,58,6):
    if abs(x)<6 or 23<abs(x)<31:continue
    place('EgmoreArcade6m',(x,-14,.0))
for x in (-27,27):
    place('EgmoreTower4m',(x,-14,0));place('EgmoreDome',(x,-12.5,12.2))
place('EgmoreEntrance9m',(0,-14,0))
module('HeritageRoof6m',[box((0,0,9.0),(6,8,.18),'Roof')])
for x in range(-57,58,6):place('HeritageRoof6m',(x,-10,0))
for x in range(-60,60,6):place('Forecourt6m',(x,-19.5,0))
for x in range(-120,120,6):
    for y in (0,5,16.35,21.35):
        place('Track6m',(x,y,0));place('Ballast6m',(x,y,0));place('BallastScatter6m',(x,y,0))
    if -108<=x<108:
        for y in (-9.675,6.675,23.025):place('Platform6x8m',(x,y,0))
    if -96<=x<54 or 78<=x<96:
        for y in (-9.675,6.675,23.025):place('EgmoreCanopy6m',(x,y+.56,.84),scale=(1,.86,1))
for x in range(-108,109,24):
    for y in (-6.0,10.675,27.0):
        place('EgmorePlatformBench',(x,y,.84));place('EgmoreLitterBins',(x+3,y,.84))
        if x<54 or x>78:place('EgmoreCeilingFan',(x,y,5.75))
for x in (-90,0,90):
    for y in (-7.5,12.5,29):place('EgmoreWaterPoint',(x,y,.84))
for x in (-99,99):
    for y in (-5.6,10.7,27):place('EgmoreNameboard4m',(x,y,.84))
for x in range(-108,108,3):
    for y in (-9.35,30.7):place('EgmoreDrain3m',(x,y,.84))
for y in (-6,0,6,12,18,24,30):place('BridgeDeckEntry'+str(y) if y in (-6,12,30) else 'BridgeDeck6m',(66,y,7.56))
for y in (-5.675,10.675,27.025):
    place('BridgeStairs',(52.02,y,.84));place('BridgePier',(66,y,.84))
for x in range(-108,109,36):
    place('OHEPortal',(x,0,0))
for x in range(-126,126,18):
    for y in (0,5,16.35,21.35):place('Wire18m',(x,y,0))
for x in (-104,-40,32,104):
    place('Cabinet',(x,33,0));place('YardLamp',(x,32,0))
for x in range(-120,120,6):place('CableDuct6m',(x,32.0,-.2))
place('SignalUnbound',(115,-2.7,0),angle=-90);place('SignalUnbound',(-115,24.1,0),angle=90)
for x in range(-144,144,24):
    for y in range(-48,49,24):place('GroundTile24m',(x,y,0))
for x in range(-132,132,12):place('RoadTile12m',(x,-28,0))
for x in range(-120,120,6):place('Boundary6m',(x,34,0))
for x in range(-120,121,12):place('TreeBroadleaf',(x,38+(x%5),0),angle=x%90,scale=(1,1,1+(x%3)*.1))
for i,x in enumerate(range(-110,111,22)):place('UrbanA' if i%2 else 'UrbanB',(x,48,0),angle=180)

# Repair inherited geometry and reduce repeated detail before source review.
for key,o in mods.items():
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001)
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.0000001)
    bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(o.data);bm.free();o.data.update()
    ratio={'EgmoreArcade6m':.35,'EgmoreCanopy6m':.36,'EgmoreDrain3m':.12,'EgmoreTower4m':.55}.get(key)
    if ratio:
        o.data=o.data.copy()
        dec=o.modifiers.new('MobileDetailReduction','DECIMATE');dec.ratio=ratio;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=dec.name)
        # Propagate reduced source to previously created linked instances.
        for p in placements:
            if p['module']==key:bpy.data.objects[p['stableId']].data=o.data
library.hide_render=True
S.world=bpy.data.worlds.new('Daylight');S.world.use_nodes=True
bg=S.world.node_tree.nodes.get('Background');bg.inputs[0].default_value=(.52,.65,.8,1);bg.inputs[1].default_value=.7
bpy.ops.object.light_add(type='SUN',location=(0,-50,70));sun=bpy.context.object;sun.name='DaySun';sun.data.energy=2.8;sun.data.angle=.12;sun.rotation_euler=(.45,-.45,-.3)
S.render.engine='CYCLES';S.cycles.samples=24;S.cycles.use_denoising=True
S.render.resolution_x=1440;S.render.resolution_y=900;S.render.resolution_percentage=100
S.view_settings.view_transform='AgX'
S.view_settings.exposure=.4
cameras=[('HeritageStreet',(38,-70,10),(0,-13,6),38),('PlatformEye',(-50,8.4,2.5),(30,8.8,3.1),32),('TrackArrival',(-113,0,3.1),(14,6,4.2),45),('BridgePlatform',(37,14,2.5),(66,11,7),34),('Layout',(150,-125,135),(0,6,0),45)]
for name,pos,target,lens in cameras:
    bpy.ops.object.camera_add(location=pos);c=bpy.context.object;c.name=name;c.data.lens=lens;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();S.camera=c
    S.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
S.camera=bpy.data.objects['PlatformEye']
# Store mesh budget from evaluated geometry, plus actual bounds, before exports.
def bounds(o):
    vs=[o.matrix_world@Vector(c) for c in o.bound_box]
    return [[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]]
def tris(o):o.data.calc_loop_triangles();return len(o.data.loop_triangles)
asset_records=[]
library.hide_render=False
bpy.ops.object.select_all(action='DESELECT')
for key,o in mods.items():
    o['asset_id']='Station.Egmore.Scene.02.'+key
    record={'module':key,'assetId':o['asset_id'],'sourceAssetId':('Station.Kit.'+key if key.startswith('Egmore') else None),'boundsM':bounds(o),'lods':[]}
    for level,ratio in [(0,1),(1,.5),(2,.2)]:
        candidate=o.copy();candidate.data=o.data.copy();S.collection.objects.link(candidate);candidate.hide_set(False);candidate.hide_render=False
        if level and tris(candidate)>150:
            dec=candidate.modifiers.new('LOD reduction','DECIMATE');dec.ratio=ratio;bpy.context.view_layer.objects.active=candidate;bpy.ops.object.modifier_apply(modifier=dec.name)
        bpy.ops.object.select_all(action='DESELECT');candidate.select_set(True);bpy.context.view_layer.objects.active=candidate
        fname=f'{key}_LOD{level}'
        bpy.ops.export_scene.gltf(filepath=str(OUT/'Modules'/(fname+'.glb')),export_format='GLB',use_selection=True,export_cameras=False,export_lights=False,export_tangents=True)
        bpy.ops.export_scene.fbx(filepath=str(OUT/'Modules'/(fname+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_scale_options='FBX_SCALE_UNITS',use_triangles=True,path_mode='RELATIVE')
        record['lods'].append({'level':level,'triangles':tris(candidate),'glb':'Modules/'+fname+'.glb','fbx':'Modules/'+fname+'.fbx'})
        bpy.data.objects.remove(candidate,do_unlink=True)
    asset_records.append(record)
library.hide_render=True;library.hide_viewport=True
for im in bpy.data.images:
    if im.source=='FILE':im.filepath='//Textures/'+Path(im.filepath).name
S.render.filepath='//Previews/PlatformEye.png'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'EgmoreScene.blend'))
# whole-scene GLB uses shared geometry for repeated modules.
bpy.ops.object.select_all(action='DESELECT')
for o in world.objects:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'EgmoreScene.glb'),export_format='GLB',use_selection=True,export_cameras=False,export_lights=False,export_tangents=True)
allbounds=[bounds(o) for o in world.objects]
manifest={'schema':'StationArt/2','version':'2.0.0','status':'candidate-under-review','assetFamilyId':'Station.Egmore.Scene.02','units':'metres','basis':'Blender right-handed X longitudinal Y lateral Z up; GLB standard Y-up; FBX -Y forward Z-up; verify importer basis','estimatedDimensions':True,'era':'representative heritage-era composition informed by 2009-2022 references; not a 2026 survey','boundsM':[[min(b[0][i] for b in allbounds) for i in range(3)],[max(b[1][i] for b in allbounds) for i in range(3)]],'modules':asset_records,'placements':placements,'geometryContract':{'gaugeInsideHeadM':1.676,'railHeadWidthM':.07,'railCentresOffsetM':.873,'railTopM':0,'platformTopM':.84,'trackCentresY':[0,5,16.35,21.35],'platformYIntervals':[[-9.675,-1.675],[6.675,14.675],[23.025,31.025]],'bridgeDeckTopM':7.56,'bridgeStairRisers':42,'bridgeStairRiseM':.16,'bridgeStairTreadM':.29,'bridgeStairStartX':52.02,'bridgeStairEndX':64.2,'bridgeDeckStartX':64.2,'contactWireHeightM':5.5},'sourceReceipts':source_receipts,'expandedLODTriangles':{str(l):sum(next(a for a in asset_records if a['module']==p['module'])['lods'][l]['triangles'] for p in placements) for l in (0,1,2)}}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('V2_BUILD_COMPLETE',manifest['expandedLODTriangles'])
