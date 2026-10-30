"""Import the WAP-7 cab V2 parts and switch the WAP-7 assembly to them.

Run as an Unreal Python commandlet after build_wap7_cab_v2.py. Meshes land under
/Game/IndianRailway/S8/Locomotives/WAP7/CabV2 (the locomotive loader only accepts assets
below the class folder) and reuse the existing cab materials by slot name, so the S9
journey material overrides still apply. Exterior and bogie rows of WAP7.json are kept.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S10/WAP7CabV2'
CONTENT = ROOT / '03_UnrealProject/MyProject/Content'
DEST = '/Game/IndianRailway/S8/Locomotives/WAP7/CabV2'
MATERIALS = '/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/'
ASSEMBLY = CONTENT / 'IndianRailway/Production/locomotives/WAP7.json'

source = json.loads((SOURCE / 'assembly.json').read_text())
tools = unreal.AssetToolsHelpers.get_asset_tools()
rows = []
for part in source['parts']:
    target = DEST + '/' + part['mesh']
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        assert unreal.EditorAssetLibrary.delete_asset(target), target
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE / part['fbx']); task.destination_path = DEST; task.destination_name = part['mesh']
    task.automated = True; task.replace_existing = True; task.save = False
    options = unreal.FbxImportUI()
    options.import_mesh = True; options.import_materials = False; options.import_textures = False
    options.import_as_skeletal = False; options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type = False
    data = options.static_mesh_import_data
    data.combine_meshes = True; data.auto_generate_collision = False; data.generate_lightmap_u_vs = False
    data.convert_scene = True; data.convert_scene_unit = True; data.force_front_x_axis = True
    data.import_uniform_scale = 1.0; data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(target); assert isinstance(mesh, unreal.StaticMesh), target
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        name = str(slot.get_editor_property('imported_material_slot_name')).split('.')[0]
        material = unreal.load_asset(MATERIALS + name)
        assert material, 'Missing cab material ' + name
        mesh.set_material(index, material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    row = {k: part[k] for k in ('role', 'location_cm', 'rotation_deg', 'scale', 'cab_only', 'rotation_axis', 'rotation_range_deg', 'triangles')}
    row['asset'] = target + '.' + part['mesh']
    rows.append(row)

assembly = json.loads(ASSEMBLY.read_text())
kept = [p for p in assembly['parts'] if not p.get('cab_only')]
assembly['parts'] = kept + rows
for key in ('driver_camera', 'instrument_display', 'speed_readout', 'cab_light_location_cm', 'instrument_view_pitch_deg'):
    assembly[key] = source[key]
assembly['cab_presentation_revision'] = source['revision']
assembly['cab_reference_limits'] = source['limitations']
assembly['triangles'] = sum(p.get('triangles', 0) for p in assembly['parts'])
ASSEMBLY.write_text(json.dumps(assembly, indent=2))
unreal.log_warning('CABV2_IMPORTED parts=%d' % len(rows))
