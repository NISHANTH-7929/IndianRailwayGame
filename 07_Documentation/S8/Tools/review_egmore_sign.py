"""Temporary close-up capture; never save capture actor or alter runtime camera."""
import unreal,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(2600,2400,320))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(2600,1700,320)),False)
c=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
c.set_editor_property('texture_target',target);c.set_editor_property('capture_every_frame',True)
c.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
c.set_editor_property('fov_angle',50)
runtime=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='Egmore Android review camera')
c.set_editor_property('post_process_settings',runtime.camera_component.get_editor_property('post_process_settings'))
c.set_editor_property('post_process_blend_weight',1.0)
start=time.monotonic()
def finish(delta):
    if time.monotonic()-start<35:return
    unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'07_Documentation/S8/Evidence/Egmore'),'unreal-signage-study.png')
    unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
