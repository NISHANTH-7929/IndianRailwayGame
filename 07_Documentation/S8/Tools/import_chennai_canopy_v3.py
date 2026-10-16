"""Root-coordinated Unreal importer. Run once before desktop acceptance."""
import unreal, importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'));b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
dest='/Game/IndianRailway/S8/Chennai';name='SM_ChennaiDenseCanopy'
t=unreal.AssetImportTask();t.filename=str(b.ROOT/'04_Assets/S8/ChennaiFoliageV3'/f'{name}.fbx');t.destination_path=dest;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
d=o.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE;t.options=o;b.ASSETS.import_asset_tasks([t])
mesh=unreal.load_asset(dest+'/'+name);assert mesh
m=unreal.load_asset(dest+'/M_ChennaiDenseLeaf')
if m is None:m=b.ASSETS.create_asset('M_ChennaiDenseLeaf',dest,unreal.Material,unreal.MaterialFactoryNew())
b.MAT.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
c=b.expression(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.10,.25,.035,1))
# Stable albedo does not rely on vertex data surviving static-mesh reduction.
noise=b.expression(m,unreal.MaterialExpressionNoise,scale=.025,quality=1,levels=1,output_min=.72,output_max=1.)
mix=b.expression(m,unreal.MaterialExpressionMultiply);b.MAT.connect_material_expressions(c,'',mix,'A');b.MAT.connect_material_expressions(noise,'',mix,'B');b.connect(mix,m,unreal.MaterialProperty.MP_BASE_COLOR)
b.connect(b.expression(m,unreal.MaterialExpressionConstant,r=.86),m,unreal.MaterialProperty.MP_ROUGHNESS)
b.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);b.MAT.recompile_material(m);mesh.set_material(0,m)
red=unreal.EditorScriptingMeshReductionOptions();red.auto_compute_lod_screen_size=False;red.reduction_settings=[unreal.EditorScriptingMeshReductionSettings(percent_triangles=1,screen_size=1),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.65,screen_size=.02),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.35,screen_size=.006)];b.EDIT.set_lods(mesh,red)
unreal.EditorAssetLibrary.save_loaded_asset(m);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
water=unreal.load_asset(dest+'/M_ChennaiRiverSurface')
if water is None:water=b.ASSETS.create_asset('M_ChennaiRiverSurface',dest,unreal.Material,unreal.MaterialFactoryNew())
b.MAT.delete_all_material_expressions(water)
low=b.expression(water,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.025,.052,.042,1))
high=b.expression(water,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.065,.11,.083,1))
ripple=b.expression(water,unreal.MaterialExpressionNoise,scale=.005,quality=1,levels=2,output_min=0.,output_max=1.)
shade=b.expression(water,unreal.MaterialExpressionLinearInterpolate)
b.MAT.connect_material_expressions(low,'',shade,'A');b.MAT.connect_material_expressions(high,'',shade,'B');b.MAT.connect_material_expressions(ripple,'',shade,'Alpha');b.connect(shade,water,unreal.MaterialProperty.MP_BASE_COLOR)
b.connect(b.expression(water,unreal.MaterialExpressionConstant,r=.24),water,unreal.MaterialProperty.MP_ROUGHNESS)
b.connect(b.expression(water,unreal.MaterialExpressionConstant,r=.65),water,unreal.MaterialProperty.MP_SPECULAR)
b.MAT.set_material_usage(water,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);b.MAT.recompile_material(water);unreal.EditorAssetLibrary.save_loaded_asset(water)
ticks=[0]
def finish(dt):
 ticks[0]+=1
 if ticks[0]>60:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
