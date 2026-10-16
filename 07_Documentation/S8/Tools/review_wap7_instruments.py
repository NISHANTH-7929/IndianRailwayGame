"""Import cab and capture isolated day/night reviews; never save a gameplay map."""
import os,runpy,json,time,hashlib
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[3]
OUT=Path(os.environ.get('RAIL_CAB_REVIEW_OUT',str(ROOT/'07_Documentation/S8/Evidence/Cab/InstrumentRefinement-20261007')))
OUT.mkdir(parents=True,exist_ok=True)
fixture=ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/S7/L_RailService.umap'
before=hashlib.sha256(fixture.read_bytes()).hexdigest()
os.environ['RAIL_CAB_KEEP_EDITOR']='1'
os.environ['RAIL_CAB_IMPORT_RECEIPT']=str(OUT/'unreal-import.json')
if not os.environ.get('RAIL_CAB_REVIEW_ONLY'):
    runpy.run_path(str(Path(__file__).with_name('import_wap7_cab.py')),run_name='__main__')
assert hashlib.sha256(fixture.read_bytes()).hexdigest()==before,'S7 fixture changed during cab import'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
review_path='/Game/IndianRailway/S8/Review/L_CabInstruments_Temporary'
loaded=levels.load_level(review_path) if unreal.EditorAssetLibrary.does_asset_exist(review_path) else levels.new_level(review_path)
assert loaded,'Unable to open isolated cab review level'
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_CabInstruments_Temporary','Refusing to modify a non-review world'
for actor in actors.get_all_level_actors():
    if isinstance(actor,(unreal.StaticMeshActor,unreal.Light,unreal.CameraActor,unreal.SceneCapture2D)):
        actors.destroy_actor(actor)
d=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/locomotives/WAP7.json').read_text())
for row in d['parts']:
    if not row.get('cab_only'):continue
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*row['location_cm']),unreal.Rotator(pitch=row['rotation_deg'][0],yaw=row['rotation_deg'][1],roll=row['rotation_deg'][2]))
    a.set_actor_label('CabInstrumentReview_'+row['name']);a.static_mesh_component.set_static_mesh(unreal.load_asset(row['asset']));a.static_mesh_component.set_forced_lod_model(1);a.set_actor_scale3d(unreal.Vector(*row['scale']))
    if row['role']=='gauge_speed':a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=-130),False)
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-40,yaw=35,roll=0)).light_component
sun.set_mobility(unreal.ComponentMobility.MOVABLE);sun.set_editor_property('intensity',3)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500)).light_component
sky.set_mobility(unreal.ComponentMobility.MOVABLE);sky.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP);sky.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'));sky.set_editor_property('intensity',1.2)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(780,-65,335),unreal.Rotator(pitch=-12,yaw=0,roll=0))
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
component.set_editor_property('always_persist_rendering_state',True)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,900,unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
component.set_editor_property('texture_target',target);component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR);component.set_editor_property('fov_angle',95)
settings=component.get_editor_property('post_process_settings')
for key,value in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',1.5),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False)]:settings.set_editor_property(key,value)
component.set_editor_property('post_process_settings',settings)
component.set_editor_property('post_process_blend_weight',1.0)
start=time.monotonic();stage=0;pending=None;lighting=[]
stabilization=os.environ.get('RAIL_CAB_DAY_STABILIZATION')=='1'
def tick(dt):
    global stage,pending
    if time.monotonic()-start<45+stage*18:return
    if pending is None:
        component.capture_scene();pending=time.monotonic();return
    if time.monotonic()-pending<2:return
    names=['driver-day','instruments-day','left-side-day','right-side-day'] if stabilization else ['driver-day','instruments-day','instruments-night','driver-night']
    unreal.RenderingLibrary.export_render_target(world,target,str(OUT),names[stage]+'.png')
    lighting.append(dict(capture=names[stage],sun=sun.get_editor_property('intensity'),sky=sky.get_editor_property('intensity')))
    pending=None
    stage+=1
    if stabilization and stage<4:
        capture.set_actor_location(unreal.Vector(780,-65,335),False,False)
        capture.set_actor_rotation(unreal.Rotator(pitch=-24 if stage==1 else 0,yaw=0 if stage==1 else (-90 if stage==2 else 90),roll=0),False)
        component.set_editor_property('fov_angle',55 if stage==1 else 95)
    elif stage==1:
        capture.set_actor_location(unreal.Vector(841,-62,328),False,False);capture.set_actor_rotation(unreal.Rotator(pitch=-14,yaw=0,roll=0),False);component.set_editor_property('fov_angle',73)
    elif stage==2:sun.set_intensity(.015);sky.set_intensity(.025)
    elif stage==3:
        capture.set_actor_location(unreal.Vector(780,-65,335),False,False);capture.set_actor_rotation(unreal.Rotator(pitch=-12,yaw=0,roll=0),False);component.set_editor_property('fov_angle',95)
    else:
        after=hashlib.sha256(fixture.read_bytes()).hexdigest()
        (OUT/'unreal-review.json').write_text(json.dumps(dict(review_world=world.get_name(),s7_before=before,s7_after=after,s7_unchanged=before==after,captures=names,lighting=lighting,render_target='RTF_RGBA8_SRGB',driver_fov=95,exposure_bias=1.5,android_validated=False),indent=2))
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
