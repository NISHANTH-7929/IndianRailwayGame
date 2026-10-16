import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level('/Game/IndianRailway/S7/L_RailService')
for a in actors.get_all_level_actors():
 if a.get_actor_label()=='S8_InspectionSun':
  a.set_actor_rotation(unreal.Rotator(pitch=-40,yaw=35,roll=0),False)
  a.light_component.set_editor_property('dynamic_shadow_distance_movable_light',3000)
  a.light_component.set_editor_property('dynamic_shadow_cascades',2)
levels.save_current_level()
unreal.SystemLibrary.quit_editor()
