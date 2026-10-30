"""Import the authored V2 rails after the parent creates MaterialsV3.

Run in a full Unreal Editor session. Imports only the new V2 rail asset,
checks authored LODs/materials/bounds, saves it, and quits after ten seconds.
"""
import hashlib
import json
import time
from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[3]
BASE = '/Game/IndianRailway/S9/TrackEnvironmentV1/'
SOURCE = ROOT / '04_Assets/S9/TrackEnvironmentV1/RailsV2'
OUT = ROOT / '07_Documentation/S9/Evidence/TrackRealism-20261027-02/rails-v2-import.json'
NAME = 'SM_Track6m_RealismV2'
PATH = BASE + 'Meshes/' + NAME
MAPPING = {'RailHead': 'M_RailHead', 'RailRust': 'M_RailRust', 'Concrete': 'M_SleeperConcrete'}
A = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S, 'Run this importer in a full editor'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


try:
    assert not OUT.exists(), 'Preserve previous import evidence'
    assert not unreal.EditorAssetLibrary.does_asset_exist(PATH), 'Import a fresh V2 derivative only'
    source = json.loads((SOURCE / 'rails-receipt.json').read_text())
    assert source['revision'] == 'track-rails-v2'
    rows = sorted(source['records'], key=lambda row: row['lod'])
    assert [row['lod'] for row in rows] == [0, 1, 2]
    for row in rows:
        assert row['shapedRailHeadWebFootPreserved'] and row['taperedSleeperTopPreserved']
        assert digest(ROOT / row['fbx']) == row['sha256']
    materials = {key: unreal.load_asset(BASE + 'MaterialsV3/' + name) for key, name in MAPPING.items()}
    assert all(isinstance(material, unreal.MaterialInterface) for material in materials.values()), 'Create MaterialsV3 first'
    protected = ROOT / '03_UnrealProject/MyProject/Content/IndianRailway/S9/TrackEnvironmentV1/Meshes/SM_Track6m_RealismV1.uasset'
    original_hash = digest(protected)
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / rows[0]['fbx'])
    task.destination_path = BASE + 'Meshes'
    task.destination_name = NAME
    task.automated = True
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
    data.import_uniform_scale = 1.
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    A.import_asset_tasks([task])
    mesh = unreal.load_asset(PATH)
    assert isinstance(mesh, unreal.StaticMesh)
    slots = mesh.get_editor_property('static_materials')
    assert len(slots) == 3
    for index, slot in enumerate(slots):
        key = str(slot.get_editor_property('imported_material_slot_name'))
        assert key in materials, key
        mesh.set_material(index, materials[key])
    assert S.set_lod_screen_sizes(mesh, [1.])
    for lod in (1, 2):
        assert S.import_lod(mesh, lod, str(ROOT / rows[lod]['fbx'])) == lod
    S.remove_collisions(mesh)
    assert S.set_lod_screen_sizes(mesh, [1., .24, .055])
    assert S.get_lod_count(mesh) == 3
    counts = [mesh.get_num_triangles(lod) for lod in range(3)]
    sections = [mesh.get_num_sections(lod) for lod in range(3)]
    assert counts == [row['triangles'] for row in rows], counts
    assert sections == [3, 3, 3], sections
    assert all(S.get_num_uv_channels(mesh, lod) >= 1 for lod in range(3))
    assert S.get_simple_collision_count(mesh) == 0
    bounds = mesh.get_bounding_box()
    measured = [[bounds.min.x / 100., bounds.min.y / 100., bounds.min.z / 100.],
                [bounds.max.x / 100., bounds.max.y / 100., bounds.max.z / 100.]]
    for actual_row, expected_row in zip(measured, rows[0]['boundsMetres']):
        assert all(abs(a - b) < .0001 for a, b in zip(actual_row, expected_row)), measured
    assert abs(measured[0][0]) < .0001 and abs(measured[1][0] - 6.) < .0001
    assert abs(measured[1][2]) < .0001 and abs(measured[0][2] + .335) < .0001
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    assert digest(protected) == original_hash, 'Original V1 asset changed'
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(dict(passed=True, railMesh=PATH, triangles=counts, sections=sections,
                                   boundsMetres=measured, lodScreenSizes=list(S.get_lod_screen_sizes(mesh)),
                                   materials=[mesh.get_material(i).get_path_name() for i in range(3)],
                                   railHeadWebFootAllLODs=True, taperedSleeperTopAllLODs=True,
                                   sourceReceipt=str((SOURCE / 'rails-receipt.json').relative_to(ROOT)),
                                   originalV1SHA256=original_hash, originalV1Unchanged=True,
                                   estimatedPresentation=True), indent=2))
    unreal.log('V2 rail import and all authored LOD checks passed: ' + str(OUT))
finally:
    started = time.monotonic()
    def finish(_dt):
        if time.monotonic() - started < 10:
            return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle = unreal.register_slate_post_tick_callback(finish)
