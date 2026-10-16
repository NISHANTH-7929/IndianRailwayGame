import bpy, math, json, hashlib, os
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"v1.0.0"
TEX=OUT/"textures"
EXPORT=OUT/"exports"; PREVIEW=OUT/"previews"
EXPORT.mkdir(parents=True,exist_ok=True); PREVIEW.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

def coll(name):
    c=bpy.data.collections.new(name); bpy.context.scene.collection.children.link(c); return c
COL={n:coll(n) for n in ["Architecture","Platforms","Canopies","Footbridge","Track","Infrastructure","Context","LOD1","LOD2"]}

def mat(name, tex_name, scale=4.0, metallic=0.0):
    m=bpy.data.materials.new(name); m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=.7; bs.inputs['Metallic'].default_value=metallic
    nodes=m.node_tree.nodes; links=m.node_tree.links
    uv=nodes.new('ShaderNodeTexCoord'); mp=nodes.new('ShaderNodeMapping'); links.new(uv.outputs['Generated'],mp.inputs['Vector'])
    mp.inputs['Scale'].default_value=(scale,scale,scale)
    bc=nodes.new('ShaderNodeTexImage'); bc.image=bpy.data.images.load(str(TEX/f"T_{tex_name}_BC.png"),check_existing=True); links.new(mp.outputs['Vector'],bc.inputs['Vector']); links.new(bc.outputs['Color'],bs.inputs['Base Color'])
    orm=nodes.new('ShaderNodeTexImage'); orm.image=bpy.data.images.load(str(TEX/f"T_{tex_name}_ORM.png"),check_existing=True); orm.image.colorspace_settings.name='Non-Color'; links.new(mp.outputs['Vector'],orm.inputs['Vector'])
    sep=nodes.new('ShaderNodeSeparateColor'); links.new(orm.outputs['Color'],sep.inputs['Color']); links.new(sep.outputs['Green'],bs.inputs['Roughness']); links.new(sep.outputs['Blue'],bs.inputs['Metallic'])
    nm=nodes.new('ShaderNodeTexImage'); nm.image=bpy.data.images.load(str(TEX/f"T_{tex_name}_N.png"),check_existing=True); nm.image.colorspace_settings.name='Non-Color'; links.new(mp.outputs['Vector'],nm.inputs['Vector']); nn=nodes.new('ShaderNodeNormalMap'); nn.inputs['Strength'].default_value=.35; links.new(nm.outputs['Color'],nn.inputs['Color']); links.new(nn.outputs['Normal'],bs.inputs['Normal'])
    return m

M={
 'red':mat('M_Egmore_RedStucco','red_stucco',5), 'cream':mat('M_Egmore_CreamLime','cream_lime',5),
 'tile':mat('M_Egmore_PlatformTile','platform_tile',7), 'concrete':mat('M_Egmore_Concrete','concrete',5),
 'blue':mat('M_Egmore_BlueMetal','blue_metal',8,.5), 'steel':mat('M_Egmore_DarkSteel','dark_steel',6,.65),
 'ballast':mat('M_Egmore_Ballast','ballast',10), 'soil':mat('M_Egmore_Soil','soil',8),
 'foliage':mat('M_Egmore_Foliage','foliage',5), 'yellow':mat('M_Egmore_YellowEdge','yellow_edge',5)
}

def move_obj(o,c):
    for cc in list(o.users_collection): cc.objects.unlink(o)
    c.objects.link(o)

def box(name,loc,dim,ma,collection,bevel=.04):
    bpy.ops.mesh.primitive_cube_add(location=loc); o=bpy.context.object; o.name=name; o.dimensions=dim; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('EdgeSoftness','BEVEL'); mod.width=bevel; mod.segments=2
    o.data.materials.append(ma); move_obj(o,COL[collection]); return o

def cyl(name,loc,r,depth,ma,collection,verts=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc); o=bpy.context.object; o.name=name; o.data.materials.append(ma); move_obj(o,COL[collection]); return o

def arch_frame(name,x,y,z,width,height,depth):
    box(name+'_L',(x-width/2+.22,y,z+height/2),(.44,depth,height),M['cream'],'Architecture')
    box(name+'_R',(x+width/2-.22,y,z+height/2),(.44,depth,height),M['cream'],'Architecture')
    bpy.ops.mesh.primitive_torus_add(major_radius=width/2-.22,minor_radius=.22,major_segments=20,minor_segments=6,location=(x,y,z+height))
    o=bpy.context.object; o.name=name+'_Arch'; o.scale=(1,depth/.44,1); bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); o.data.materials.append(M['cream']); move_obj(o,COL['Architecture'])
    box(name+'_Void',(x,y-.03,z+height*.46),(width-.65,depth+.08,height*.86),M['steel'],'Architecture',.02)

# Overall bounded site: longitudinal X, lateral Y, vertical Z; rail top z=0.
box('CTX_Ground',(0,0,-.65),(260,72,1.0),M['soil'],'Context',0)
for yi in (-7.5,-3.0,3.0,7.5):
    box(f'TRK_Ballast_{yi}',(0,yi,-.33),(240,3.35,.45),M['ballast'],'Track',0)
    for ry in (yi-.838,yi+.838):
        box(f'TRK_Rail_{yi}_{ry}',(0,ry,.04),(240,.075,.15),M['steel'],'Track',.015)
    for x in range(-118,119,2): box(f'TRK_Sleeper_{yi}_{x}',(x,yi,-.12),(1.05,2.7,.13),M['concrete'],'Track',.01)

def platform(name,y,width):
    box(name,(0,y,.36),(220,width,.72),M['tile'],'Platforms',.06)
    for edge in (y-width/2,y+width/2):
        box(name+'_Coping_'+str(edge),(0,edge,.78),(220,.38,.12),M['cream'],'Platforms',.02)
        box(name+'_Yellow_'+str(edge),(0,edge+(0.28 if edge<y else -0.28),.85),(220,.14,.025),M['yellow'],'Platforms',.005)
platform('PLAT_Island_A',-5.25,2.2); platform('PLAT_Island_B',5.25,2.2)
platform('PLAT_Heritage',11.0,4.6)

# Heritage frontage: a coherent 126 m composition set behind platform 4.
box('ARCH_MainMass',(0,17.2,5.0),(126,8.0,10.0),M['red'],'Architecture',.12)
box('ARCH_Cornice',(0,13.1,9.4),(128,.55,.55),M['cream'],'Architecture',.08)
box('ARCH_PlatformPlinth',(0,13.05,1.0),(128,.65,1.1),M['cream'],'Architecture',.04)
bay_x=[x for x in range(-57,58,6) if abs(x)>11]
for i,x in enumerate(bay_x): arch_frame(f'ARCH_Arcade_{i:02}',x,12.72,1.15,3.5,4.2,.55)
# central entrance and towers
box('ARCH_EntranceMass',(0,13.9,6.0),(19,6.5,12),M['red'],'Architecture',.12)
for x in (-7,0,7): arch_frame('ARCH_EntranceArch_'+str(x),x,10.58,1.3,4.6,5.3,.7)
box('ARCH_EntranceCornice',(0,10.45,10.3),(20,.8,.7),M['cream'],'Architecture',.08)
for x in (-10.5,10.5):
    box('ARCH_Tower_'+str(x),(x,15.2,8.1),(5.5,6.2,16.2),M['red'],'Architecture',.12)
    box('ARCH_TowerTrim_'+str(x),(x,12.03,9.0),(6.1,.45,.6),M['cream'],'Architecture',.05)
    for z in (3.1,7.0,11.0): arch_frame(f'ARCH_TowerOpening_{x}_{z}',x,11.78,z,2.2,2.0,.45)
    cyl('ARCH_DomeDrum_'+str(x),(x,15.2,16.7),2.15,1.1,M['cream'],'Architecture',20)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,location=(x,15.2,18.1),scale=(2.5,2.5,1.8)); o=bpy.context.object; o.name='ARCH_Dome_'+str(x); o.data.materials.append(M['cream']); move_obj(o,COL['Architecture'])
    cyl('ARCH_Finial_'+str(x),(x,15.2,20.0),.12,1.8,M['steel'],'Architecture',10)

# Platform canopies with readable repeated structure and gutters.
for y,label in [(-5.25,'A'),(5.25,'B'),(11.0,'H')]:
    for x in range(-102,103,12):
        cyl(f'CAN_{label}_Column_{x}',(x,y,3.05),.11,4.45,M['steel'],'Canopies',10)
        box(f'CAN_{label}_Beam_{x}',(x,y,5.08),(.18,4.1,.22),M['steel'],'Canopies',.02)
    box(f'CAN_{label}_Roof',(0,y,5.35),(216,4.4,.18),M['blue'],'Canopies',.03)
    for sy in (-2.18,2.18): box(f'CAN_{label}_Gutter_{sy}',(0,y+sy,5.23),(216,.14,.18),M['steel'],'Canopies',.02)

# Footbridge at west end, high enough for OHE envelope.
box('FBR_Deck',(-68,2.8,8.2),(5.2,23.0,1.1),M['blue'],'Footbridge',.09)
box('FBR_Roof',(-68,2.8,11.25),(5.6,23.5,.22),M['blue'],'Footbridge',.05)
for y in (-8.4,14.0):
    box(f'FBR_Pier_{y}',(-68,y,4.4),(1.0,1.0,8.8),M['concrete'],'Footbridge',.05)
for y0,side in [(-8.0,'S'),(13.6,'N')]:
    # stepped visual mass, 16 risers
    direction=-1 if y0<0 else 1
    for i in range(16): box(f'FBR_Stair_{side}_{i:02}',(-68,y0+direction*i*.55,.32+i*.48),(3.2,.62,.64),M['concrete'],'Footbridge',.025)
    for dx in (-1.7,1.7): box(f'FBR_Rail_{side}_{dx}',(-68+dx,y0+direction*4.2,4.35),(0.08,9.3,1.0),M['steel'],'Footbridge',.015)

# OHE and presentation infrastructure.
for x in range(-108,109,18):
    for y in (-10.5,10.5): cyl(f'INF_OHEMast_{x}_{y}',(x,y,3.1),.13,6.2,M['steel'],'Infrastructure',10)
    box(f'INF_OHEPortal_{x}',(x,0,6.05),(.22,21.2,.22),M['steel'],'Infrastructure',.02)
    for y in (-7.5,-3,3,7.5):
        cyl(f'INF_Insulator_{x}_{y}',(x,y,5.72),.08,.55,M['cream'],'Infrastructure',10)
for y in (-7.5,-3,3,7.5): box(f'INF_ContactWire_{y}',(0,y,5.48),(226,.025,.025),M['steel'],'Infrastructure',.005)
for x in (-92,-28,38,96):
    box(f'INF_Cabinet_{x}',(x,15.1,1.05),(1.2,.75,1.8),M['concrete'],'Infrastructure',.06)

# Bounded urban edge and vegetation, intentionally low-detail behind the player space.
for i,(x,y,h) in enumerate([(-110,29,12),(-87,31,18),(-57,30,14),(-28,32,22),(10,31,15),(38,30,19),(68,32,13),(94,30,17),(116,31,11)]):
    box(f'CTX_Urban_{i}',(x,y,h/2),(18,7,h),M['concrete'],'Context',.08)
for i,x in enumerate(range(-116,117,13)):
    y=23+(i%3)*2.2; cyl(f'CTX_TreeTrunk_{i}',(x,y,2.2),.24,4.4,M['soil'],'Context',9)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=2.7+(i%2)*.5,location=(x,y,5.1)); o=bpy.context.object; o.name=f'CTX_TreeCrown_{i}'; o.data.materials.append(M['foliage']); move_obj(o,COL['Context'])
box('CTX_BoundaryWall',(0,25.5,1.0),(245,.35,2.0),M['red'],'Context',.05)

# Simple LOD proxy meshes retain stable component identity without clutter.
for lvl,scale in [(1,1.0),(2,1.0)]:
    c=f'LOD{lvl}'; box(f'LOD{lvl}_Heritage',(0,17.2,5.0),(126,8,10),M['red'],c,.05)
    for y in (-5.25,5.25,11): box(f'LOD{lvl}_Platform_{y}',(0,y,.36),(220,2.2 if y!=11 else 4.6,.72),M['tile'],c,.02)
    box(f'LOD{lvl}_Footbridge',(-68,2.8,8.2),(5.2,23,1.1),M['blue'],c,.03)
    COL[c].hide_render=True; COL[c].hide_viewport=True

scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
scene.render.engine='BLENDER_EEVEE'; scene.render.resolution_x=1280; scene.render.resolution_y=720; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.film_transparent=False
scene.world=bpy.data.worlds.new('EgmoreWorld'); scene.world.color=(.035,.055,.08)
bpy.ops.object.light_add(type='SUN',location=(20,-30,45)); sun=bpy.context.object; sun.name='Sun'; sun.rotation_euler=(math.radians(28),math.radians(-18),math.radians(132)); sun.data.energy=2.4; sun.data.angle=math.radians(4)
bpy.ops.object.light_add(type='AREA',location=(0,-8,22)); bpy.context.object.data.energy=1700; bpy.context.object.data.shape='RECTANGLE'; bpy.context.object.data.size=80

def camera(name,loc,target,lens):
    bpy.ops.object.camera_add(location=loc); c=bpy.context.object; c.name=name; c.data.lens=lens; c.data.sensor_width=36
    c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler(); return c
cams=[camera('CAM_PlatformEye',(-24,-1.0,2.0),(7,11,4.8),33),camera('CAM_TrackArrival',(-112,-10,3.0),(-14,11,5.4),42),camera('CAM_HeritageWide',(48,-25,8.2),(0,14,6.6),44)]
for c in cams:
    scene.camera=c; scene.render.filepath=str(PREVIEW/(c.name+'.png')); bpy.ops.render.render(write_still=True)

# Save source first; exports carry only visible LOD0 collections.
blend=OUT/'ChennaiEgmore_StationArt_v1.0.0.blend'; bpy.ops.wm.save_as_mainfile(filepath=str(blend))
for o in bpy.context.scene.objects: o.select_set(o.type=='MESH' and not o.name.startswith('LOD'))
bpy.context.view_layer.objects.active=next(o for o in bpy.context.selected_objects if o.type=='MESH')
bpy.ops.export_scene.gltf(filepath=str(EXPORT/'ChennaiEgmore_StationArt_v1.0.0.glb'),export_format='GLB',use_selection=True,export_apply=True)
bpy.ops.export_scene.fbx(filepath=str(EXPORT/'ChennaiEgmore_StationArt_v1.0.0.fbx'),use_selection=True,apply_scale_options='FBX_SCALE_UNITS',mesh_smooth_type='FACE',add_leaf_bones=False)

def stats(prefix=None):
    obs=[o for o in scene.objects if o.type=='MESH' and (prefix is None or o.name.startswith(prefix))]
    tris=sum(len(p.vertices)-2 for o in obs for p in o.data.polygons)
    return {"objects":len(obs),"sourceTriangles":tris}
manifest={
 "schema":"IndianRailwayGame.StationArtPlacement/1","assetFamilyId":"Station.Egmore.Scene.01","version":"1.0.0","status":"ready-source-preview",
 "units":"metres","axis":{"longitudinal":"+X","lateral":"+Y","up":"+Z","railTopZ":0},
 "siteBoundsM":{"min":[-130,-36,-1.15],"max":[130,36,21]},
 "estimatedDimensions":True,"surveyAccuracyClaim":False,"trackGaugeM":1.676,
 "placements":[{"stableId":o.name,"collection":o.users_collection[0].name,"locationM":[round(v,4) for v in o.location],"rotationDeg":[round(math.degrees(v),3) for v in o.rotation_euler],"dimensionsM":[round(v,4) for v in o.dimensions]} for o in scene.objects if o.type=='MESH' and not o.name.startswith('LOD')],
 "lodPolicy":[{"level":0,"use":"0-90m","stats":stats()},{"level":1,"use":"90-180m","stats":stats('LOD1')},{"level":2,"use":"180m+","stats":stats('LOD2')}],
 "playerCameras":[{"id":c.name,"locationM":[round(v,3) for v in c.location],"lensMm":c.data.lens} for c in cams],
 "notes":["Architecture and site dimensions are visual estimates from registered public photographs, not survey data.","WAP-7/LHB train placement is intentionally left to the integration chat.","No Unreal or Android validation is claimed."]
}
(OUT/'placement_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({"blend":str(blend),"manifest":str(OUT/'placement_manifest.json'),"stats":stats()}))
