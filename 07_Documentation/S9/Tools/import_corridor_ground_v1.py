"""Root-coordinated full-editor importer; call import_assets(material).

No editor quit, unrelated material modification or overwriting existing assets.
"""
from pathlib import Path
import hashlib
import json
import unreal

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/'04_Assets/S9/CorridorNatureV1/Ground/mesh-receipt.json'
DEST='/Game/IndianRailway/S9/CorridorNatureV1/Meshes'
NAME='SM_CorridorVerge12m'


def import_assets(material):
    assert isinstance(material, unreal.MaterialInterface)
    assert not unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+NAME)
    receipt=json.loads(SOURCE.read_text())
    rows=receipt['records']
    assert [r['lod'] for r in rows]==[0,1,2]
    for row in rows:
        assert hashlib.sha256((ROOT/row['fbx']).read_bytes()).hexdigest()==row['sha256']
    task=unreal.AssetImportTask()
    task.filename=str(ROOT/rows[0]['fbx']);task.destination_path=DEST
    task.destination_name=NAME;task.automated=True;task.replace_existing=False;task.save=False
    options=unreal.FbxImportUI()
    options.import_mesh=True;options.import_materials=False;options.import_textures=False
    options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type=False
    data=options.static_mesh_import_data
    data.combine_meshes=True;data.auto_generate_collision=False;data.generate_lightmap_u_vs=False
    data.import_mesh_lods=False;data.convert_scene=True;data.convert_scene_unit=True
    data.force_front_x_axis=True;data.import_uniform_scale=1.
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    data.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    task.options=options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=unreal.load_asset(DEST+'/'+NAME)
    assert isinstance(mesh,unreal.StaticMesh)
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mesh.set_material(0,material)
    assert editor.set_lod_screen_sizes(mesh,[1.])
    for lod in (1,2):
        assert editor.import_lod(mesh,lod,str(ROOT/rows[lod]['fbx']))==lod
    editor.remove_collisions(mesh)
    assert editor.set_lod_screen_sizes(mesh,[1.,.16,.04])
    actual=[mesh.get_num_triangles(i) for i in range(3)]
    assert actual==[r['triangles'] for r in rows]
    assert [mesh.get_num_sections(i) for i in range(3)]==[1,1,1]
    assert editor.get_simple_collision_count(mesh)==0
    bounds=mesh.get_bounding_box()
    measured=[[bounds.min.x/100,bounds.min.y/100,bounds.min.z/100],
              [bounds.max.x/100,bounds.max.y/100,bounds.max.z/100]]
    error=max(abs(measured[i][j]-rows[0]['bounds_m'][i][j]) for i in range(2) for j in range(3))
    assert error < .001
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    return dict(asset=mesh.get_path_name(),triangles=actual,boundsMetres=measured,
        material=material.get_path_name(),collision=False,sourceReceiptSha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest())
