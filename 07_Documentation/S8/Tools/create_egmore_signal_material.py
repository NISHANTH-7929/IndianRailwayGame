"""Create the isolated Egmore signal lens material used by authoritative lamps."""
import unreal
path = '/Game/IndianRailway/S8/Egmore'
asset = path + '/M_SignalLens'
material = unreal.load_asset(asset)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_SignalLens', path, unreal.Material, unreal.MaterialFactoryNew())
assert material
edit = unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
color = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter)
color.set_editor_property('parameter_name', 'Color')
color.set_editor_property('default_value', unreal.LinearColor(.005, .005, .005, 1))
assert edit.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('EGMORE_SIGNAL_MATERIAL_READY')
