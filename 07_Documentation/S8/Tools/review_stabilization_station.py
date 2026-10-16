"""Capture the existing service scene in memory; never save maps or assets."""
import unreal, json, hashlib, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/Stabilization-20261010/StationDesktop'
OUT.mkdir(parents=True,exist_ok=True)
CONTENT=ROOT/'03_UnrealProject/MyProject/Content'
def hashes():
    return {str(p.relative_to(CONTENT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in CONTENT.rglob('*.umap')}
before=hashes()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/IndianRailway/S8/Egmore/L_RailService_Egmore')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_RailService_Egmore'
report={'lights':[],'scenes':[],'desktopOnly':True,'assetsSaved':False}
for a in actors.get_all_level_actors():
    if isinstance(a,(unreal.DirectionalLight,unreal.SkyLight)):
        c=a.light_component
        report['lights'].append(dict(label=a.get_actor_label(),intensity=c.get_editor_property('intensity'),mobility=str(c.get_editor_property('mobility')),castShadows=c.get_editor_property('cast_shadows')))
    if isinstance(a,unreal.RailEgmoreSceneView):
        report['scenes'].append(dict(placements=a.get_editor_property('placement_count'),diagnostic=str(a.get_editor_property('import_diagnostic'))))
views=[('platform',(9000,-460,250),(3,180,0)),('waiting',(3000,-560,250),(5,0,0)),('stairs',(4900,-560,250),(12,0,0))]
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector())
c=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
c.set_editor_property('capture_every_frame',False)
c.set_editor_property('capture_on_movement',False)
c.set_editor_property('always_persist_rendering_state',True)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,900,unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
c.set_editor_property('texture_target',target)
c.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
c.set_editor_property('fov_angle',78)
# Match existing scene review exposure, without altering saved lighting/materials.
settings=c.get_editor_property('post_process_settings')
for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',.5),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False)]:settings.set_editor_property(k,v)
c.set_editor_property('post_process_settings',settings)
def position(i):
    _,p,r=views[i]
    capture.set_actor_location(unreal.Vector(*p),False,False)
    capture.set_actor_rotation(unreal.Rotator(pitch=r[0],yaw=r[1],roll=r[2]),False)
position(0)
start=time.monotonic();stage=0;pending=None
def tick(dt):
    global stage,pending
    if time.monotonic()-start<35+stage*12:return
    if pending is None:c.capture_scene();pending=time.monotonic();return
    if time.monotonic()-pending<2:return
    unreal.RenderingLibrary.export_render_target(world,target,str(OUT),views[stage][0]+'.png')
    stage+=1;pending=None
    if stage<len(views):position(stage);return
    after=hashes()
    report.update(mapsUnchanged=before==after,mapHashes=after,views=views,exposureBias=.5)
    (OUT/'receipt.json').write_text(json.dumps(report,indent=2))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
