"""Interior-only additive supplement to immutable StationArt v2.0.0."""
import bpy,math,json,hashlib,shutil,bmesh
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parents[1];BASE=O/'v2.0.0';R=O/'v2.1.0-interior'
assert not (R/'READY.lock').exists()
for d in ('Modules','Previews','Evidence','Textures'):(R/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(BASE/'EgmoreScene.blend'))
S=bpy.context.scene
for im in bpy.data.images:
    if im.source=='FILE' and im.filepath and Path(im.filepath).suffix:
        src=BASE/'Textures'/Path(im.filepath).name;dst=R/'Textures'/src.name
        if not dst.exists():shutil.copy2(src,dst)
        im.filepath=str(dst)
lib=bpy.data.collections.new('INTERIOR_MODULES');S.collection.children.link(lib)
inst=bpy.data.collections.new('INTERIOR_PLACEMENTS');S.collection.children.link(inst)
M={};assets=[];placements=[];lights=[]
def mat(name,col,rough=.65,emit=0):
    m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*col,1)
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=rough
    p.inputs['Emission Color'].default_value=(*col,1);p.inputs['Emission Strength'].default_value=emit;M[name]=m
mat('SignBlue',(.018,.055,.12));mat('SignWhite',(.86,.86,.77));mat('ExitGreen',(.025,.16,.08));mat('Housing',(.25,.27,.28),.5);mat('LampLens',(.94,.88,.71),.35,2)
def box(loc,dim,ma):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=dim;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(M[ma]);return o
def text(body,loc,size,angle=0):
    bpy.ops.object.text_add(location=loc,rotation=(math.pi/2,0,math.radians(angle)));o=bpy.context.object;o.data.body=body;o.data.size=size;o.data.align_x='CENTER';o.data.align_y='CENTER';o.data.extrude=.0008;o.data.resolution_u=4
    bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.data.materials.append(M['SignWhite']);return o
def join(name,objects):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;S.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');o.name=name
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7);bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-8);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for p in o.data.polygons:
        axis=max(range(3),key=lambda i:abs(p.normal[i]));ij=[i for i in range(3) if i!=axis]
        for li in p.loop_indices:
            c=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(c[ij[0]],c[ij[1]])
    for col in list(o.users_collection):col.objects.unlink(o)
    lib.objects.link(o);return o
def board(name,top,bottom,colour='SignBlue'):
    objs=[box((0,0,0),(1.65,.10,.66),colour)]
    for x in (-.65,.65):objs.append(box((x,0,.65),(.018,.018,.65),'Housing'))
    for y,angle in [(-.056,0),(.056,180)]:
        objs.extend([text(top,(0,y,.13),.20,angle),text(bottom,(0,y,-.16),.25,angle)])
    return join(name,objs)
modules={}
for i in (1,2,3):modules[f'PlatformNumber{i}']=board(f'PlatformNumber{i}','PLATFORM',str(i))
modules['ExitFootbridge']=board('ExitFootbridge','EXIT','FOOTBRIDGE','ExitGreen')
modules['CanopyLight']=join('CanopyLight',[box((0,0,0),(1.25,.20,.09),'Housing'),box((0,0,-.053),(1.16,.15,.022),'LampLens')])
for key,src in modules.items():
    src.data.calc_loop_triangles();lods=[]
    # Text signs keep their readability at all three levels; lamp is already minimal.
    for level in (0,1,2):
        bpy.ops.object.select_all(action='DESELECT');src.select_set(True);bpy.context.view_layer.objects.active=src
        filename=f'{key}_LOD{level}'
        bpy.ops.export_scene.gltf(filepath=str(R/'Modules'/(filename+'.glb')),export_format='GLB',use_selection=True,export_tangents=True,export_cameras=False,export_lights=False)
        bpy.ops.export_scene.fbx(filepath=str(R/'Modules'/(filename+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_scale_options='FBX_SCALE_UNITS',use_triangles=True)
        lods.append({'level':level,'triangles':len(src.data.loop_triangles),'glb':'Modules/'+filename+'.glb','fbx':'Modules/'+filename+'.fbx','policy':'retain text silhouette; cull sign below readable screen size'})
    assets.append({'module':key,'assetId':'Station.Egmore.Interior.01.'+key,'lods':lods,'boundsM':[[min(v.co[i] for v in src.data.vertices) for i in range(3)],[max(v.co[i] for v in src.data.vertices) for i in range(3)]]})
def place(key,loc,angle=0):
    o=modules[key].copy();inst.objects.link(o);o.name=f'EGI.{key}.{len(placements):03}';o.location=loc;o.rotation_euler.z=math.radians(angle)
    placements.append({'stableId':o.name,'module':key,'positionM':list(loc),'rotationZDeg':angle,'scale':[1,1,1]});return o
for i,y in enumerate((-5.675,10.675,27.025),1):
    for x in (-78,-30,18,84):place('PlatformNumber'+str(i),(x,y,4.5),90)
    place('ExitFootbridge',(42,y,4.5),90)
    for x in range(-90,91,12):
        if 54<=x<78:continue
        o=place('CanopyLight',(x,y,5.03))
        lights.append({'stableId':o.name+'.light','positionM':[x,y,4.94],'direction':[0,0,-1],'colourLinear':[1,.88,.72],'previewWatts':150,'engineIntensity':'calibrate in engine; Blender watts are not Unreal lumens','sourceSizeM':[1.16,.15],'castShadowSuggested':False,'runtimeBinding':'night lighting preset; do not create one dynamic shadowed light per fixture on mobile'})
lib.hide_render=True;lib.hide_viewport=True
# Source camera views point down the platform and up the bridge approach.
for name,pos,target,lens in [('PlatformWayfinding',(-49,10.675,2.45),(18,10.675,3.6),39),('BridgeApproach',(44,10.675,2.45),(65,10.675,5.8),30),('WaitingArea',(-34,8.4,2.40),(-18,11.0,2.2),35)]:
    bpy.ops.object.camera_add(location=pos);c=bpy.context.object;c.name=name;c.data.lens=lens;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();S.camera=c
    S.render.filepath=str(R/'Previews'/(name+'.png'));S.cycles.samples=24;bpy.ops.render.render(write_still=True)
S.camera=bpy.data.objects['PlatformWayfinding']
for im in bpy.data.images:
    if im.source=='FILE' and im.filepath and Path(im.filepath).suffix:im.filepath='//Textures/'+Path(im.filepath).name
bpy.ops.wm.save_as_mainfile(filepath=str(R/'EgmoreInterior.blend'))
bpy.ops.object.select_all(action='DESELECT')
for o in inst.objects:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(R/'InteriorDelta.glb'),export_format='GLB',use_selection=True,export_tangents=True,export_cameras=False,export_lights=False)
manifest={'version':'2.1.0','assetFamilyId':'Station.Egmore.Interior.01','status':'candidate','baseVersion':'2.0.0','baseSceneSha256':hashlib.sha256((BASE/'EgmoreScene.glb').read_bytes()).hexdigest(),'baseSourceSha256':hashlib.sha256((BASE/'EgmoreScene.blend').read_bytes()).hexdigest(),'units':'metres','basis':'same as v2; Blender Z up, GLB standard Y up','modules':assets,'placements':placements,'baseUnchanged':True,'platformNumbers':'Representative scene labels 1/2/3, not a surveyed real Egmore platform assignment. Integrator must bind labels to scenario.','runtimeLightingValidated':False,'sourcePreviewOnly':True}
(R/'manifest.json').write_text(json.dumps(manifest,indent=2));(R/'lighting-bindings.json').write_text(json.dumps(lights,indent=2))
# Preview the same fixture layout at night; save separate source variation.
bpy.data.objects['DaySun'].data.energy=.03;bg=S.world.node_tree.nodes['Background'];bg.inputs[0].default_value=(.025,.04,.085,1);bg.inputs[1].default_value=.14
for spec in lights:
    bpy.ops.object.light_add(type='AREA',location=spec['positionM']);l=bpy.context.object;l.data.energy=spec['previewWatts'];l.data.shape='RECTANGLE';l.data.size=1.16;l.data.size_y=.15;l.data.color=spec['colourLinear']
S.view_settings.exposure=1;S.render.filepath=str(R/'Previews/PlatformWayfindingNight.png');bpy.ops.render.render(write_still=True)
print('INTERIOR_COMPLETE',len(assets),len(placements),len(lights))
