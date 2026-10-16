import unreal,importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'));b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
dest='/Game/IndianRailway/S8/Chennai';name='SM_ChennaiLeafCluster'
t=unreal.AssetImportTask();t.filename=str(b.ROOT/'04_Assets/S8/ChennaiFoliageV2'/f'{name}.fbx');t.destination_path=dest;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
d=o.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE;t.options=o;b.ASSETS.import_asset_tasks([t])
mesh=unreal.load_asset(dest+'/'+name);assert mesh
m=b.ASSETS.create_asset('M_ChennaiLeafSurface',dest,unreal.Material,unreal.MaterialFactoryNew()) if not unreal.EditorAssetLibrary.does_asset_exist(dest+'/M_ChennaiLeafSurface') else unreal.load_asset(dest+'/M_ChennaiLeafSurface')
b.MAT.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
v=b.expression(m,unreal.MaterialExpressionVertexColor);c=b.expression(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.08,.20,.025,1))
# Retain green albedo even if an imported/reduced leaf loses vertex colour.
# Multiplying raw zero vertex RGB made those leaves black/grey in native views.
variation=b.expression(m,unreal.MaterialExpressionLinearInterpolate,const_a=.6,const_b=1.)
b.MAT.connect_material_expressions(v,'R',variation,'Alpha')
mul=b.expression(m,unreal.MaterialExpressionMultiply);b.MAT.connect_material_expressions(variation,'',mul,'A');b.MAT.connect_material_expressions(c,'',mul,'B');b.connect(mul,m,unreal.MaterialProperty.MP_BASE_COLOR)
b.connect(b.expression(m,unreal.MaterialExpressionConstant,r=.8),m,unreal.MaterialProperty.MP_ROUGHNESS);b.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);b.MAT.recompile_material(m);mesh.set_material(0,m)
red=unreal.EditorScriptingMeshReductionOptions();red.auto_compute_lod_screen_size=False;red.reduction_settings=[unreal.EditorScriptingMeshReductionSettings(percent_triangles=1,screen_size=1),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.8,screen_size=.025),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.6,screen_size=.008)];b.EDIT.set_lods(mesh,red)
unreal.EditorAssetLibrary.save_loaded_asset(m);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
ground=unreal.load_asset(dest+'/M_ChennaiGroundSurface')
if ground is None:ground=b.ASSETS.create_asset('M_ChennaiGroundSurface',dest,unreal.Material,unreal.MaterialFactoryNew())
b.MAT.delete_all_material_expressions(ground)
color=b.expression(ground,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.15,.105,.055,1))
noise=b.expression(ground,unreal.MaterialExpressionNoise,scale=.035,quality=1,levels=2,output_min=.45,output_max=1.)
mix=b.expression(ground,unreal.MaterialExpressionMultiply);b.MAT.connect_material_expressions(color,'',mix,'A');b.MAT.connect_material_expressions(noise,'',mix,'B');b.connect(mix,ground,unreal.MaterialProperty.MP_BASE_COLOR)
b.connect(b.expression(ground,unreal.MaterialExpressionConstant,r=.95),ground,unreal.MaterialProperty.MP_ROUGHNESS);b.MAT.set_material_usage(ground,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);b.MAT.recompile_material(ground);unreal.EditorAssetLibrary.save_loaded_asset(ground)
# Defer closing until the level editor has completed startup.
ticks=[0]
def finish(dt):
 ticks[0]+=1
 if ticks[0]>60:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
