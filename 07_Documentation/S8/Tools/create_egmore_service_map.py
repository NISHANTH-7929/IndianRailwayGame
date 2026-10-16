"""Create separately named service/chunk maps from verified presentation assets."""
import unreal,json,time,hashlib,gc
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/EgmoreScene02'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets=unreal.EditorAssetLibrary
fixture=ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/S7/L_RailService.umap'
before=hashlib.sha256(fixture.read_bytes()).hexdigest()
for suffix,chunk in [('A',1),('B',2)]:
    path='/Game/IndianRailway/S8/Egmore/L_EgmoreChunk'+suffix
    assert (levels.load_level(path) if assets.does_asset_exist(path) else levels.new_level_from_template(path,'/Game/IndianRailway/S6/L_Chunk'+suffix))
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.RailChunkView):a.set_editor_property('chunk_id',chunk)
    levels.save_current_level()
    del a
    gc.collect()
path='/Game/IndianRailway/S8/Egmore/L_RailService_Egmore'
assert (levels.load_level(path) if assets.does_asset_exist(path) else levels.new_level_from_template(path,'/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study'))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_RailService_Egmore'
scene=None
for a in actors.get_all_level_actors():
    label=a.get_actor_label()
    if isinstance(a,(unreal.SceneCapture2D,unreal.CameraActor,unreal.SkyAtmosphere)) or label.startswith(('WAP7_','LHB_')):
        actors.destroy_actor(a)
    elif isinstance(a,unreal.RailEgmoreSceneView):scene=a
assert scene
scene.set_editor_property('include_service_track_extension',True)
assert scene.rebuild_scene()
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.DirectionalLight):a.light_component.set_editor_property('atmosphere_sun_light',False)
levels.save_current_level()
(OUT/'service-map-authoring.json').write_text(json.dumps(dict(map=path,scene_instances=scene.get_editor_property('placement_count'),s7_unchanged=hashlib.sha256(fixture.read_bytes()).hexdigest()==before,required_flags=['S8Egmore','S8Station','S8LHB'],service='S8-Egmore-01',content='IR-S8-EGMORE-1',stationary_train_removed=True,sky='existing mobile sky dome; SkyAtmosphere removed',signals_bound=False,android_validated=False),indent=2))
finished=time.monotonic()
def tick(dt):
    if time.monotonic()-finished<10:return
    unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
