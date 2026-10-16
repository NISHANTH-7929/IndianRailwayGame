import unreal
ROOT='/Game/IndianRailway/S2'
unreal.EditorAssetLibrary.make_directory(ROOT)
tools=unreal.AssetToolsHelpers.get_asset_tools()
for name,stable_id,length,mass,front,rear,powered in [('DA_Powered',1,20.,80000.,7.,-7.,True),('DA_Coach',2,22.,40000.,8.,-8.,False)]:
    asset=unreal.load_asset(ROOT+'/'+name)
    if not asset:
        factory=unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class',unreal.RailVehicleDefinition)
        asset=tools.create_asset(name,ROOT,unreal.RailVehicleDefinition,factory)
    for key,value in {'stable_id':stable_id,'version':1,'length_metres':length,'mass_kg':mass,'front_bogie_offset_metres':front,'rear_bogie_offset_metres':rear,'powered':powered}.items():
        asset.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.new_level(ROOT+'/L_RailKernel',False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,500))
level.save_current_level()
unreal.log('S2_ASSETS_CREATED')
