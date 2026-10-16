import unreal,json,time
from pathlib import Path
ROOT=Path('D:/Project/IndianRailwayGame')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.new_level('/Game/IndianRailway/S8/Review/L_SelectedLocomotives_Verified')
world=editor.get_editor_world()
report=[]
for family,offset in [('WAP7',-450),('WAG9',450)]:
 d=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/locomotives'/(family+'.json')).read_text())
 for row in d['parts']:
  mesh=unreal.load_asset(row['asset']);loc=row['location_cm'];a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(loc[0],loc[1]+offset,loc[2]));a.set_actor_label(family+'_'+row['name']);a.static_mesh_component.set_static_mesh(mesh)
  a.set_actor_rotation(unreal.Rotator(*row['rotation_deg']),False);a.set_actor_scale3d(unreal.Vector(*row['scale']))
  report.append({'family':family,'part':row['name'],'bounds':str(a.get_actor_bounds(False)), 'lods':unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_lod_count(mesh)})
light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(-35,35,0));light.light_component.set_editor_property('intensity',4)
fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(-35,140,0));fill.light_component.set_editor_property('intensity',2)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'))
sky.light_component.set_editor_property('intensity',2)
camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(2600,-2900,2100));camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,230)),False)

levels.save_current_level()
(ROOT/'07_Documentation/S8/Evidence/Locomotives/unreal-review-bounds.json').write_text(json.dumps(report,indent=2))
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,camera.get_actor_location(),camera.get_actor_rotation())
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('fov_angle',60)
start=time.monotonic()
def finish(delta):
 if time.monotonic()-start<30:return
 component.capture_scene()
 unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'07_Documentation/S8/Evidence/Locomotives'),'selected-unreal-preview.png')
 unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)



