"""Unreal Python commandlet: preserve acquired glTF PBR and activate WAP7 assembly.

Original draft assets remain untouched. Imported assets use WAP7/Acquired,
which satisfies the runtime class path allowlist without a C++ change.
"""
import importlib.util
import json
import shutil
from pathlib import Path
import unreal

spec = importlib.util.spec_from_file_location('s8_import', Path(__file__).with_name('import_locomotives.py'))
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
SOURCE = None
MAT = base.MAT
textures = {}


def scalar(material, value):
    return base.expression(material, unreal.MaterialExpressionConstant, r=float(value))


def vector(material, values):
    return base.expression(material, unreal.MaterialExpressionConstant3Vector,
                           constant=unreal.LinearColor(*values[:3], 1))


def multiply(material, left, right, output=''):
    node = base.expression(material, unreal.MaterialExpressionMultiply)
    MAT.connect_material_expressions(left, output, node, 'A')
    MAT.connect_material_expressions(right, '', node, 'B')
    return node


def texture(material, family, info, kind):
    source = (SOURCE / info['uri']).resolve()
    if not source.is_relative_to(SOURCE.resolve()) or not source.is_file():
        raise ValueError('Missing or unsafe texture path: ' + str(source))
    key = (str(source), kind)
    if key not in textures:
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = f'{base.DEST}/{family}/Textures'
        task.destination_name = base.name(source.stem) + '_' + kind
        task.automated = True
        task.replace_existing = True
        task.save = True
        base.ASSETS.import_asset_tasks([task])
        asset = unreal.load_asset(task.destination_path + '/' + task.destination_name)
        if not isinstance(asset, unreal.Texture2D):
            raise RuntimeError('Texture import failed: ' + str(source))
        asset.set_editor_property('srgb', kind == 'color')
        asset.set_editor_property('compression_settings',
            unreal.TextureCompressionSettings.TC_NORMALMAP if kind == 'normal' else
            unreal.TextureCompressionSettings.TC_MASKS if kind == 'mask' else
            unreal.TextureCompressionSettings.TC_DEFAULT)
        # glTF tangent normals use +Y; Unreal uses DirectX -Y.
        if kind == 'normal':
            asset.set_editor_property('flip_green_channel', True)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        textures[key] = asset
    node = base.expression(material, unreal.MaterialExpressionTextureSample, texture=textures[key])
    node.set_editor_property('sampler_type',
        unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind == 'normal' else
        unreal.MaterialSamplerType.SAMPLERTYPE_MASKS if kind == 'mask' else
        unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    uv = base.expression(material, unreal.MaterialExpressionTextureCoordinate,
                         coordinate_index=int(info.get('texCoord', 0)))
    MAT.connect_material_expressions(uv, '', node, 'UVs')
    return node


def material_for(family, definition):
    path = f'{base.DEST}/{family}/Materials'
    label = base.name(definition['name'])
    material = unreal.load_asset(path + '/' + label)
    if material is None:
        material = base.ASSETS.create_asset(label, path, unreal.Material, unreal.MaterialFactoryNew())
    MAT.delete_all_material_expressions(material)
    mode = definition.get('alphaMode', 'OPAQUE')
    material.set_editor_property('blend_mode', {'OPAQUE': unreal.BlendMode.BLEND_OPAQUE,
        'MASK': unreal.BlendMode.BLEND_MASKED, 'BLEND': unreal.BlendMode.BLEND_TRANSLUCENT}[mode])
    material.set_editor_property('two_sided', bool(definition.get('doubleSided', False)))
    material.set_editor_property('opacity_mask_clip_value', float(definition.get('alphaCutoff', .5)))
    pbr = definition.get('pbrMetallicRoughness', {})
    rgba = pbr.get('baseColorFactor', [1, 1, 1, 1])
    color = vector(material, rgba)
    albedo = texture(material, family, pbr['baseColorTexture'], 'color') if 'baseColorTexture' in pbr else None
    if albedo:
        color = multiply(material, albedo, color, 'RGB')
    base.connect(color, material, unreal.MaterialProperty.MP_BASE_COLOR)
    packed = texture(material, family, pbr['metallicRoughnessTexture'], 'mask') if 'metallicRoughnessTexture' in pbr else None
    for field, channel, prop in [('roughnessFactor', 'G', unreal.MaterialProperty.MP_ROUGHNESS),
                                  ('metallicFactor', 'B', unreal.MaterialProperty.MP_METALLIC)]:
        value = scalar(material, pbr.get(field, 1))
        if packed:
            value = multiply(material, packed, value, channel)
        base.connect(value, material, prop)
    if 'normalTexture' in definition:
        info = definition['normalTexture']
        normal = texture(material, family, info, 'normal')
        scale = float(info.get('scale', 1))
        if scale != 1:
            normal = multiply(material, normal, vector(material, [scale, scale, 1]), 'RGB')
        base.connect(normal, material, unreal.MaterialProperty.MP_NORMAL, 'RGB' if scale == 1 else '')
    if mode != 'OPAQUE':
        alpha = scalar(material, rgba[3])
        if albedo:
            alpha = multiply(material, albedo, alpha, 'A')
        base.connect(alpha, material, unreal.MaterialProperty.MP_OPACITY_MASK if mode == 'MASK' else unreal.MaterialProperty.MP_OPACITY)
    if 'emissiveFactor' in definition:
        strength = definition.get('extensions', {}).get('KHR_materials_emissive_strength', {}).get('emissiveStrength', 1)
        emission = vector(material, [v * strength for v in definition['emissiveFactor']])
        if 'emissiveTexture' in definition:
            emission = multiply(material, texture(material, family, definition['emissiveTexture'], 'color'), emission, 'RGB')
        base.connect(emission, material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


base.material_for = material_for
reports=[]
for family in ['WAP7','WAG9']:
    SOURCE=base.ROOT/'04_Assets/S8/Acquired'/(family+'_Selected')/'Prepared'
    textures.clear()
    report=base.import_family(family, source=SOURCE, asset_family=family+'/Selected')
    report['texture_count']=len(textures)
    reports.append(report)
unreal.EditorAssetLibrary.save_directory(base.DEST, only_if_is_dirty=True, recursive=True)
evidence=base.ROOT/'07_Documentation/S8/Evidence/Locomotives'
evidence.mkdir(parents=True,exist_ok=True)
(evidence/'selected-locomotives-unreal-import.json').write_text(json.dumps(reports,indent=2))
unreal.log('S8_SELECTED_IMPORT_COMPLETE '+json.dumps(reports))

unreal.SystemLibrary.quit_editor()
