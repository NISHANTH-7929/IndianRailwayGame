import bpy, json, math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]; OUT=ROOT/'v1.0.0'; EXPORT=OUT/'exports'; PREVIEW=OUT/'previews'
scene=bpy.context.scene
scene.view_settings.look='AgX - Medium High Contrast'; scene.view_settings.exposure=1.25
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(0.22,0.29,0.40,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.45
for c in [o for o in scene.objects if o.type=='CAMERA']:
    scene.camera=c; scene.render.filepath=str(PREVIEW/(c.name+'.png')); bpy.ops.render.render(write_still=True)
for o in scene.objects: o.select_set(o.type=='MESH' and not o.name.startswith('LOD'))
bpy.context.view_layer.objects.active=next(o for o in bpy.context.selected_objects if o.type=='MESH')
bpy.ops.export_scene.fbx(filepath=str(EXPORT/'ChennaiEgmore_StationArt_v1.0.0.fbx'),use_selection=True,apply_scale_options='FBX_SCALE_UNITS',mesh_smooth_type='FACE',add_leaf_bones=False)

def stat(obs): return {"objects":len(obs),"sourceTriangles":sum(len(p.vertices)-2 for o in obs for p in o.data.polygons)}
lod0=[o for o in scene.objects if o.type=='MESH' and not o.name.startswith('LOD')]
manifest={"schema":"IndianRailwayGame.StationArtPlacement/1","assetFamilyId":"Station.Egmore.Scene.01","version":"1.0.0","status":"ready-source-preview","units":"metres","axis":{"longitudinal":"+X","lateral":"+Y","up":"+Z","railTopZ":0},"siteBoundsM":{"min":[-130,-36,-1.15],"max":[130,36,21]},"estimatedDimensions":True,"surveyAccuracyClaim":False,"trackGaugeM":1.676,"placements":[{"stableId":o.name,"collection":o.users_collection[0].name,"locationM":[round(v,4) for v in o.location],"rotationDeg":[round(math.degrees(v),3) for v in o.rotation_euler],"dimensionsM":[round(v,4) for v in o.dimensions]} for o in lod0],"lodPolicy":[{"level":0,"use":"0-90m","stats":stat(lod0)},{"level":1,"use":"90-180m","stats":stat([o for o in scene.objects if o.name.startswith('LOD1')])},{"level":2,"use":"180m+","stats":stat([o for o in scene.objects if o.name.startswith('LOD2')])}],"playerCameras":[{"id":c.name,"locationM":[round(v,3) for v in c.location],"lensMm":c.data.lens} for c in scene.objects if c.type=='CAMERA'],"integrationAnchors":{"trackCentresY_M":[-7.5,-3.0,3.0,7.5],"platformCentresY_M":[-5.25,5.25,11.0],"platformTopZ_M":0.84,"contactWireZ_M":5.48,"suggestedTrainTrack":"centre y=-3.0; longitudinal origin x=0"},"notes":["Architecture and site dimensions are visual estimates from registered public photographs, not survey data.","WAP-7/LHB train placement is intentionally left to the integration chat.","No Unreal or Android validation is claimed."]}
(OUT/'placement_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ChennaiEgmore_StationArt_v1.0.0.blend'))
print(json.dumps({"lod0":stat(lod0),"manifest":str(OUT/'placement_manifest.json')}))
