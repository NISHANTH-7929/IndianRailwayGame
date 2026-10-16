import unreal
ROOT='/Game/IndianRailway/S3'
unreal.EditorAssetLibrary.make_directory(ROOT)
tools=unreal.AssetToolsHelpers.get_asset_tools()
for name,stable_id,traction,power,aero in [('DA_PoweredHandling',1,150000.,900000.,25.),('DA_CoachHandling',2,0.,0.,15.)]:
    asset=unreal.load_asset(ROOT+'/'+name)
    if not asset:
        factory=unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class',unreal.RailHandlingDefinition)
        asset=tools.create_asset(name,ROOT,unreal.RailHandlingDefinition,factory)
    values={'vehicle_definition_id':stable_id,'version':1,'max_traction_n':traction,'max_power_watts':power,
            'traction_cutoff_mps':16.,'adhesive_mass_fraction':1.,'adhesion_coefficient':.25,
            'rolling_n_per_kg':.015,'aero_n_per_mps_squared':aero,'service_brake_n_per_kg':.8,
            'emergency_brake_n_per_kg':1.2,'traction_response_seconds':.5,'brake_apply_seconds':1.5,
            'brake_release_seconds':2.,'emergency_apply_seconds':.5}
    for key,value in values.items():
        asset.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not unreal.EditorAssetLibrary.does_asset_exist(ROOT+'/L_TrainHandling'):
    assert level.new_level(ROOT+'/L_TrainHandling',False)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,500))
    level.save_current_level()
unreal.log('S3_ASSETS_CREATED')
