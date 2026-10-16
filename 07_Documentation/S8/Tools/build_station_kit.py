"""Independently authored modular railway kit. Run with Blender --background.
Public previews are comparison-only: this script reads no marketplace images.
All dimensions are metres; unsurveyed architectural dimensions are estimates.
"""
import bpy, json, math, random
import numpy as np
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'04_Assets/S8/StationKit'
for d in ['Parts','Textures']:(OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rng=np.random.default_rng(831)
materials={};definitions=[];parts=[]

def surface(label,color,rough=.75,metal=0,kind='paint'):
    n=1024 if kind=='aggregate' else 512;y,x=np.mgrid[0:n,0:n]/n
    grain=rng.normal(0,.025,(n,n))
    # Periodic, smoothly interpolated random fields avoid directional sine bands.
    broad=np.zeros((n,n))
    for cells,amplitude in [(5,.022),(13,.012),(37,.006)]:
        grid=rng.normal(0,1,(cells,cells))
        u=x*cells;v=y*cells;ix=u.astype(int);iy=v.astype(int)
        fx=u-ix;fy=v-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
        broad+=amplitude*((1-fy)*((1-fx)*grid[iy,ix]+fx*grid[iy,(ix+1)%cells])+fy*((1-fx)*grid[(iy+1)%cells,ix]+fx*grid[(iy+1)%cells,(ix+1)%cells]))
    height=grain*.15+broad*.2
    shade=1+grain+broad
    if kind=='aggregate':
        # Periodic cellular stone relief: the 2 m cube-projected tile contains
        # roughly 3 cm fragments. Unlike colour noise, joints affect normals.
        stone_rng=np.random.default_rng(17831)
        cells=64;seeds=stone_rng.uniform(.15,.85,(cells,cells,2))
        tones=stone_rng.uniform(.48,1.35,(cells,cells))
        u=x*cells;v=y*cells;ix=u.astype(int);iy=v.astype(int)
        first=np.full_like(x,1.e9);second=first.copy();tone=np.ones_like(x)
        for oy in [-1,0,1]:
            for ox in [-1,0,1]:
                sx=(ix+ox)%cells;sy=(iy+oy)%cells
                delta_x=u-(ix+ox+seeds[sy,sx,0])
                delta_y=v-(iy+oy+seeds[sy,sx,1])
                distance=delta_x*delta_x+delta_y*delta_y
                closer=distance<first
                second=np.where(closer,first,np.minimum(second,distance))
                tone=np.where(closer,tones[sy,sx],tone)
                first=np.minimum(first,distance)
        joint=np.clip((np.sqrt(second)-np.sqrt(first))/.12,0,1)
        shade=(.30+.70*joint)*tone+grain*.8+broad
        height=joint*.9+grain*.35
    if kind=='tile':
        joint=(np.minimum(x% .25,.25-x% .25)<.004)|(np.minimum(y% .25,.25-y% .25)<.004)
        shade=np.where(joint,.48,shade);height=np.where(joint,-.12,height)
    if kind=='metal':
        shade+=.05*np.sin(x*math.tau*48);height+=.012*np.sin(x*math.tau*48)
    chips=(grain+broad<-.085)
    base=np.clip(np.array(color)[None,None,:]*shade[:,:,None],0,1)
    if kind in ('paint','metal'):base[chips]=base[chips]*.7+np.array([.10,.055,.025])
    dy,dx=np.gradient(height);normal=np.stack([-dx*5,-dy*5,np.ones_like(dx)],axis=2)
    normal/=np.linalg.norm(normal,axis=2)[:,:,None];normal=normal*.5+.5
    orm=np.stack([np.clip(shade,.5,1),np.clip(rough+grain+broad,.15,.98),np.full_like(x,metal)],axis=2)
    files={}
    for suffix,data in [('BC',base),('N',normal),('ORM',orm)]:
        image=bpy.data.images.new('T_'+label+'_'+suffix,n,n,alpha=True)
        if suffix!='BC':image.colorspace_settings.name='Non-Color'
        image.pixels.foreach_set(np.concatenate([data,np.ones((n,n,1))],axis=2).astype(np.float32).ravel())
        image.filepath_raw=str(OUT/'Textures'/(image.name+'.png'));image.file_format='PNG';image.save()
        files[suffix]='Textures/'+image.name+'.png'
    mat=bpy.data.materials.new(label);mat.use_nodes=True
    nodes=mat.node_tree.nodes;links=mat.node_tree.links;bs=nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=metal
    for suffix in files:
        t=nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(OUT/files[suffix]),check_existing=True)
        if suffix=='BC':links.new(t.outputs['Color'],bs.inputs['Base Color'])
        elif suffix=='N':
            normalnode=nodes.new('ShaderNodeNormalMap');links.new(t.outputs['Color'],normalnode.inputs['Color']);links.new(normalnode.outputs['Normal'],bs.inputs['Normal'])
        else:
            separate=nodes.new('ShaderNodeSeparateColor');links.new(t.outputs['Color'],separate.inputs['Color'])
            links.new(separate.outputs['Green'],bs.inputs['Roughness']);links.new(separate.outputs['Blue'],bs.inputs['Metallic'])
    materials[label]=mat;definitions.append({'name':label,'base_color':[*color,1],'roughness':rough,'metallic':metal,'textures':files})

surface('Paving',[.55,.49,.40],.87,kind='tile')
surface('Concrete',[.48,.46,.41],.9,kind='stone')
surface('Cream',[.74,.69,.55],.81)
surface('RedOxide',[.35,.075,.042],.78)
surface('Steel',[.075,.085,.085],.47,.75,'metal')
surface('Roof',[.22,.30,.35],.53,.55,'metal')
surface('Yellow',[.82,.59,.075],.63)
surface('Black',[.018,.02,.018],.68)
surface('Galvanized',[.38,.40,.40],.46,.8,'metal')
surface('Insulator',[.19,.072,.031],.3)
surface('Wood',[.25,.115,.052],.81)
surface('Glass',[.07,.14,.16],.18,.35)
surface('Ballast',[.24,.22,.19],.94,kind='stone')
surface('Soil',[.25,.18,.10],.97,kind='stone')

objects=[]
def finish_object(o,mat):
    o.data.materials.append(materials[mat]);objects.append(o);return o
def box(pos,size,mat,bevel=.012):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('manufactured edge','BEVEL');mod.width=bevel;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return finish_object(o,mat)
def beam(a,b,width,mat,depth=None):
    v=Vector(b)-Vector(a);o=box((Vector(a)+Vector(b))/2,(width,depth or width,v.length),mat,.005)
    o.rotation_euler=v.to_track_quat('Z','Y').to_euler();return o
def cylinder(pos,radius,depth,mat,vertices=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=pos)
    return finish_object(bpy.context.object,mat)
def text(body,pos,size,mat='Black'):
    bpy.ops.object.text_add(location=pos,rotation=(math.pi/2,0,0));o=bpy.context.object
    o.data.body=body;o.data.align_x='CENTER';o.data.size=size;o.data.extrude=.001
    bpy.ops.object.convert(target='MESH');o=bpy.context.object;o['readable_text']=True;return finish_object(o,mat)
def export(label):
    global objects
    text_planes=[]
    for item in objects:
        if item.get('readable_text'):
            # Preserve readable signs across Blender/UE handedness, locally.
            group=item.vertex_groups.new(name='ReadableText_'+str(len(text_planes)))
            group.add(list(range(len(item.data.vertices))),1.0,'REPLACE')
            text_planes.append((group.name,item.matrix_world.translation.copy(),
                                (item.matrix_world.to_3x3()@Vector((1,0,0))).normalized()))
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object
    o.name='SM_Station_'+label;scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.cube_project(cube_size=2);bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles()
    filename='Parts/'+o.name+'.fbx'
    # UE's force-front-X FBX conversion reflects Y. Compensate only the exported
    # data so authored metric coordinates and runtime placement share one basis.
    original_mesh=o.data;o.data=original_mesh.copy()
    import bmesh
    glyph_vertices=set()
    for name,center,axis in text_planes:
        group_index=o.vertex_groups[name].index
        for vertex in o.data.vertices:
            if any(g.group==group_index for g in vertex.groups):
                vertex.co-=2*(vertex.co-center).dot(axis)*axis
                glyph_vertices.add(vertex.index)
    if glyph_vertices:
        bm=bmesh.new();bm.from_mesh(o.data);bm.verts.ensure_lookup_table()
        bmesh.ops.reverse_faces(bm,faces=[f for f in bm.faces if all(v.index in glyph_vertices for v in f.verts)])
        bm.to_mesh(o.data);bm.free()
    reflection=Matrix.Diagonal((1,-1,1,1))
    o.data.transform(reflection);o.data.flip_normals();o.data.update()
    bpy.ops.export_scene.fbx(filepath=str(OUT/filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',use_triangles=True,path_mode='STRIP')
    exported_mesh=o.data;o.data=original_mesh;bpy.data.meshes.remove(exported_mesh)
    parts.append({'name':label,'asset_id':'Station.Kit.'+label,'role':'scenery','fbx':filename,'location_cm':[0,0,0],'rotation_deg':[0,0,0],'scale':[1,1,1],
      'triangles':len(o.data.loop_triangles),'dimensions_m':list(o.dimensions),'bounds_m':[[min(v.co[i] for v in o.data.vertices) for i in range(3)],[max(v.co[i] for v in o.data.vertices) for i in range(3)]],
      'dimensionalEvidence':'estimated architectural dimensions; metric authored, not surveyed'})
    o.hide_render=True;o.hide_set(True);objects=[]

box((2.5,2.5,.37),(5,5,.74),'Concrete');box((2.5,2.5,.79),(5,5,.10),'Paving')
box((2.5,4.80,.81),(5,.4,.06),'Cream');box((2.5,4.5,.84),(5,.08,.004),'Yellow',0)
export('Platform5m')
box((2.5,2.5,.37),(5,5,.74),'Concrete');box((2.5,2.5,.79),(5,5,.10),'Paving');export('Apron5m')
for y in [-1.5,1.5]:
    box((0,y,.35),(.75,.75,.7),'Concrete');box((0,y,3.7),(.32,.32,7.4),'Galvanized')
beam((0,-1.5,7.4),(0,1.5,7.4),.28,'Steel');export('BridgePier')
beam((0,0,0),(3,0,0),.012,'Steel');export('ContactWire3m')
for x in [.2,5.8]:
    box((x,2.5,.15),(.8,.8,.3),'RedOxide');box((x,2.5,.33),(.8,.8,.06),'Cream')
    for yy in [2.36,2.64]:box((x,yy,2.15),(.16,.09,3.6),'Steel')
    for z in [.55,1.7,3.4]:
        box((x,2.5,z),(.23,.42,.23),'Steel')
        for y in [2.32,2.68]:cylinder((x,y,z+.13),.025,.03,'Galvanized',8)
    for y in [0,5]:beam((x,2.5,3.3),(x,y,3.95),.09,'Steel')
    for y in [0,1,2,3,4]:
        beam((x,y,3.95),(x,y+1,4.35-.16*abs(y+1-2.5)),.045,'Steel')
    beam((x,0,3.95),(x,2.5,4.35),.12,'Steel');beam((x,2.5,4.35),(x,5,3.95),.12,'Steel')
for y in [0,1.25,2.5,3.75,5]:box((3,y,4.0+.35*(1-abs(y-2.5)/2.5)),(6,.09,.12),'Steel')
for i in range(41):
    x=i*.15
    for side in [-1,1]:
        o=box((x,2.5+side*1.35,4.16),(.151,2.78,.025),'Roof',0);o.rotation_euler.x=-side*.15
        beam((x,2.5,4.4),(x,2.5+side*2.74,3.99),.028,'Roof')
export('Canopy6m')
for x in [-.68,.68]:
    box((x,0,.26),(.22,.48,.52),'Concrete');beam((x,.16,.4),(x,.3,1.03),.08,'Steel')
for y in [-.16,0,.16]:box((0,y,.52),(1.9,.145,.07),'RedOxide')
for z in [.76,.94]:box((0,.28,z),(1.9,.065,.14),'RedOxide')
export('Bench')
for x in [0,1.5,3]:cylinder((x,0,.55),.035,1.1,'Steel')
for z in [.15,.55,1.07]:beam((0,0,z),(3,0,z),.04,'Steel')
export('Railing3m')
# A closed stair section with a sloping 220 mm soffit, rather than stacked
# full-height blocks. The flight stays 6.38 m long and rises exactly 3.52 m.
profile=[(0,-.22),(6.38,3.30),(6.38,3.52)]
for i in reversed(range(22)):
    profile.extend([(i*.29,(i+1)*.16),(i*.29,i*.16)])
verts=[(x,y,z) for y in [-1.2,1.2] for x,z in profile];count=len(profile)
faces=[tuple(reversed(range(count))),tuple(range(count,2*count))]
faces.extend((i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count))
mesh=bpy.data.meshes.new('inclined concrete stair');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('inclined concrete stair',mesh);scene.collection.objects.link(o);finish_object(o,'Concrete')
for y in [-1.24,1.24]:
    beam((0,y,.9),(6.38,y,4.42),.05,'Galvanized')
    beam((0,y,.45),(6.38,y,3.97),.035,'Galvanized')
    for i in range(0,23,2):beam((i*.29,y,i*.16),(i*.29,y,i*.16+.95),.045,'Galvanized')
export('StairFlight')
box((3,0,0),(6,2.7,.2),'Concrete')
for y in [-1.4,1.4]:
    for x in [0,1.5,3,4.5,6]:beam((x,y,0),(x,y,2.55),.09,'Galvanized')
    for z in [.5,1.05,2.5]:beam((0,y,z),(6,y,z),.055,'Galvanized')
    for i in range(4):beam((i*1.5,y,0),((i+1)*1.5,y,1),.05,'Galvanized')
box((3,0,2.6),(6,3.0,.06),'Roof');export('Footbridge6m')
box((0,0,.12),(3.2,2.5,.24),'Concrete')
for x in [-1.5,1.5]:box((x,0,1.35),(.10,2.3,2.5),'Cream')
box((0,1.1,1.35),(3,.10,2.5),'Cream');box((0,0,2.65),(3.4,2.7,.12),'Roof')
box((0,-1.05,.57),(3,.15,1.05),'RedOxide');box((0,-1.1,1.14),(3.2,.65,.10),'Galvanized')
box((0,-1.13,2.2),(3,.08,.46),'Yellow');text('TEA  &  SNACKS',(0,-1.18,2.08),.22)
export('VendorKiosk')
for x in [-2,2]:box((x,0,1.4),(.10,.10,2.8),'Steel')
box((0,0,2.85),(4.5,2.6,.10),'Roof');box((0,.9,1.5),(4,.06,1.9),'Cream');export('WaitingShelter')
box((0,0,.2),(6,4,.4),'Concrete')
for x in [-3,3]:box((x,0,1.85),(.18,4,3.3),'Cream')
box((0,1.9,1.85),(6,.18,3.3),'Cream');box((0,0,3.65),(6.4,4.4,.25),'RedOxide')
for x in [-2,2]:
    box((x,-1.9,1.8),(1.7,.15,3.2),'Cream');box((x,-2,1.95),(1.1,.04,1.35),'Glass')
    for xx in [-.4,0,.4]:box((x+xx,-2.05,1.95),(.035,.035,1.4),'Steel')
box((0,-1.9,3),(2.2,.16,.8),'Cream');box((0,-2,3.25),(2.4,.05,.5),'Yellow')
text('SHANTIPUR',(0,-2.04,3.13),.22);export('StationFacade6m')
for x in [-1.2,1.2]:box((x,0,1.25),(.13,.13,2.5),'Black')
box((0,0,2.05),(2.8,.10,1.05),'Yellow');text('SHANTIPUR',(0,-.06,2.08),.27)
text('PLATFORM  1',(0,-.06,1.78),.17)
for wording,z,size in [('SHANTIPUR',2.08,.27),('PLATFORM  1',1.78,.17)]:
    o=text(wording,(0,.06,z),size);o.rotation_euler.z=math.pi
export('StationBoard')
cylinder((0,0,2.5),.055,5,'Galvanized');beam((0,0,5),(0,.8,5.15),.055,'Galvanized')
box((0,.85,5.12),(.28,.6,.08),'Cream');box((0,.85,5.06),(.2,.48,.018),'Yellow');export('LampPost')
box((0,0,.15),(.8,.65,.3),'Concrete');box((0,0,.9),(.65,.45,1.2),'Galvanized')
box((0,-.24,.91),(.56,.035,1.05),'Steel');box((0,-.268,1.13),(.32,.018,.20),'Yellow')
text('DANGER',(0,-.282,1.09),.065)
for z in [.58,.65,.72]:box((0,-.268,z),(.43,.02,.012),'Black',0)
box((.21,-.28,.91),(.035,.035,.13),'Galvanized');export('ElectricalCabinet')
for label,words in [('WhistleBoard','W / L'),('TerminationBoard','T / P'),('SpeedBoard','10'),('EMUBoard','EMU STOP')]:
    for i in range(12):box((0,0,(i+.5)*.18),(.065,.065,.18),'Black' if i%2 else 'Cream',0)
    if label=='SpeedBoard':
        mesh=bpy.data.meshes.new('triangular board');mesh.from_pydata([(-.4,0,1.7),(.4,0,1.7),(0,0,2.45)],[],[(0,1,2)])
        o=bpy.data.objects.new('triangular board',mesh);scene.collection.objects.link(o);finish_object(o,'Yellow')
        mod=o.modifiers.new('board thickness','SOLIDIFY');mod.thickness=.025
        bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
    elif label=='TerminationBoard':
        o=cylinder((0,0,2.0),.36,.035,'Yellow',32);o.rotation_euler.x=math.pi/2
    else:box((0,0,2.0),(.7,.045,.65),'Yellow')
    text(words,(0,-.028,1.91),.14)
    export(label)
box((0,0,.25),(.8,.8,.5),'Concrete')
for x in [-.16,.16]:beam((x,0,.5),(x,0,7.2),.075,'Steel')
for i in range(12):beam((-.16,0,.5+i*.55),(.16,0,1.05+i*.55),.038,'Steel')
beam((0,0,6.5),(0,3,5.9),.045,'Galvanized');beam((0,0,5.3),(0,3,5.9),.045,'Galvanized')
beam((0,2.8,5.9),(0,3.3,5.5),.025,'Galvanized')
for z in [5.4,6.35]:
    for i in range(6):cylinder((0,.12,z+i*.045),.065,.02,'Insulator')
export('OHEMast')
for x in [0,12]:
    box((x,0,.2),(.9,.9,.4),'Concrete')
    for y in [-.22,.22]:beam((x,y,.4),(x,y,7.7),.08,'Steel')
    for z in range(7):beam((x,-.22,z+.4),(x,.22,z+1.4),.045,'Steel')
for z in [7.15,7.7]:beam((0,0,z),(12,0,z),.08,'Steel')
for i in range(16):beam((i*.75,0,7.15),((i+1)*.75,0,7.7),.04,'Steel')
export('OHEPortal12m')
# Rail section is a visual module; authoritative path comes from the kernel.
for y in [-.8705,.8705]:
    box((1.5,y,.04),(3,.15,.08),'Steel',.004);box((1.5,y,.105),(3,.025,.10),'Steel',.002)
    box((1.5,y,.165),(3,.065,.035),'Galvanized',.008)
export('Rail3m')
box((0,0,-.08),(.25,2.7,.19),'Concrete',.025)
for y in [-.8705,.8705]:
    box((0,y,.028),(.21,.2,.035),'Steel',.003)
    for yy in [-.10,.10]:cylinder((0,y+yy,.055),.028,.025,'Steel',8)
export('Sleeper')
box((1.5,0,-.30),(3,3.4,.32),'Ballast',.13);export('Ballast3m')
box((0,0,-.55),(50,50,.15),'Soil',0);export('Ground50m')
box((1.5,0,.7),(3,.22,1.4),'Cream');box((1.5,0,1.43),(3,.32,.10),'RedOxide');export('Boundary3m')

assembly={'asset_id':'Station.Kit.01','version':1,'units':'metres','materials':definitions,'parts':parts,
 'source':'Independent scripted geometry and procedural PBR; public previews comparison only',
 'status':'candidate; visual, Unreal, LOD, collision and Android validation pending'}
(OUT/'assembly.json').write_text(json.dumps(assembly,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'StationKit.blend'))
# Contact sheet places original modules in an open grid; never changes FBX pivots.
for i,p in enumerate(parts):
    o=bpy.data.objects['SM_Station_'+p['name']];o.hide_render=False;o.hide_set(False);o.location=((i%5)*15,(i//5)*13,0)
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1800;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Review daylight');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.36,.42,.52,1)
bpy.ops.object.light_add(type='SUN',location=(0,0,20));bpy.context.object.data.energy=2.5;bpy.context.object.rotation_euler=(.5,-.4,-.5)
bpy.ops.object.camera_add(location=(78,-75,85));cam=bpy.context.object;cam.rotation_euler=(Vector((29,23,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=100;scene.camera=cam
scene.render.filepath=str(OUT/'contact-sheet.png');bpy.ops.render.render(write_still=True)
print('STATION_KIT_COMPLETE',len(parts),sum(p['triangles'] for p in parts))
