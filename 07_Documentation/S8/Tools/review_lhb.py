"""Dedicated Unreal review map/capture; never edits the S7 service map."""
import unreal,json,time
from pathlib import Path
ROOT=Path('D:/Project/IndianRailwayGame')
CONTENT=ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_path='/Game/IndianRailway/S8/Review/L_LHBFormation'
if unreal.EditorAssetLibrary.does_asset_exist(level_path):levels.load_level(level_path)
else:levels.new_level(level_path)
for old in actors.get_all_level_actors():
 if old.get_class().get_name() not in ('WorldSettings','LevelScriptActor','Brush'):actors.destroy_actor(old)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
report=[]
for document,offset in [(CONTENT/'coaches/LHB3A.json',0),(CONTENT/'locomotives/WAP7.json',(24+21.33043212890625)*50)]:
 assembly=json.loads(document.read_text(encoding='utf-8-sig'))
 for part in assembly['parts']:
  if part.get('cab_only'):continue
  location=list(part['location_cm']);location[0]+=offset
  actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location))
  actor.static_mesh_component.set_static_mesh(unreal.load_asset(part['asset']))
  actor.set_actor_rotation(unreal.Rotator(*part['rotation_deg']),False)
  actor.set_actor_scale3d(unreal.Vector(*part['scale']))
  origin,extent=actor.get_actor_bounds(False)
  report.append({'asset':part['asset'],'origin_cm':[origin.x,origin.y,origin.z],'extent_cm':[extent.x,extent.y,extent.z]})
for yaw,intensity in [(35,4),(140,2)]:
 light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(-35,yaw,0))
 light.light_component.set_editor_property('intensity',intensity)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'))
sky.light_component.set_editor_property('intensity',1.5)
location=unreal.Vector(3800,-5000,2100)
rotation=unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(900,0,200))
camera=actors.spawn_actor_from_class(unreal.CameraActor,location,rotation)
levels.save_current_level()
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,location,rotation)
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('fov_angle',60)
out=ROOT/'07_Documentation/S8/Evidence/LHB'
(out/'unreal-review-bounds.json').write_text(json.dumps(report,indent=2))
start=time.monotonic()
def finish(delta):
 if time.monotonic()-start<30:return
 component.capture_scene()
 unreal.RenderingLibrary.export_render_target(world,target,str(out),'wap7-lhb-unreal.png')
 unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
