"""Import original complete-phrase rasters, not fonts; no map or simulation edits."""
import json
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[3]
source = root / '02_Art/Stations/ChennaiNameboards01'
manifest = json.loads((source / 'manifest.json').read_text(encoding='utf-8-sig'))
package = '/Game/IndianRailway/S8/Chennai/Nameboards'
unreal.EditorAssetLibrary.make_directory(package)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
edit = unreal.MaterialEditingLibrary
assets = []
for row in manifest['boards']:
    name = Path(row['file']).stem
    task = unreal.AssetImportTask()
    task.filename = str(source / row['file'])
    task.destination_path = package
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    asset_tools.import_asset_tasks([task])
    texture = unreal.load_asset(package + '/' + name)
    assert texture is not None, name
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    texture.set_editor_property('lod_bias', 0)
    texture.set_editor_property('never_stream', True)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SHARPEN4)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    material_name = 'M_Nameboard_%02d' % row['index']
    material = unreal.load_asset(package + '/' + material_name)
    if material is None:
        material = asset_tools.create_asset(material_name, package, unreal.Material, unreal.MaterialFactoryNew())
    assert material is not None
    edit.delete_all_material_expressions(material)
    material.set_editor_property('two_sided', False)
    tex = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 0)
    tex.set_editor_property('texture', texture)
    edit.connect_material_property(tex, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = edit.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 180)
    rough.set_editor_property('r', .76)
    edit.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    edit.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    edit.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)
    assets.append(dict(index=row['index'], material=package+'/'+material_name, texture=texture.get_path_name()))
report = root / '07_Documentation/S8/Evidence/EgmoreTambaram/nameboards-import.json'
report.write_text(json.dumps(dict(imported=True, count=len(assets), assets=assets, renderedAcceptance=False), indent=2))
unreal.log('CHENNAI_NAMEBOARDS_IMPORTED: 15; rendered acceptance still required')
