"""Full-editor import for original bounded vegetation; root coordinates execution.

Creates only /Game/IndianRailway/S9/CorridorNatureV1. Save every new material and
mesh explicitly; verify imported LODs, bounds, colours and collision before exit.
"""
import hashlib
import json
import time
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/'04_Assets/S9/CorridorNatureV1'
BASE='/Game/IndianRailway/S9/CorridorNatureV1'
OUT=SOURCE/'unreal-import-receipt.json'
A=unreal.AssetToolsHelpers.get_asset_tools()
E=unreal.MaterialEditingLibrary
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S, 'Full editor required'


def expr(material, kind, **properties):
    node=E.create_material_expression(material,kind)
    for key,value in properties.items():
        node.set_editor_property(key,value)
    return node


def material(name,color,foliage):
    path=BASE+'/Materials/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(path), 'Fresh nature material required: '+path
    m=A.create_asset(name,BASE+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    assert m
    m.set_editor_property('two_sided',foliage)
    if foliage:
        m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    color_node=expr(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',
                    default_value=unreal.LinearColor(*color,1))
    vertices=expr(m,unreal.MaterialExpressionVertexColor)
    shade=expr(m,unreal.MaterialExpressionMultiply)
    E.connect_material_expressions(color_node,'',shade,'A')
    E.connect_material_expressions(vertices,'RGB',shade,'B')
    E.connect_material_property(shade,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=expr(m,unreal.MaterialExpressionConstant,r=.87 if foliage else .94)
    specular=expr(m,unreal.MaterialExpressionConstant,r=.18)
    E.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    E.connect_material_property(specular,'',unreal.MaterialProperty.MP_SPECULAR)
    if foliage:
        # Modest transmitted light avoids opaque black undersides. No opacity
        # mask/texture sampling, vertex animation or translucent overdraw.
        transmitted=expr(m,unreal.MaterialExpressionMultiply)
        amount=expr(m,unreal.MaterialExpressionConstant,r=.45)
        E.connect_material_expressions(shade,'',transmitted,'A')
        E.connect_material_expressions(amount,'',transmitted,'B')
        E.connect_material_property(transmitted,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        E.connect_material_property(amount,'',unreal.MaterialProperty.MP_OPACITY)
    E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    E.recompile_material(m)
    assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


try:
    assert not OUT.exists(), 'Preserve earlier import receipt'
    receipt=SOURCE/'mesh-receipt.json'
    source=json.loads(receipt.read_text())
    assert source['revision']=='corridor-nature-v1'
    for row in source['records']:
        assert hashlib.sha256((ROOT/row['fbx']).read_bytes()).hexdigest()==row['sha256']
        assert not unreal.EditorAssetLibrary.does_asset_exist(BASE+'/Meshes/'+row['name'])
    mats={name:material(name,color,foliage) for name,color,foliage in [
        ('M_CorridorLeaf',(.085,.14,.035),True),
        ('M_CorridorDryGrass',(.22,.235,.085),True),
        ('M_CorridorBark',(.10,.075,.046),False)]}
    results=[]
    for family in ('foliage','grass','branch'):
        rows=sorted((r for r in source['records'] if r['family']==family),key=lambda r:r['lod'])
        assert [r['lod'] for r in rows]==[0,1,2]
        name=rows[0]['name']
        task=unreal.AssetImportTask()
        task.filename=str(ROOT/rows[0]['fbx'])
        task.destination_path=BASE+'/Meshes'
        task.destination_name=name
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
        mesh=unreal.load_asset(BASE+'/Meshes/'+name)
        assert isinstance(mesh,unreal.StaticMesh)
        assert len(mesh.get_editor_property('static_materials'))==1
        mesh.set_material(0,mats[rows[0]['material']])
        assert S.set_lod_screen_sizes(mesh,[1.0])
        for level in (1,2):
            assert S.import_lod(mesh,level,str(ROOT/rows[level]['fbx']))==level
        S.remove_collisions(mesh)
        thresholds=[1.0,.025,.008] if family=='foliage' else [1.0,.035,.012]
        assert S.set_lod_screen_sizes(mesh,thresholds)
        count=S.get_lod_count(mesh)
        bounds=mesh.get_bounding_box()
        measured=[[bounds.min.x/100,bounds.min.y/100,bounds.min.z/100],
                  [bounds.max.x/100,bounds.max.y/100,bounds.max.z/100]]
        triangles=[mesh.get_num_triangles(i) for i in range(count)]
        sections=[mesh.get_num_sections(i) for i in range(count)]
        uv_channels=[S.get_num_uv_channels(mesh,i) for i in range(count)]
        error=max(abs(measured[i][j]-rows[0]['boundsMetres'][i][j]) for i in range(2) for j in range(3))
        assert count==3 and triangles==[r['triangles'] for r in rows]
        assert sections==[1,1,1] and uv_channels==[1,1,1]
        assert error<.001 and S.get_simple_collision_count(mesh)==0
        actual_thresholds=list(S.get_lod_screen_sizes(mesh))
        assert all(abs(a-b)<1e-6 for a,b in zip(actual_thresholds,thresholds))
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        results.append(dict(family=family,asset=mesh.get_path_name(),triangles=triangles,
            lodCount=count,sections=sections,uvChannels=uv_channels,boundsMetres=measured,
            importedBoundsErrorMetres=error,screenSizes=actual_thresholds,
            material=mesh.get_material(0).get_path_name(),collisionShapes=0,
            vertexColorImportOption=str(data.vertex_color_import_option)))
    OUT.write_text(json.dumps(dict(passed=True,meshes=results,
        materials=[m.get_path_name() for m in mats.values()],
        sourceReceiptSha256=hashlib.sha256(receipt.read_bytes()).hexdigest(),
        sourceFbxSha256=[r['sha256'] for r in source['records']],
        generatedImages=False,originalAssetsModified=False,mobileTested=False),indent=2))
    unreal.log('CORRIDOR_NATURE_V1_READY '+str(OUT))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:
            return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
