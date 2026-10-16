"""Author an original reusable lit scenery material; no imported imagery or map edits."""
import json
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[3]
package = '/Game/IndianRailway/S8/Chennai'
unreal.EditorAssetLibrary.make_directory(package)
name = 'M_SceneryTint'
material = unreal.load_asset(package + '/' + name)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package, unreal.Material, unreal.MaterialFactoryNew())
assert material is not None
edit = unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property('two_sided', True)
color = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
color.set_editor_property('parameter_name', 'Color')
color.set_editor_property('default_value', unreal.LinearColor(.45, .38, .28, 1))
roughness = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 180)
roughness.set_editor_property('parameter_name', 'Roughness')
roughness.set_editor_property('default_value', .85)
edit.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
edit.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
edit.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
edit.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material)
evidence = root / '07_Documentation/S8/Evidence/EgmoreTambaram/chennai-material.json'
evidence.write_text(json.dumps(dict(asset=package+'/'+name, authored=True,
    copiedImagery=False, parameters=['Color','Roughness'], sourceScript=Path(__file__).name,
    licence='Original project-authored material; no third-party texture'), indent=2))
unreal.log('CHENNAI_MATERIAL_COMPLETE')
