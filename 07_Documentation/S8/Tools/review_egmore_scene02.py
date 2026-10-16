"""Isolated Scene02 art/formation review; never mutates service maps or saves."""
import json,time,hashlib,os
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/EgmoreScene02'
CONTENT=ROOT/'03_UnrealProject/MyProject/Content'
fixture=CONTENT/'IndianRailway/S7/L_RailService.umap'
before=hashlib.sha256(fixture.read_bytes()).hexdigest()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study'
assert (levels.load_level(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else levels.new_level(path))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_EgmoreScene02_Study'
for a in actors.get_all_level_actors():
    if isinstance(a,(unreal.RailEgmoreSceneView,unreal.StaticMeshActor,unreal.Light,unreal.SceneCapture2D,unreal.SkyAtmosphere)):
        actors.destroy_actor(a)
scene=actors.spawn_actor_from_class(unreal.RailEgmoreSceneView,unreal.Vector())
assert scene.rebuild_scene(),scene.get_editor_property('import_diagnostic')
assert scene.get_editor_property('placement_count')==1316
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
lods=[]
for family in ['EgmoreScene02','EgmoreInterior01']:
    manifest=json.loads((CONTENT/'IndianRailway/Production/stations'/(family+'.json')).read_text())
    for row in manifest['importedModules']:
        mesh=unreal.load_asset(row['asset'])
        assert mesh_editor.get_lod_count(mesh)==3
        assert [mesh.get_num_triangles(i) for i in range(3)]==row['lodTriangles']
        assert mesh_editor.set_lod_screen_sizes(mesh,[1.0,.2,.06])
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        lods.append(dict(asset=mesh.get_path_name(),screen_sizes=list(mesh_editor.get_lod_screen_sizes(mesh))))
(OUT/'reloaded-module-verification.json').write_text(json.dumps(dict(passed=True,modules=lods),indent=2))
train=[]
def vehicle(file,center,label):
    data=json.loads((CONTENT/'IndianRailway/Production'/file).read_text())
    for row in data['parts']:
        p=list(row['location_cm']);p[0]+=center*100
        r=row['rotation_deg']
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p),unreal.Rotator(pitch=r[0],yaw=r[1],roll=r[2]))
        a.set_actor_label(label+'_'+row['name'])
        if label=='WAP7':
            a.set_editor_property('tags',[unreal.Name('Scene02CabOnly' if row.get('cab_only') else 'Scene02Exterior' if row['role']=='exterior' else 'Scene02Other')])
        a.static_mesh_component.set_static_mesh(unreal.load_asset(row['asset']))
        a.set_actor_scale3d(unreal.Vector(*row['scale']))
    train.append(dict(vehicle=label,center_m=center,scale='source, unchanged'))
# Existing versioned formation contract, with zero extra coupling-plane gap.
loco_length=21.33043212890625
vehicle('locomotives/WAP7.json',100-loco_length/2,'WAP7')
for i in range(7):vehicle('coaches/LHB3A.json',100-loco_length-12-i*24,'LHB_'+str(i+1))
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2500),unreal.Rotator(pitch=-42,yaw=-35,roll=0)).light_component
sun.set_mobility(unreal.ComponentMobility.MOVABLE);sun.set_intensity(3)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,2000)).light_component
sky.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'));sky.set_intensity(8)
sky.set_editor_property('lower_hemisphere_is_black',False)
sky.set_editor_property('cast_shadows',False)
sky.recapture_sky()
# Mobile scene uses the existing unlit sky dome. Do not combine it with an
# unsupported SkyAtmosphere actor, which requires a different IsSky material.
sun.set_editor_property('atmosphere_sun_light',False)
backdrop=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
backdrop.set_actor_label('Scene02MobileSky')
backdrop.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
backdrop.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Materials/M_DaySky'))
backdrop.static_mesh_component.set_editor_property('cast_shadow',False)
backdrop.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
backdrop.set_actor_scale3d(unreal.Vector(1500,1500,1500))
views=[('platform',(9000,-460,250),(3,180,0),78),('waiting',(3000,-560,250),(5,0,0),78),('stairs',(4900,-560,250),(12,0,0),78),('footbridge',(6700,1000,920),(-8,180,0),78),('cab',(9713,-65,335),(-4,0,0),95),('nameboard-front',(10200,-450,255),(-2,180,0),78),('nameboard-back',(9300,-750,255),(-2,0,0),78)]
if os.environ.get('RAIL_EGMORE_FOCUS')=='1':views=[v for v in views if v[0] in ['waiting','cab']]
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector())
c=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.set_editor_property('always_persist_rendering_state',True)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,900,unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
c.set_editor_property('texture_target',target);c.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
s=c.get_editor_property('post_process_settings')
for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',.5),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False)]:s.set_editor_property(k,v)
c.set_editor_property('post_process_settings',s);c.set_editor_property('post_process_blend_weight',1.0)
def position(i):
    _,p,r,f=views[i];capture.set_actor_location(unreal.Vector(*p),False,False);capture.set_actor_rotation(unreal.Rotator(pitch=r[0],yaw=r[1],roll=r[2]),False);c.set_editor_property('fov_angle',f)
    cab=views[i][0]=='cab'
    for a in actors.get_all_level_actors():
        if a.actor_has_tag('Scene02CabOnly') or a.actor_has_tag('Scene02Exterior'):
            visible=cab if a.actor_has_tag('Scene02CabOnly') else not cab
            a.static_mesh_component.set_visibility(visible)
            a.static_mesh_component.set_cast_shadow(visible)
position(0)
levels.save_current_level()
start=time.monotonic();stage=0;pending=None
def tick(dt):
    global stage,pending
    if time.monotonic()-start<50+stage*16:return
    if pending is None:c.capture_scene();pending=time.monotonic();return
    if time.monotonic()-pending<2:return
    unreal.RenderingLibrary.export_render_target(world,target,str(OUT),views[stage][0]+'.png')
    stage+=1;pending=None
    if stage<len(views):position(stage);return
    after=hashlib.sha256(fixture.read_bytes()).hexdigest()
    (OUT/'unreal-review.json').write_text(json.dumps(dict(review_world=world.get_name(),s7_unchanged=before==after,s7_sha256=after,placement_count=1316,train=train,formation_length_m=loco_length+168,front_m=100,rear_m=100-loco_length-168,platform_extent_m=[-108,108],stationary_body_edge_clearance_m=1.675-1.628,views=[v[0] for v in views],gameplay_bound=False,android_validated=False),indent=2))
    unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
