import unreal
for source,target in [('M_S1_Left','M_S2_Body'),('M_S1_Right','M_S2_Bogie')]:
    path='/Game/IndianRailway/S2/'+target
    material=unreal.load_asset(path)
    if not material:
        material=unreal.EditorAssetLibrary.duplicate_asset('/Game/IndianRailway/Validation/Materials/'+source,path)
    assert material
    material.set_editor_property('used_with_instanced_static_meshes',True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('S2_INSTANCE_MATERIALS_SAVED')
