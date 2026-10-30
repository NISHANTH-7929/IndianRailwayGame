"""Import fresh S9 track geometry in a full Unreal Editor Python session.

Run after the TrackEnvironmentV1 materials have been created. A combined entry
may exec this file and manage editor shutdown itself; no quit is requested by
default. Set RAIL_TRACK_MESH_QUIT_EDITOR=1 to request a delayed standalone quit.
No source mesh, gameplay fixture or runtime manifest is modified.
"""
import hashlib
import json
import os
import time
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S9/TrackEnvironmentV1/Meshes'
CONTENT = ROOT / '03_UnrealProject/MyProject/Content'
EVIDENCE = ROOT / '07_Documentation/S9/Evidence/TrackEnvironment-20261025-01'
RECEIPT = EVIDENCE / 'mesh-import.json'
DEST = '/Game/IndianRailway/S9/TrackEnvironmentV1/Meshes'
MATERIAL_ROOT = '/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV2/'
ORIGINAL_BED = '/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Ballast6m'
TARGETS = {
    'ballast': ('SM_Ballast6m_RealismV1', 'M_TrackBallast'),
    'stone': ('SM_AngularBallastStoneV1', 'M_TrackStone'),
}
SCREEN_SIZES = [1.0, .2, .06]
EDITOR = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert EDITOR, 'StaticMeshEditorSubsystem requires a full editor session'


def bounds_metres(mesh):
    box = mesh.get_bounding_box()
    return [[box.min.x/100, box.min.y/100, box.min.z/100],
            [box.max.x/100, box.max.y/100, box.max.z/100]]


def stats(mesh):
    count = EDITOR.get_lod_count(mesh)
    return dict(lod_count=count,
                lod_triangles=[mesh.get_num_triangles(i) for i in range(count)],
                lod_uv_channels=[EDITOR.get_num_uv_channels(mesh,i) for i in range(count)],
                lod_screen_sizes=list(EDITOR.get_lod_screen_sizes(mesh)),
                bounds_m=bounds_metres(mesh),
                material_slots=len(mesh.get_editor_property('static_materials')),
                simple_collision_count=EDITOR.get_simple_collision_count(mesh))


def make_task(row, name):
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / row['fbx'])
    task.destination_path = DEST
    task.destination_name = name
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
    return task


def import_meshes():
    assert not RECEIPT.exists(), 'Fresh evidence receipt required: ' + str(RECEIPT)
    source_receipt_path = SOURCE / 'mesh-receipt.json'
    source_receipt = json.loads(source_receipt_path.read_text())
    records = source_receipt['records']
    assert source_receipt['original_sources_unchanged']
    source_reference = ROOT / source_receipt['source_reference']
    assert hashlib.sha256(source_reference.read_bytes()).hexdigest() == source_receipt['source_sha256']
    source_by_family = {}
    materials = {}
    # Preflight the whole batch before the first import, including every LOD.
    for family, (name, material_name) in TARGETS.items():
        target = DEST + '/' + name
        assert not unreal.EditorAssetLibrary.does_asset_exist(target), 'Fresh target required: ' + target
        rows = sorted((r for r in records if r['family'] == family), key=lambda r:r['lod'])
        assert [r['lod'] for r in rows] == [0,1,2], family
        assert all(r['closed_manifold'] and r['triangles'] <= 2000 for r in rows)
        for row in rows:
            path = ROOT / row['fbx']
            assert hashlib.sha256(path.read_bytes()).hexdigest() == row['sha256'], str(path)
            assert row['material_slots'] == ['M_TrackBallast']
        if family == 'ballast':
            assert all(r['seam_matches'] and r['seam_cross_section_m'] == rows[0]['seam_cross_section_m']
                       for r in rows), 'Authored LOD seams differ'
        source_by_family[family] = rows
        material = unreal.load_asset(MATERIAL_ROOT + material_name)
        assert isinstance(material, unreal.MaterialInterface), 'Missing material: ' + material_name
        materials[family] = material
    original = unreal.load_asset(ORIGINAL_BED)
    assert isinstance(original, unreal.StaticMesh), ORIGINAL_BED
    original_stats = stats(original)
    assert original_stats['lod_count'] >= 3, 'Original bed LOD comparison unavailable'
    protected_paths = [
        CONTENT / 'IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Ballast6m.uasset',
        CONTENT / 'IndianRailway/Production/stations/EgmoreScene02.json',
        source_reference,
    ]
    protected_hashes = {str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()
                        for path in protected_paths}
    imported = []
    for family, (name, material_name) in TARGETS.items():
        rows = source_by_family[family]
        task = make_task(rows[0],name)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        mesh = unreal.load_asset(DEST + '/' + name)
        assert isinstance(mesh,unreal.StaticMesh), 'Import failed: ' + name
        slots = mesh.get_editor_property('static_materials')
        assert len(slots) == 1, 'Unexpected material sections: ' + name
        assert str(slots[0].get_editor_property('imported_material_slot_name')) == 'M_TrackBallast'
        mesh.set_material(0,materials[family])
        # Authored LODs use fixed thresholds. Set this before importing them so
        # each import-triggered build preserves the same manual-size policy.
        assert EDITOR.set_lod_screen_sizes(mesh,[1.0]), name + ': manual LOD policy could not be set'
        for level in (1,2):
            result = EDITOR.import_lod(mesh,level,str(ROOT / rows[level]['fbx']))
            assert result == level, 'Authored LOD import failed: ' + name + ' ' + str(result)
        EDITOR.remove_collisions(mesh)
        # Collision removal may rebuild render data; apply screen sizes after
        # every operation that can rebuild it, then validate the actual values.
        assert EDITOR.set_lod_screen_sizes(mesh,SCREEN_SIZES), name + ': LOD sizes could not be set'
        after = stats(mesh)
        assert after['lod_count'] == 3, name
        expected_triangles = [r['triangles'] for r in rows]
        assert after['lod_triangles'] == expected_triangles, name + ': imported topology differs'
        assert after['lod_uv_channels'] == [1,1,1], name + ': missing metric UV0 or extra generated UVs'
        assert len(after['lod_screen_sizes']) == 3
        assert all(abs(actual-expected) < 1e-5 for actual,expected in zip(after['lod_screen_sizes'],SCREEN_SIZES)), name + ': LOD sizes differ: ' + str(after['lod_screen_sizes'])
        assert after['simple_collision_count'] == 0, name + ': collision geometry introduced'
        assert all(mesh.get_num_sections(i) == 1 for i in range(3)), name + ': extra LOD sections'
        expected_bounds = rows[0]['bounds_m']
        bounds_error = max(abs(after['bounds_m'][i][j]-expected_bounds[i][j])
                           for i in range(2) for j in range(3))
        assert bounds_error < .001, name + ': metric basis mismatch ' + str(after['bounds_m'])
        assert mesh.get_material(0) == materials[family]
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh), name + ': save failed'
        item = dict(family=family, asset=mesh.get_path_name(), imported=after,
                    expected_lod_triangles=expected_triangles, expected_bounds_m=expected_bounds,
                    bounds_error_m=bounds_error, material=materials[family].get_path_name(),
                    fbx_sha256=[r['sha256'] for r in rows],
                    source_closed_manifold=True, normals_imported=True,
                    source_uv_contract=source_receipt['uv_contract'])
        if family == 'ballast':
            item['original_bed_lod_triangles'] = original_stats['lod_triangles'][:3]
            item['triangle_delta_per_lod'] = [a-b for a,b in zip(after['lod_triangles'],original_stats['lod_triangles'])]
            item['authored_seams_match_all_lods'] = True
        imported.append(item)
    assert stats(original) == original_stats, 'Original S8 bed changed'
    for relative,digest in protected_hashes.items():
        assert hashlib.sha256((ROOT/relative).read_bytes()).hexdigest() == digest, 'Protected file changed: ' + relative
    receipt = dict(passed=True, meshes=imported, original_bed_asset=ORIGINAL_BED,
        original_bed=original_stats, protected_files_sha256=protected_hashes,
        source_receipt_sha256=hashlib.sha256(source_receipt_path.read_bytes()).hexdigest(),
        source_normals='Closed manifold outward normals recalculated before FBX; Y reflection and winding correction match prepare_egmore_scene02.py.',
        runtime_manifest_changed=False, collision_added=False,
        visual_accepted=False, mobile_validated=False)
    EVIDENCE.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(receipt,indent=2))
    unreal.log('TRACK_ENVIRONMENT_MESHES_IMPORTED ' + json.dumps([r['asset'] for r in imported]))
    return receipt


try:
    IMPORT_RESULT = import_meshes()
finally:
    if os.environ.get('RAIL_TRACK_MESH_QUIT_EDITOR') == '1':
        _finished = time.monotonic()
        def _finish_tick(delta_seconds):
            if time.monotonic()-_finished < 10:
                return
            unreal.unregister_slate_post_tick_callback(_finish_handle)
            unreal.SystemLibrary.quit_editor()
        _finish_handle = unreal.register_slate_post_tick_callback(_finish_tick)
