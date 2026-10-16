import unreal
ROOT='/Game/IndianRailway/S5'
unreal.EditorAssetLibrary.make_directory(ROOT)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not unreal.EditorAssetLibrary.does_asset_exist(ROOT+'/L_RailAI'):
    assert level.new_level(ROOT+'/L_RailAI',False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,500))
    level.save_current_level()
unreal.log('S5_ASSETS_CREATED')

