import unreal
path='/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/ReadoutText'
m=unreal.load_asset(path)
if not m:m=unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque',path)
e=unreal.MaterialEditingLibrary
node=e.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
if node:e.connect_material_property(node,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
e.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('CAB_READOUT_READY')
unreal.SystemLibrary.quit_editor()
