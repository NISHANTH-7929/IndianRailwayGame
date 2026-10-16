"""Import separate Egmore Scene02 family; never touch fixtures or old station assets."""
import unreal,json,importlib.util,os,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
INTERIOR=os.environ.get('RAIL_EGMORE_INTERIOR')=='1'
family='EgmoreInterior01' if INTERIOR else 'EgmoreScene02'
SOURCE=ROOT/'04_Assets/S8'/family/('2.1.0' if INTERIOR else '2.0.0')
OUT=ROOT/'07_Documentation/S8/Evidence'/family
spec=importlib.util.spec_from_file_location('pbr',Path(__file__).with_name('gltf_material_import.py'));pbr=importlib.util.module_from_spec(spec);spec.loader.exec_module(pbr)
base=pbr.base;base.DEST='/Game/IndianRailway/S8/Stations';pbr.SOURCE=SOURCE
manifest=json.loads((SOURCE/'manifest.json').read_text())
materials={}
for d in json.loads((SOURCE/'materials.json').read_text()):
    mat=pbr.material_for(family,d);base.MAT.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    if 'occlusionTexture' in d:
        sample=pbr.texture(mat,family,d['occlusionTexture'],'mask');base.connect(sample,mat,unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,'R')
    base.MAT.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);materials[base.name(d['name'])]=mat
report=[]
for module in manifest['modules']:
    name=module['module'];task=unreal.AssetImportTask();task.filename=str(SOURCE/'Parts'/(name+'_LOD0.fbx'));task.destination_path=f'{base.DEST}/{family}/Meshes';task.destination_name='SM_'+name;task.automated=True;task.replace_existing=True;task.save=True
    options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
    data=options.static_mesh_import_data;data.combine_meshes=True;data.auto_generate_collision=False;data.generate_lightmap_u_vs=False;data.import_mesh_lo_ds=False;data.convert_scene=True;data.convert_scene_unit=True;data.force_front_x_axis=True;data.import_uniform_scale=1;data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options=options;base.ASSETS.import_asset_tasks([task]);mesh=unreal.load_asset(task.destination_path+'/'+task.destination_name)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Mesh import failed '+name)
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=base.name(str(slot.get_editor_property('imported_material_slot_name')))
        if key not in materials:raise RuntimeError('Unmapped material '+key+' on '+name)
        mesh.set_material(index,materials[key])
    bounds=mesh.get_bounding_box();actual=[[bounds.min.x/100,bounds.min.y/100,bounds.min.z/100],[bounds.max.x/100,bounds.max.y/100,bounds.max.z/100]]
    error=max(abs(actual[i][j]-module['boundsM'][i][j]) for i in range(2) for j in range(3))
    if error>.001:raise RuntimeError('Metric basis mismatch '+name+': '+str(actual)+' expected '+str(module['boundsM']))
    for level in [1,2]:
        result=base.EDIT.import_lod(mesh,level,str(SOURCE/'Parts'/f'{name}_LOD{level}.fbx'))
        if result!=level:raise RuntimeError('Authored LOD import failed '+name+' '+str(result))
    if base.EDIT.get_lod_count(mesh)!=3:raise RuntimeError('Missing LODs '+name)
    actual_triangles=[mesh.get_num_triangles(i) for i in range(3)]
    if actual_triangles!=[lod['triangles'] for lod in module['lods']]:raise RuntimeError('Imported topology differs '+name+' '+str(actual_triangles))
    base.EDIT.set_lod_screen_sizes(mesh,[1.0,.2,.06])
    base.EDIT.remove_collisions(mesh);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    report.append(dict(module=name,assetId=module['assetId'],asset=mesh.get_path_name(),boundsM=actual,boundsErrorM=error,lodTriangles=actual_triangles,materials=mesh.get_num_sections(0)))
    (OUT/'unreal-import-progress.json').write_text(json.dumps(report,indent=2))
data=dict(manifest);data['importedModules']=report;data['runtimeAccepted']=False
path=base.CONTENT/'IndianRailway/Production/stations'/(family+'.json');path.write_text(json.dumps(data,indent=2))
(OUT/'unreal-import.json').write_text(json.dumps(dict(passed=True,modules=report,materialCount=len(materials),textureBindings=len(pbr.textures),runtime=str(path),simulationChanged=False),indent=2))
unreal.log('EGMORE_SCENE02_IMPORTED')
# Quitting during startup can race the editor mode-manager initialization.
finished=time.monotonic()
def finish_tick(dt):
    if time.monotonic()-finished<10:return
    unreal.unregister_slate_post_tick_callback(finish_handle)
    unreal.SystemLibrary.quit_editor()
finish_handle=unreal.register_slate_post_tick_callback(finish_tick)
