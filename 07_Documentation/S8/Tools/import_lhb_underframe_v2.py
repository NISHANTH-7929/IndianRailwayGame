"""Versioned in-project derivative import; retains old coach assets and saves."""
import unreal,json,importlib.util,os
from pathlib import Path
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'));b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
src=b.ROOT/'04_Assets/S8/LHBUnderframeV2';dest='/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2'
materials={}
original=unreal.load_asset('/Game/IndianRailway/S8/Coaches/LHB3A/Meshes/SM_LHB3A_Body_LOD0')
for i,s in enumerate(original.get_editor_property('static_materials')):materials[b.name(str(s.get_editor_property('imported_material_slot_name')))]=original.get_material(i)
definitions=[('FrameSteel',[.19,.20,.19,1],.48,.64),('DarkSteel',[.055,.060,.055,1],.55,.60),('DustSteel',[.23,.21,.17,1],.25,.78),('TankIvory',[.46,.44,.37,1],.10,.74),('Rubber',[.023,.025,.022,1],.0,.83),('PistonSteel',[.43,.45,.46,1],.9,.23),('WeldSteel',[.16,.17,.16,1],.6,.55),('LHB_GlassV2',[.045,.13,.15,1],.0,.19),('LHB_PaintRedV2',[.48,.007,.020,1],.0,.38),('LHB_PaintGreyV2',[.43,.46,.46,1],.0,.44)]
# Existing underframe materials are reused and refined in this derivative
# namespace. Original body paints use constant colour/metallic (red was metal=1)
# and cannot express the baked local wear, so these two new parents are needed.
for n,c,metal,rough in definitions:
 b.DEST='/Game/IndianRailway/S8/Coaches/LHB3A';m=b.material_for('UnderframeV2',dict(name=n,base_color=c,metallic=metal,roughness=rough,vertex_color=True))
 # Replace constants with adjustable useful controls. The baked RGB multipliers
 # give restrained dirt and surface variation that follows the moving mesh.
 b.MAT.delete_all_material_expressions(m)
 m.set_editor_property('use_material_attributes',False)
 if 'Glass' in n:m.set_editor_property('two_sided',True)
 tint=b.expression(m,unreal.MaterialExpressionVectorParameter,parameter_name='SurfaceTint',default_value=unreal.LinearColor(*c))
 vertex=b.expression(m,unreal.MaterialExpressionVertexColor);multiply=b.expression(m,unreal.MaterialExpressionMultiply)
 b.MAT.connect_material_expressions(tint,'RGB',multiply,'A');b.MAT.connect_material_expressions(vertex,'RGB',multiply,'B')
 surface_color=multiply
 if 'Glass' in n:
  # Subtle view-angle coating response. This remains an explicitly opaque
  # exterior approximation: no fabricated transparent interior is implied.
  fresnel=b.expression(m,unreal.MaterialExpressionFresnel)
  edge=b.expression(m,unreal.MaterialExpressionMultiply)
  strength=b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='GlazingEdgeTint',default_value=.20)
  assert b.MAT.connect_material_expressions(fresnel,'',edge,'A')
  assert b.MAT.connect_material_expressions(strength,'',edge,'B')
  plus=b.expression(m,unreal.MaterialExpressionAdd,const_b=1.0)
  assert b.MAT.connect_material_expressions(edge,'',plus,'A')
  surface_color=b.expression(m,unreal.MaterialExpressionMultiply)
  assert b.MAT.connect_material_expressions(multiply,'',surface_color,'A')
  assert b.MAT.connect_material_expressions(plus,'',surface_color,'B')
 b.connect(surface_color,m,unreal.MaterialProperty.MP_BASE_COLOR)
 parameters={}
 for field,prop,val in [('Metallic',unreal.MaterialProperty.MP_METALLIC,metal),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS,rough)]:
  parameter=b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name=field,default_value=val)
  if field=='Roughness' and 'Glass' not in n:
   # UV-space micro variation follows moving meshes and stays restrained.
   uv=b.expression(m,unreal.MaterialExpressionTextureCoordinate);zero=b.expression(m,unreal.MaterialExpressionConstant,r=0.0);append=b.expression(m,unreal.MaterialExpressionAppendVector)
   b.MAT.connect_material_expressions(uv,'',append,'A');b.MAT.connect_material_expressions(zero,'',append,'B')
   noise=b.expression(m,unreal.MaterialExpressionNoise,scale=42.0,quality=1,levels=1)
   b.MAT.connect_material_expressions(append,'',noise,'Position');gain=b.expression(m,unreal.MaterialExpressionMultiply);strength=b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='RoughnessVariation',default_value=.035)
   b.MAT.connect_material_expressions(noise,'',gain,'A');b.MAT.connect_material_expressions(strength,'',gain,'B');add=b.expression(m,unreal.MaterialExpressionAdd)
   b.MAT.connect_material_expressions(parameter,'',add,'A');b.MAT.connect_material_expressions(gain,'',add,'B');parameter=add
   if n not in ('Rubber','PistonSteel'):
    # Dirt-darkened vertex regions become rougher as well as darker. This keeps
    # the underframe from looking like uniformly polished plastic. Piston rods
    # retain their clean sliding surfaces and rubber stays non-metallic/matte.
    inverse=b.expression(m,unreal.MaterialExpressionOneMinus)
    assert b.MAT.connect_material_expressions(vertex,'R',inverse,''),'Dirt inversion input'
    dirt=b.expression(m,unreal.MaterialExpressionMultiply)
    amount=b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='DirtRoughnessStrength',default_value=.32)
    b.MAT.connect_material_expressions(inverse,'',dirt,'A');b.MAT.connect_material_expressions(amount,'',dirt,'B')
    worn=b.expression(m,unreal.MaterialExpressionAdd)
    b.MAT.connect_material_expressions(parameter,'',worn,'A');b.MAT.connect_material_expressions(dirt,'',worn,'B')
    bounded=b.expression(m,unreal.MaterialExpressionClamp,min_default=.04,max_default=.98)
    assert b.MAT.connect_material_expressions(worn,'',bounded,''),'Dirt clamp input'
    parameter=bounded
  b.connect(parameter,m,prop)
  parameters[field]=parameter
 specular=b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='Specular',default_value=.5)
 b.connect(specular,m,unreal.MaterialProperty.MP_SPECULAR)
 if 'Paint' in n or 'Glass' in n:
  m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_CLEAR_COAT)
  # Hidden CustomData enum entries aren't exposed by this UE Python build.
  # MakeMaterialAttributes exposes the named coat pins without hidden enums.
  attrs=b.expression(m,unreal.MaterialExpressionMakeMaterialAttributes)
  for node,pin in [(surface_color,'BaseColor'),(parameters['Metallic'],'Metallic'),(parameters['Roughness'],'Roughness'),(specular,'Specular'),(b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='ClearCoat',default_value=.5 if 'Paint' in n else 1.0),'ClearCoat'),(b.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='CoatRoughness',default_value=.25 if 'Paint' in n else .09),'ClearCoatRoughness')]:
   assert b.MAT.connect_material_expressions(node,'',attrs,pin),pin
  m.set_editor_property('use_material_attributes',True)
  assert b.MAT.connect_material_property(attrs,'',unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
 b.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);b.MAT.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials[n]=m
materials['Material_004']=materials['LHB_PaintRedV2'];materials['Material_006']=materials['LHB_PaintGreyV2']
rows=[]
for row in json.loads((src/'manifest.json').read_text())['parts']:
 name=row['name'];t=unreal.AssetImportTask();t.filename=str(src/(name+'.fbx'));t.destination_path=dest;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
 o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=False;d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;d.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE;t.options=o;b.ASSETS.import_asset_tasks([t])
 mesh=unreal.load_asset(dest+'/'+name);assert mesh
 for i,s in enumerate(mesh.get_editor_property('static_materials')):
  slot=b.name(str(s.get_editor_property('imported_material_slot_name')))
  if slot in materials:mesh.set_material(i,materials[slot])
  elif 'Body' in name and i<len(original.get_editor_property('static_materials')):mesh.set_material(i,original.get_material(i))
  else:raise RuntimeError('Unmapped material '+slot)
 red=unreal.EditorScriptingMeshReductionOptions();red.auto_compute_lod_screen_size=False;red.reduction_settings=[unreal.EditorScriptingMeshReductionSettings(percent_triangles=1,screen_size=1),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.5,screen_size=.3),unreal.EditorScriptingMeshReductionSettings(percent_triangles=.18,screen_size=.10)];b.EDIT.set_lods(mesh,red);unreal.EditorAssetLibrary.save_loaded_asset(mesh);rows.append(dict(asset=mesh.get_path_name(),lods=b.EDIT.get_lod_count(mesh)))
(b.ROOT/'07_Documentation/S8/Evidence/EgmoreTambaram/lhb-underframe-import.json').write_text(json.dumps(rows,indent=2))
unreal.log('LHB_REFINEMENT_IMPORT_COMPLETE '+json.dumps(rows))
ticks=[0]
def finish(dt):
 ticks[0]+=1
 if ticks[0]>60:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
if os.environ.get('RAIL_KEEP_EDITOR_OPEN','0')!='1':handle=unreal.register_slate_post_tick_callback(finish)
