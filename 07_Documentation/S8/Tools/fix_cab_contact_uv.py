import unreal
edit=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
m=unreal.load_asset('/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Meshes/SM_WAP7_Cab_StaticCore')
build=edit.get_lod_build_settings(m,0)
build.set_editor_property('generate_lightmap_u_vs',False)
edit.set_lod_build_settings(m,0,build)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('CAB_CONTACT_UV_PRESERVED')
unreal.SystemLibrary.quit_editor()
