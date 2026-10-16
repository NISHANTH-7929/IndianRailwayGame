import unreal

ROOT = '/Game/IndianRailway/S6'
unreal.EditorAssetLibrary.make_directory(ROOT)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
view_class = unreal.load_class(None, '/Script/MyProject.RailChunkView')
assert view_class
for index, suffix in [(1, 'A'), (2, 'B')]:
    mesh_path = ROOT + '/SM_Proxy' + suffix
    material_path = ROOT + '/M_Proxy' + suffix
    if not unreal.EditorAssetLibrary.does_asset_exist(mesh_path):
        assert unreal.EditorAssetLibrary.duplicate_asset('/Engine/BasicShapes/Cube', mesh_path)
    if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
        assert unreal.EditorAssetLibrary.duplicate_asset('/Game/IndianRailway/S2/M_S2_Bogie', material_path)
    assert levels.new_level(ROOT + '/L_Chunk' + suffix, False)
    view = actors.spawn_actor_from_class(view_class, unreal.Vector(0, 0, 0))
    view.set_editor_property('chunk_id', index)
    for component in view.get_components_by_class(unreal.InstancedStaticMeshComponent):
        component.set_static_mesh(unreal.load_asset(mesh_path))
        component.set_material(0, unreal.load_asset(material_path if component.get_name() == 'Scenery' else '/Game/IndianRailway/S2/M_S2_Body'))
    assert levels.save_current_level()
assert levels.new_level(ROOT + '/L_RailContinuity', False)
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 500))
assert levels.save_current_level()
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
unreal.log('S6_ASSETS_CREATED')
