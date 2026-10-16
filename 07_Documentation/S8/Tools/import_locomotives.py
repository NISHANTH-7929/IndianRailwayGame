"""Run in Unreal Editor Python. Imports the two locally authored locomotive assemblies.
Never modifies simulation data. Failures abort the import; runtime retains its proxy.
"""
import json
import os
import re
from pathlib import Path
import unreal

ROOT = Path(os.environ.get('RAIL_PROJECT_ROOT', r'D:\Project\IndianRailwayGame'))
CONTENT = ROOT / '03_UnrealProject/MyProject/Content'
DEST = '/Game/IndianRailway/S8/Locomotives'
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.EditorStaticMeshLibrary
MAT = unreal.MaterialEditingLibrary

def name(value):
    return re.sub(r'[^A-Za-z0-9_]', '_', value)

def expression(material, cls, **properties):
    result = MAT.create_material_expression(material, cls)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result

def connect(node, material, prop, output=''):
    MAT.connect_material_property(node, output, prop)

def material_for(family, definition):
    path = f'{DEST}/{family}/Materials'
    label = name(definition['name'])
    material = unreal.load_asset(path + '/' + label)
    if material is None:
        material = ASSETS.create_asset(label, path, unreal.Material, unreal.MaterialFactoryNew())
    MAT.delete_all_material_expressions(material)
    rgba = definition.get('base_color', [0.5, 0.5, 0.5, 1])
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT if rgba[3] < 0.99 else unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property('two_sided', rgba[3] < 0.99)
    color = expression(material, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*rgba[:3], 1))
    # Vertex colors are optional baked wear/albedo multipliers, never procedural Blender nodes.
    if definition.get('vertex_color', False):
        vertex = expression(material, unreal.MaterialExpressionVertexColor)
        multiply = expression(material, unreal.MaterialExpressionMultiply)
        MAT.connect_material_expressions(color, '', multiply, 'A')
        MAT.connect_material_expressions(vertex, 'RGB', multiply, 'B')
        color = multiply
    connect(color, material, unreal.MaterialProperty.MP_BASE_COLOR)
    for field, prop, default in [('metallic', unreal.MaterialProperty.MP_METALLIC, 0), ('roughness', unreal.MaterialProperty.MP_ROUGHNESS, .6)]:
        connect(expression(material, unreal.MaterialExpressionConstant, r=float(definition.get(field, default))), material, prop)
    if rgba[3] < .99:
        connect(expression(material, unreal.MaterialExpressionConstant, r=float(rgba[3])), material, unreal.MaterialProperty.MP_OPACITY)
    if 'Lamp' in label:
        intensity = expression(material, unreal.MaterialExpressionScalarParameter, parameter_name='LampIntensity', default_value=4.0)
        emission = expression(material, unreal.MaterialExpressionMultiply)
        MAT.connect_material_expressions(color, '', emission, 'A')
        MAT.connect_material_expressions(intensity, '', emission, 'B')
        connect(emission, material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material

def import_family(family, source=None, asset_family=None, publish_runtime=True):
    asset_family = asset_family or family
    source = Path(source) if source else ROOT / '04_Assets/S8/Locomotives' / family
    assembly = json.loads((source / 'assembly.json').read_text(encoding='utf-8-sig'))
    if not 0 < len(assembly['parts']) <= 64:
        raise ValueError(f'{family}: runtime component budget exceeded')
    materials = {name(d['name']): material_for(asset_family, d) for d in assembly['materials']}
    runtime = {k: v for k, v in assembly.items() if k not in ['materials', 'parts']}
    runtime['parts'] = []
    report = []
    for row in assembly['parts']:
        mesh_name = name(Path(row['fbx']).stem)
        task = unreal.AssetImportTask()
        task.filename = str(source / row['fbx'])
        task.destination_path = f'{DEST}/{asset_family}/Meshes'
        task.destination_name = mesh_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_materials = False
        options.import_textures = False
        options.import_as_skeletal = False
        options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        options.automated_import_should_detect_type = False
        data = options.static_mesh_import_data
        data.combine_meshes = True
        data.auto_generate_collision = False
        data.generate_lightmap_u_vs = False
        data.import_mesh_lo_ds = False
        data.convert_scene = True
        data.convert_scene_unit = True
        data.force_front_x_axis = True
        data.import_uniform_scale = 1.0
        data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
        data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.options = options
        ASSETS.import_asset_tasks([task])
        mesh = unreal.load_asset(f'{task.destination_path}/{mesh_name}')
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError(f'{family}: mesh import failed: {row["fbx"]}')
        slots = mesh.get_editor_property('static_materials')
        for index, slot in enumerate(slots):
            slot_name = name(str(slot.get_editor_property('imported_material_slot_name')))
            material = materials.get(slot_name)
            if material is None:
                raise ValueError(f'Unmapped material {slot_name} on {mesh_name}')
            mesh.set_material(index, material)
        reduction = unreal.EditorScriptingMeshReductionOptions()
        reduction.auto_compute_lod_screen_size = False
        reduction.reduction_settings = [
            unreal.EditorScriptingMeshReductionSettings(percent_triangles=1.0, screen_size=1.0),
            unreal.EditorScriptingMeshReductionSettings(percent_triangles=.55, screen_size=.35),
            unreal.EditorScriptingMeshReductionSettings(percent_triangles=.25, screen_size=.12),
        ]
        EDIT.set_lods(mesh, reduction)
        if EDIT.get_lod_count(mesh) != 3:
            raise RuntimeError("LOD generation requires full editor execution")
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        part = {k: v for k, v in row.items() if k not in ['fbx', 'object']}
        part['asset'] = mesh.get_path_name()
        part.setdefault('rotation_axis', [0, 1, 0])
        aliases = {'brake': 'train_brake', 'speedometer': 'gauge_speed', 'gauge': 'gauge_brake'}
        part['role'] = aliases.get(part['role'], part['role'])
        if part['role'] in ['gauge_speed', 'gauge_brake', 'gauge_power']:
            part.setdefault('rotation_range_deg', [-130, 130])
        elif part['role'] in ['throttle', 'train_brake', 'reverser']:
            part.setdefault('rotation_range_deg', [-35, 35])
        runtime['parts'].append(part)
        report.append({'asset': mesh.get_path_name(), 'lods': EDIT.get_lod_count(mesh), 'materials': len(slots)})
    target = CONTENT / 'IndianRailway/Production/locomotives'
    target.mkdir(parents=True, exist_ok=True)
    if publish_runtime:
        (target / f'{family}.json').write_text(json.dumps(runtime, indent=2), encoding='utf-8')
    return {'family': family, 'parts': report, 'runtime': str(target / f'{family}.json'), 'runtime_data': runtime if not publish_runtime else None}

if __name__ == '__main__':
    reports = [import_family(f) for f in ['WAP7', 'WAG9']]
    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=True, recursive=True)
    target = ROOT / '07_Documentation/S8/Evidence/Locomotives'
    target.mkdir(parents=True, exist_ok=True)
    (target / 'unreal-import.json').write_text(json.dumps(reports, indent=2), encoding='utf-8')
    unreal.log('S8_LOCOMOTIVE_IMPORT_COMPLETE ' + json.dumps(reports))


