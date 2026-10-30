"""Apply findings from the first in-engine review to this S9 revision only."""
from pathlib import Path
import hashlib
import json
import runpy
import time
import unreal

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S9/Evidence/TrackEnvironment-20261025-01'
assert not (OUT/'visual-refinement.json').exists()
E=unreal.MaterialEditingLibrary
A=unreal.AssetToolsHelpers.get_asset_tools()
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
BASE='/Game/IndianRailway/S9/TrackEnvironmentV1/'
FINISH=BASE+'MaterialsV2/'

def node(m,kind,**props):
    n=E.create_material_expression(m,kind)
    for key,value in props.items():n.set_editor_property(key,value)
    return n
def link(a,b,pin=''):assert E.connect_material_expressions(a,'',b,pin)
def nodes(m,root):
    seen=set();stack=[root]
    while stack:
        n=stack.pop()
        if not n or n.get_path_name() in seen:continue
        seen.add(n.get_path_name());yield n
        stack.extend(E.get_inputs_for_material_expression(m,n))

try:
    refinements=[]
    for name in ('M_TrackBallast','M_TrackStone','M_DryGround','M_SleeperConcrete','M_RailHead','M_RailRust','M_Fastener'):
        assert not unreal.EditorAssetLibrary.does_asset_exist(FINISH+name), 'Fresh derivative required'
    for name in ('M_TrackBallast','M_TrackStone','M_DryGround','M_SleeperConcrete','M_RailHead','M_RailRust','M_Fastener'):
        derivative=unreal.EditorAssetLibrary.duplicate_asset(BASE+'Materials/'+name,FINISH+name)
        assert derivative and unreal.EditorAssetLibrary.save_loaded_asset(derivative)
    for name in ('M_TrackBallast','M_TrackStone','M_DryGround'):
        m=unreal.load_asset(FINISH+name)
        base=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
        changed=0
        for n in nodes(m,base):
            if isinstance(n,unreal.MaterialExpressionConstant):
                old=n.get_editor_property('r')
                value=.30 if abs(old-.78)<1e-5 else .50 if abs(old-.90)<1e-5 else None
                if value is not None:n.set_editor_property('r',value);changed+=1
        assert any(isinstance(n,unreal.MaterialExpressionConstant) and abs(n.get_editor_property('r')-(.30 if name=='M_DryGround' else .50))<1e-5 for n in nodes(m,base)),name
        E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        refinements.append(dict(material=name,albedoMultipliersChanged=changed))
    m=unreal.load_asset(FINISH+'M_SleeperConcrete')
    base=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
    colors=[n for n in nodes(m,base) if isinstance(n,unreal.MaterialExpressionConstant3Vector)]
    assert len(colors)==1
    colors[0].set_editor_property('constant',unreal.LinearColor(.18,.175,.15,1))
    # World-XY grain would stretch down vertical faces. Keep it on top faces;
    # vertical concrete uses the shared body colour, without stretched stripes.
    normal=node(m,unreal.MaterialExpressionVertexNormalWS)
    z=node(m,unreal.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False);link(normal,z)
    absolute=node(m,unreal.MaterialExpressionAbs);link(z,absolute)
    blend=node(m,unreal.MaterialExpressionLinearInterpolate)
    link(colors[0],blend,'A');link(base,blend,'B');link(absolute,blend,'Alpha')
    assert E.connect_material_property(blend,'',unreal.MaterialProperty.MP_BASE_COLOR)
    E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    refinements.append(dict(material='M_SleeperConcrete',concreteTopGrainOnly=True))

    # Import an explicit rail-preserving hierarchy, independent of the old
    # decimated station module. Original station assets remain unchanged.
    source=ROOT/'04_Assets/S9/TrackEnvironmentV1/Rails/rails-receipt.json'
    records=json.loads(source.read_text())['records']
    name='SM_Track6m_RealismV1';path=BASE+'Meshes/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(path)
    for row in records:
        assert hashlib.sha256((ROOT/row['fbx']).read_bytes()).hexdigest()==row['sha256']
    task=unreal.AssetImportTask();task.filename=str(ROOT/records[0]['fbx'])
    task.destination_path=BASE+'Meshes';task.destination_name=name;task.automated=True;task.save=False
    options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False
    options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type=False
    data=options.static_mesh_import_data;data.combine_meshes=True;data.auto_generate_collision=False
    data.generate_lightmap_u_vs=False;data.import_mesh_lo_ds=False;data.convert_scene=True
    data.convert_scene_unit=True;data.force_front_x_axis=True;data.import_uniform_scale=1.
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options=options;A.import_asset_tasks([task]);mesh=unreal.load_asset(path)
    assert isinstance(mesh,unreal.StaticMesh)
    mapping={'RailHead':'M_RailHead','RailRust':'M_RailRust','Concrete':'M_SleeperConcrete'}
    slots=mesh.get_editor_property('static_materials');assert len(slots)==3
    for index,slot in enumerate(slots):
        key=str(slot.get_editor_property('imported_material_slot_name'))
        material=unreal.load_asset(FINISH+mapping[key]);assert material
        mesh.set_material(index,material)
    assert S.set_lod_screen_sizes(mesh,[1.])
    for level in (1,2):assert S.import_lod(mesh,level,str(ROOT/records[level]['fbx']))==level
    S.remove_collisions(mesh);assert S.set_lod_screen_sizes(mesh,[1.,.18,.04])
    counts=[mesh.get_num_triangles(i) for i in range(3)]
    assert counts==[992,192,132],counts
    assert all(mesh.get_num_sections(i)==3 for i in range(3))
    bounds=mesh.get_bounding_box()
    assert abs(bounds.min.x)<.01 and abs(bounds.max.x-600)<.01
    assert abs(bounds.min.y+137.5)<.01 and abs(bounds.max.y-137.5)<.01
    assert abs(bounds.min.z+33.5)<.01 and abs(bounds.max.z)<.01
    assert S.get_simple_collision_count(mesh)==0
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    runpy.run_path(str(Path(__file__).with_name('import_track_environment_meshes.py')),run_name='__main__')
    (OUT/'visual-refinement.json').write_text(json.dumps(dict(passed=True,materials=refinements,
        railMesh=path,railTriangles=counts,railSectionsAllLODs=3,railHeadWebFootPreserved=True,
        lodScreenSizes=list(S.get_lod_screen_sizes(mesh)),originalStationAssetsChanged=False),indent=2))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
