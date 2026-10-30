"""Import only the denser all-angle leaf derivative. Root launches full editor."""
import hashlib
import json
import time
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[3]
BASE='/Game/IndianRailway/S9/CorridorNatureV1'
SOURCE=ROOT/'04_Assets/S9/CorridorNatureV1/MeshesV2'
OUT=SOURCE/'unreal-import-receipt.json'
NAME='SM_CorridorLeafClusterV2'
A=unreal.AssetToolsHelpers.get_asset_tools()
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S
try:
    assert not OUT.exists(), 'Preserve earlier import receipt'
    assert not unreal.EditorAssetLibrary.does_asset_exist(BASE+'/Meshes/'+NAME), 'Fresh V2 target required'
    receipt=SOURCE/'mesh-receipt.json'
    source=json.loads(receipt.read_text())
    assert source['revision']=='corridor-nature-v2'
    rows=source['records']
    assert [r['triangles'] for r in rows]==[664,240,96]
    for row in rows:
        assert hashlib.sha256((ROOT/row['fbx']).read_bytes()).hexdigest()==row['sha256']
    material=unreal.load_asset(BASE+'/Materials/M_CorridorLeaf')
    assert isinstance(material,unreal.Material)
    task=unreal.AssetImportTask()
    task.filename=str(ROOT/rows[0]['fbx'])
    task.destination_path=BASE+'/Meshes'
    task.destination_name=NAME
    task.automated=True
    task.replace_existing=False
    task.save=False
    options=unreal.FbxImportUI()
    options.import_mesh=True
    options.import_materials=False
    options.import_textures=False
    options.import_as_skeletal=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type=False
    data=options.static_mesh_import_data
    data.combine_meshes=True
    data.auto_generate_collision=False
    data.generate_lightmap_u_vs=False
    data.import_mesh_lods=False
    data.convert_scene=True
    data.convert_scene_unit=True
    data.force_front_x_axis=True
    data.import_uniform_scale=1.0
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    data.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    task.options=options
    A.import_asset_tasks([task])
    mesh=unreal.load_asset(BASE+'/Meshes/'+NAME)
    assert isinstance(mesh,unreal.StaticMesh)
    assert len(mesh.get_editor_property('static_materials'))==1
    mesh.set_material(0,material)
    assert S.set_lod_screen_sizes(mesh,[1.0])
    for level in (1,2):
        assert S.import_lod(mesh,level,str(ROOT/rows[level]['fbx']))==level
    S.remove_collisions(mesh)
    thresholds=[1.0,.025,.008]
    assert S.set_lod_screen_sizes(mesh,thresholds)
    count=S.get_lod_count(mesh)
    bounds=mesh.get_bounding_box()
    measured=[[bounds.min.x/100,bounds.min.y/100,bounds.min.z/100],
              [bounds.max.x/100,bounds.max.y/100,bounds.max.z/100]]
    triangles=[mesh.get_num_triangles(i) for i in range(count)]
    sections=[mesh.get_num_sections(i) for i in range(count)]
    uv_channels=[S.get_num_uv_channels(mesh,i) for i in range(count)]
    error=max(abs(measured[i][j]-rows[0]['boundsMetres'][i][j]) for i in range(2) for j in range(3))
    assert count==3 and triangles==[664,240,96]
    assert sections==[1,1,1] and uv_channels==[1,1,1]
    assert error<.001 and S.get_simple_collision_count(mesh)==0
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    for path,expected in source['protectedV1Sha256'].items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==expected
    OUT.write_text(json.dumps(dict(passed=True,asset=mesh.get_path_name(),triangles=triangles,
        sections=sections,uvChannels=uv_channels,boundsMetres=measured,importedBoundsErrorMetres=error,
        screenSizes=list(S.get_lod_screen_sizes(mesh)),material=mesh.get_material(0).get_path_name(),
        sourceReceiptSha256=hashlib.sha256(receipt.read_bytes()).hexdigest(),
        protectedV1Sha256=source['protectedV1Sha256'],collisionShapes=0,mobileTested=False),indent=2))
    unreal.log('CORRIDOR_CANOPY_V2_IMPORTED '+str(OUT))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:
            return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
