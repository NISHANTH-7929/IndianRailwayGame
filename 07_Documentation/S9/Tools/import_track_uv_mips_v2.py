"""Import corrected bed UVs and mipmapped texture derivatives in a full editor.

V1 geometry/textures and original PNGs stay untouched. MaterialsV2 are the only
existing assets changed: every generated-image sample now uses TexturesV2.
The parent task must finish active game/editor reviews before running this file.
"""
import hashlib
import json
import time
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
BASE = '/Game/IndianRailway/S9/TrackEnvironmentV1/'
SOURCE = ROOT / '04_Assets/S9/TrackEnvironmentV1/MeshesV2'
CONTENT = ROOT / '03_UnrealProject/MyProject/Content/IndianRailway/S9/TrackEnvironmentV1'
OUT = ROOT / '07_Documentation/S9/Evidence/TrackEnvironment-20261025-01/uv-mips-v2.json'
E = unreal.MaterialEditingLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S, 'A full editor session is required'
assert not OUT.exists(), 'Preserve the previous correction receipt'
BED_NAME = 'SM_Ballast6m_RealismV2'
BED_PATH = BASE + 'Meshes/' + BED_NAME
TEXTURE_NAMES = ('T_TrackBallast_BC', 'T_DryGround_BC')
MATERIAL_SAMPLES = {'M_TrackBallast': 4, 'M_TrackStone': 1, 'M_DryGround': 3,
                    'M_RailHead': 1, 'M_RailRust': 1, 'M_SleeperConcrete': 1, 'M_Fastener': 1}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stats(mesh):
    count = S.get_lod_count(mesh)
    bounds = mesh.get_bounding_box()
    return dict(lodCount=count, triangles=[mesh.get_num_triangles(i) for i in range(count)],
                sections=[mesh.get_num_sections(i) for i in range(count)],
                uvChannels=[S.get_num_uv_channels(mesh, i) for i in range(count)],
                screenSizes=list(S.get_lod_screen_sizes(mesh)),
                boundsMetres=[[bounds.min.x/100, bounds.min.y/100, bounds.min.z/100],
                              [bounds.max.x/100, bounds.max.y/100, bounds.max.z/100]],
                simpleCollisions=S.get_simple_collision_count(mesh),
                material=mesh.get_material(0).get_path_name())


try:
    # Verify the whole batch and preserve source hashes before the first write.
    assert not unreal.EditorAssetLibrary.does_asset_exist(BED_PATH), 'Fresh V2 bed required'
    source_file = SOURCE / 'mesh-receipt.json'
    source = json.loads(source_file.read_text())
    assert source['revision'] == 'track-environment-mesh-v2'
    rows = sorted((r for r in source['records'] if r['family'] == 'ballast'), key=lambda r: r['lod'])
    assert [r['lod'] for r in rows] == [0, 1, 2]
    assert [r['triangles'] for r in rows] == [446, 230, 122]
    for row in rows:
        assert row['all_faces_metric_xy_uv'] and row['maximum_metric_xy_uv_error'] < 1e-6
        assert row['closed_manifold'] and row['seam_matches']
        assert row['seam_cross_section_m'] == rows[0]['seam_cross_section_m']
        assert digest(ROOT / row['fbx']) == row['sha256']
    protected = [ROOT / '04_Assets/S9/TrackEnvironmentV1/Textures' / (name + '.png') for name in TEXTURE_NAMES]
    protected += [CONTENT / 'Textures' / (name + '.uasset') for name in TEXTURE_NAMES]
    protected += [CONTENT / 'Meshes/SM_Ballast6m_RealismV1.uasset',
                  CONTENT / 'Meshes/SM_AngularBallastStoneV1.uasset',
                  ROOT / '07_Documentation/S9/Tools/build_track_bed_v1.py']
    hashes = {str(p.relative_to(ROOT)): digest(p) for p in protected}
    material_before = {name: digest(CONTENT / 'MaterialsV2' / (name + '.uasset')) for name in MATERIAL_SAMPLES}
    originals = {}
    for name in TEXTURE_NAMES:
        original = unreal.load_asset(BASE + 'Textures/' + name)
        assert isinstance(original, unreal.Texture2D)
        originals[name] = original
    materials = {name: unreal.load_asset(BASE + 'MaterialsV2/' + name) for name in MATERIAL_SAMPLES}
    assert all(isinstance(m, unreal.Material) for m in materials.values())

    textures = {}
    texture_rows = []
    for name, original in originals.items():
        target = BASE + 'TexturesV2/' + name
        # Resume a partial import only at these two named derivative paths.
        texture = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else unreal.EditorAssetLibrary.duplicate_asset(original.get_path_name(), target)
        assert isinstance(texture, unreal.Texture2D)
        texture.set_editor_property('power_of_two_mode', unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
        texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        texture.set_editor_property('max_texture_size', 1024)
        texture.set_editor_property('never_stream', False)
        texture.set_editor_property('srgb', True)
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        texture.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
        assert texture.get_editor_property('power_of_two_mode') == unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO
        assert texture.get_editor_property('mip_gen_settings') == unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE
        assert texture.get_editor_property('max_texture_size') == 1024
        assert not texture.get_editor_property('never_stream')
        assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
        textures[original.get_path_name()] = texture
        textures[texture.get_path_name()] = texture
        # GetNumMips is native-only in UE5.8. Do not invent a measured mip count.
        getter = getattr(texture, 'get_num_mips', None)
        mip_count = int(getter()) if callable(getter) else None
        if mip_count is not None:
            assert mip_count > 1, 'Built texture has no minification mips'
        texture_rows.append(dict(asset=texture.get_path_name(), original=original.get_path_name(),
            powerOfTwoMode=str(texture.get_editor_property('power_of_two_mode')),
            mipGenSettings=str(texture.get_editor_property('mip_gen_settings')),
            maxTextureSize=texture.get_editor_property('max_texture_size'),
            neverStream=texture.get_editor_property('never_stream'),
            actualPlatformMipCount=mip_count, actualPlatformMipCountAvailable=mip_count is not None,
            sourcePngSha256=hashes[str((ROOT/'04_Assets/S9/TrackEnvironmentV1/Textures'/(name+'.png')).relative_to(ROOT))]))

    material_rows = []
    for name, material in materials.items():
        changed = 0
        samples = [n for n in E.get_material_expressions(material) if isinstance(n, unreal.MaterialExpressionTextureSample)]
        assert len(samples) == MATERIAL_SAMPLES[name], name + ': unexpected texture sample count'
        for sample in samples:
            original = sample.get_editor_property('texture')
            assert original and original.get_path_name() in textures, name + ': unexpected source texture'
            sample.set_editor_property('texture', textures[original.get_path_name()])
            changed += 1
        E.recompile_material(material)
        assert unreal.EditorAssetLibrary.save_loaded_asset(material)
        # Avoid the deprecated Python get_used_textures API. Validate persisted
        # graph bindings here; runtime review independently checks the compiled
        # scene with the current GetUsedTextures API after shader warmup.
        used = sorted(set(sample.get_editor_property('texture').get_path_name() for sample in samples))
        assert used and all(p.startswith(BASE+'TexturesV2/') for p in used), name
        material_rows.append(dict(asset=material.get_path_name(), replacedSamples=changed, usedTextures=used,
            beforeSha256=material_before[name], afterSha256=digest(CONTENT/'MaterialsV2'/(name+'.uasset'))))

    task = unreal.AssetImportTask()
    task.filename = str(ROOT / rows[0]['fbx'])
    task.destination_path = BASE + 'Meshes'
    task.destination_name = BED_NAME
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
    data.import_mesh_lods = False
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.import_uniform_scale = 1.0
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    A.import_asset_tasks([task])
    mesh = unreal.load_asset(BED_PATH)
    assert isinstance(mesh, unreal.StaticMesh)
    slots = mesh.get_editor_property('static_materials')
    assert len(slots) == 1 and str(slots[0].get_editor_property('imported_material_slot_name')) == 'M_TrackBallast'
    mesh.set_material(0, materials['M_TrackBallast'])
    assert S.set_lod_screen_sizes(mesh, [1.0])
    for level in (1, 2):
        assert S.import_lod(mesh, level, str(ROOT / rows[level]['fbx'])) == level
    S.remove_collisions(mesh)
    assert S.set_lod_screen_sizes(mesh, [1.0, .2, .06])
    actual = stats(mesh)
    assert actual['lodCount'] == 3 and actual['triangles'] == [446, 230, 122]
    assert actual['sections'] == [1, 1, 1] and actual['uvChannels'] == [1, 1, 1]
    assert all(abs(a-b) < 1e-5 for a,b in zip(actual['screenSizes'], [1.0, .2, .06]))
    error = max(abs(actual['boundsMetres'][i][j]-rows[0]['bounds_m'][i][j]) for i in range(2) for j in range(3))
    assert error < .001 and actual['simpleCollisions'] == 0
    assert actual['material'] == materials['M_TrackBallast'].get_path_name()
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    for path, expected in hashes.items():
        assert digest(ROOT/path) == expected, 'Protected source changed: ' + path
    OUT.write_text(json.dumps(dict(passed=True, bed=BED_PATH, imported=actual,
        sourceReceiptSha256=digest(source_file), sourceUVContract=source['uv_contract'],
        metricXYUVVerifiedOnEverySourceLoop=True, sourceFbxSha256=[r['sha256'] for r in rows],
        importedBoundsErrorMetres=error, textures=texture_rows, materials=material_rows,
        protectedFilesSha256=hashes, stoneReimported=False, mobileTested=False), indent=2))
    unreal.log('TRACK_UV_MIPS_V2_READY ' + str(OUT))
finally:
    started = time.monotonic()
    def finish(_dt):
        if time.monotonic()-started < 10:
            return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle = unreal.register_slate_post_tick_callback(finish)
