"""Import versioned running gear, preserving existing assets and assembly JSON."""
import unreal,json,importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'));base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
src=base.ROOT/'04_Assets/S8/RunningGearV2';dest='/Game/IndianRailway/S8/RunningGearV2'
materials={}
for name,color,metal,rough in [('WheelWeb',[.12,.105,.09,1],.65,.5),('WheelTread',[.46,.48,.5,1],.95,.24),('WheelHub',[.2,.21,.22,1],.8,.34),('Witness',[.65,.60,.45,1],.1,.7)]:
 base.DEST='/Game/IndianRailway/S8';m=base.material_for('RunningGearV2',dict(name=name,base_color=color,metallic=metal,roughness=rough));base.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);unreal.EditorAssetLibrary.save_loaded_asset(m);materials[name]=m
rows=[]
for row in json.loads((src/'manifest.json').read_text())['parts']:
 name=row['name'];task=unreal.AssetImportTask();task.filename=str(src/(name+'.fbx'));task.destination_path=dest;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
 options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
 data=options.static_mesh_import_data;data.combine_meshes=True;data.auto_generate_collision=False;data.generate_lightmap_u_vs=False;data.convert_scene=True;data.convert_scene_unit=True;data.force_front_x_axis=True;data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;task.options=options
 base.ASSETS.import_asset_tasks([task]);mesh=unreal.load_asset(dest+'/'+name);assert mesh
 original=None
 if 'Frame' in name:original=unreal.load_asset('/Game/IndianRailway/S8/Locomotives/WAP7/Selected/Meshes/SM_WAP7_Selected_Bogie_'+('Front' if 'Front' in name else 'Rear')+'_LOD0')
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  m=original.get_material(i) if original else materials[str(slot.get_editor_property('imported_material_slot_name'))]
  mesh.set_material(i,m)
 reduction=unreal.EditorScriptingMeshReductionOptions();reduction.auto_compute_lod_screen_size=False;reduction.reduction_settings=[unreal.EditorScriptingMeshReductionSettings(percent_triangles=1.,screen_size=1.),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.45,screen_size=.3),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.16,screen_size=.1)]
 base.EDIT.set_lods(mesh,reduction);unreal.EditorAssetLibrary.save_loaded_asset(mesh);rows.append(dict(asset=mesh.get_path_name(),lods=base.EDIT.get_lod_count(mesh)))
(base.ROOT/'07_Documentation/S8/Evidence/EgmoreTambaram/running-gear-import.json').write_text(json.dumps(rows,indent=2))
exec(compile(Path(__file__).with_name('import_chennai_nameboards.py').read_text(),str(Path(__file__).with_name('import_chennai_nameboards.py')),'exec'),{'__file__':str(Path(__file__).with_name('import_chennai_nameboards.py'))})
unreal.SystemLibrary.quit_editor()
