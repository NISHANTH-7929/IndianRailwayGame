"""Import the cab hardware derivative into a fresh S9 mesh asset.

Run inside a full Unreal Editor Python session, after the Blender exporter.
No S8 asset, material graph, runtime manifest or gameplay fixture is changed.
"""
import hashlib
import json
import re
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S9/CabHardwareV1'
CONTENT = ROOT / '03_UnrealProject/MyProject/Content'
EVIDENCE = ROOT / '07_Documentation/S9/Evidence/CabRealism-20261023-01'
RECEIPT = EVIDENCE / 'hardware-import.json'
DEST = '/Game/IndianRailway/S9/CabRealismV1/Meshes'
MESH_NAME = 'SM_WAP7_Cab_StaticCore_HardwareV1'
TARGET = DEST + '/' + MESH_NAME
ORIGINAL = '/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Meshes/SM_WAP7_Cab_StaticCore'
MATERIAL_ROOT = '/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/'
assert not RECEIPT.exists(), 'Use a fresh evidence revision; import receipt already exists'
assert not unreal.EditorAssetLibrary.does_asset_exist(TARGET), 'Fresh target required: ' + TARGET

source_receipt = json.loads((SOURCE / 'receipt.json').read_text())
fbx = SOURCE / (MESH_NAME + '.fbx')
assert source_receipt['original_uvs_preserved'] and source_receipt['original_geometry_preserved']
assert source_receipt['added_runtime_components'] == 0 and source_receipt['added_triangles'] <= 650
assert hashlib.sha256(fbx.read_bytes()).hexdigest() == source_receipt['output_sha256'][fbx.name]
for relative, digest in source_receipt['source_sha256'].items():
    assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest, 'Source changed: ' + relative

editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert editor, 'StaticMeshEditorSubsystem required'
original = unreal.load_asset(ORIGINAL)
assert isinstance(original, unreal.StaticMesh), ORIGINAL
protected_paths = [CONTENT / 'IndianRailway/Production/locomotives/WAP7.json',
                   CONTENT / 'IndianRailway/S8/Locomotives/WAP7/Cab/Meshes/SM_WAP7_Cab_StaticCore.uasset']
protected_hashes = {str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in protected_paths}


def label(value):
    return re.sub(r'[^A-Za-z0-9_]', '_', str(value))


def bounds(mesh):
    box = mesh.get_bounding_box()
    return [[box.min.x, box.min.y, box.min.z], [box.max.x, box.max.y, box.max.z]]


def mesh_stats(mesh):
    count = editor.get_lod_count(mesh)
    return dict(lod_count=count, lod_triangles=[mesh.get_num_triangles(i) for i in range(count)],
                lod_vertices=[editor.get_number_verts(mesh,i) for i in range(count)],
                lod_uv_channels=[editor.get_num_uv_channels(mesh,i) for i in range(count)],
                lod_screen_sizes=list(editor.get_lod_screen_sizes(mesh)),
                bounds_cm=bounds(mesh), material_slots=len(mesh.get_editor_property('static_materials')))


before = mesh_stats(original)
original_slots = original.get_editor_property('static_materials')
original_materials = {}
for index, slot in enumerate(original_slots):
    key = label(slot.get_editor_property('imported_material_slot_name'))
    material = original.get_material(index)
    assert material and material.get_path_name().startswith(MATERIAL_ROOT), key
    assert key not in original_materials, 'Duplicate original material slot: ' + key
    original_materials[key] = material

task = unreal.AssetImportTask()
task.filename = str(fbx)
task.destination_path = DEST
task.destination_name = MESH_NAME
task.automated = True
task.replace_existing = False
task.save = False
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
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh = unreal.load_asset(TARGET)
assert isinstance(mesh, unreal.StaticMesh), 'Mesh import failed'
slots = mesh.get_editor_property('static_materials')
assert len(slots) == len(original_slots), 'Material slot budget changed'
assigned = []
for index, slot in enumerate(slots):
    key = label(slot.get_editor_property('imported_material_slot_name'))
    assert key in original_materials, 'Unmapped original S8 material: ' + key
    mesh.set_material(index, original_materials[key])
    assigned.append(dict(slot=index, imported_name=key, material=original_materials[key].get_path_name()))
assert set(row['imported_name'] for row in assigned) == set(original_materials)

reduction = unreal.EditorScriptingMeshReductionOptions()
reduction.auto_compute_lod_screen_size = False
reduction.reduction_settings = [
    unreal.EditorScriptingMeshReductionSettings(percent_triangles=1.0, screen_size=1.0),
    unreal.EditorScriptingMeshReductionSettings(percent_triangles=.55, screen_size=.35),
    unreal.EditorScriptingMeshReductionSettings(percent_triangles=.25, screen_size=.12),
]
editor.set_lods(mesh, reduction)
after = mesh_stats(mesh)
assert after['lod_count'] == 3, 'LOD generation requires full editor execution'
assert all(abs(actual-expected) < .0001 for actual,expected in zip(after['lod_screen_sizes'],[1.,.35,.12]))
assert len(after['lod_screen_sizes']) == 3
assert after['lod_triangles'][0] > after['lod_triangles'][1] > after['lod_triangles'][2] > 0
assert after['lod_uv_channels'] == before['lod_uv_channels'], 'Original UV-channel count changed'
assert after['lod_uv_channels'][0] >= 2, 'Contact/detail UV channels absent'
bound_delta = [[actual-reference for actual,reference in zip(a,b)]
               for a,b in zip(after['bounds_cm'],before['bounds_cm'])]
assert max(abs(value) for row in bound_delta for value in row) < .01, 'Cab outer bounds changed'
imported_delta = after['lod_triangles'][0] - before['lod_triangles'][0]
assert 0 <= imported_delta <= 650, 'Imported triangle delta exceeds hardware budget'
cab_triangles = (source_receipt['baseline_cab_triangles'] - source_receipt['baseline_static_triangles']
                 + after['lod_triangles'][0])
assert cab_triangles < 50000, 'Cab triangle budget exceeded'
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
assert mesh_stats(original) == before, 'Original S8 mesh modified'
for relative,digest in protected_hashes.items():
    assert hashlib.sha256((ROOT/relative).read_bytes()).hexdigest() == digest, 'Protected file changed: ' + relative
receipt = dict(passed=True, asset=mesh.get_path_name(), source_asset=original.get_path_name(),
               original=before, imported=after, bounds_delta_cm=bound_delta,
               imported_lod0_triangle_delta=imported_delta, full_cab_lod0_triangles=cab_triangles,
               triangle_budget=50000, added_triangle_budget=650, added_runtime_components=0,
               material_slots_preserved=True, materials=assigned,
               source_uv_preservation_verified=source_receipt['original_uvs_preserved'],
               protected_files_sha256=protected_hashes,
               fbx_sha256=source_receipt['output_sha256'][fbx.name],
               runtime_manifest_changed=False, visual_accepted=False, mobile_validated=False)
EVIDENCE.mkdir(parents=True,exist_ok=True)
RECEIPT.write_text(json.dumps(receipt,indent=2))
unreal.log('CAB_HARDWARE_V1_IMPORTED ' + json.dumps(dict(asset=mesh.get_path_name(),triangles=cab_triangles)))
