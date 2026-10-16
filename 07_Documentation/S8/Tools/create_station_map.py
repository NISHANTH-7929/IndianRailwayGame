"""Save a separate daylit station service map; original LHB map stays intact."""
import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/IndianRailway/S8/LHB/L_RailService_LHB'):raise RuntimeError('Source load failed')
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.DirectionalLight):
        a.set_actor_rotation(unreal.Rotator(-42,-35,0),False);a.light_component.set_editor_property('intensity',3.0)
    if isinstance(a,unreal.SkyLight):
        a.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
        a.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'))
        a.light_component.set_editor_property('intensity',1.4)
# Constant sky uses an inward visible unlit sphere, no gameplay or atmosphere cost.
path='/Game/IndianRailway/S8/Stations/Kit01/Materials'
tools=unreal.AssetToolsHelpers.get_asset_tools();mat=unreal.load_asset(path+'/M_DaySky') or tools.create_asset('M_DaySky',path,unreal.Material,unreal.MaterialFactoryNew())
mat.set_editor_property('two_sided',True);mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
edit=unreal.MaterialEditingLibrary;edit.delete_all_material_expressions(mat)
color=edit.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);color.set_editor_property('constant',unreal.LinearColor(.22,.35,.52,1))
edit.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);edit.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
sky=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
sky.set_actor_label('Station_DaySky');sky.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
sky.static_mesh_component.set_material(0,mat);sky.static_mesh_component.set_editor_property('cast_shadow',False)
sky.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION);sky.set_actor_scale3d(unreal.Vector(1500,1500,1500))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.EditorLoadingAndSavingUtils.save_map(world,'/Game/IndianRailway/S8/Stations/L_RailService_Station'):raise RuntimeError('Save failed')
unreal.log('STATION_MAP_COMPLETE')
