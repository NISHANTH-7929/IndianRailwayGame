"""Create new presentation derivatives; preserve S8 and first journey finishes."""
import unreal,json,importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('base',ROOT/'07_Documentation/S8/Tools/import_locomotives.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
DEST='/Game/IndianRailway/S9/JourneyFinishV2'
receipt=ROOT/'07_Documentation/S9/Evidence/JourneyFinish-20261017-01/materials.json'
assert not receipt.exists(), 'Evidence already exists'
rows=[]
for name in ['DeskEnamel','DeskEnamel_StaticAO','CabIvory','CabIvory_StaticAO','PanelCharcoal','PanelCharcoal_StaticAO','WindowGlass','InstrumentGlass']:
 source='/Game/IndianRailway/S9/JourneyMaterials/'+name
 # A failed first import can leave a saved, unmodified duplicate.
 m=unreal.load_asset(DEST+'/'+name) if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name) else unreal.EditorAssetLibrary.duplicate_asset(source,DEST+'/'+name);assert m
 if 'Glass' in name:
  # Clear visibility with a smooth reflection lobe; keep prior Fresnel opacity.
  rough=b.expression(m,unreal.MaterialExpressionConstant,r=.075 if name=='WindowGlass' else .11)
  b.connect(rough,m,unreal.MaterialProperty.MP_ROUGHNESS)
 else:
  # Fine finish variation follows the authored UVs and leaves baked AO intact.
  original=b.MAT.get_material_property_input_node(m,unreal.MaterialProperty.MP_ROUGHNESS);assert original
  uv=b.expression(m,unreal.MaterialExpressionTextureCoordinate)
  zero=b.expression(m,unreal.MaterialExpressionConstant,r=0.)
  pos=b.expression(m,unreal.MaterialExpressionAppendVector)
  assert b.MAT.connect_material_expressions(uv,'',pos,'A')
  assert b.MAT.connect_material_expressions(zero,'',pos,'B')
  noise=b.expression(m,unreal.MaterialExpressionNoise,scale=90.,quality=1,levels=1)
  assert b.MAT.connect_material_expressions(pos,'',noise,'')
  amount=b.expression(m,unreal.MaterialExpressionMultiply,const_b=.025)
  assert b.MAT.connect_material_expressions(noise,'',amount,'A')
  added=b.expression(m,unreal.MaterialExpressionAdd)
  assert b.MAT.connect_material_expressions(original,'',added,'A')
  assert b.MAT.connect_material_expressions(amount,'',added,'B')
  bounded=b.expression(m,unreal.MaterialExpressionClamp,min_default=.24,max_default=.94)
  assert b.MAT.connect_material_expressions(added,'',bounded,'')
  b.connect(bounded,m,unreal.MaterialProperty.MP_ROUGHNESS)
 b.MAT.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
 rows.append(dict(source=source,destination=m.get_path_name()))
# Differentiate clean rods, rough castings, rubber and coated tank surfaces.
# Existing geometry, vertex wear, clear coat and LODs are retained.
settings={
 'FrameSteel':{'Roughness':.57,'Metallic':.62},
 'DarkSteel':{'Roughness':.65,'Metallic':.48},
 'DustSteel':{'Roughness':.77,'Metallic':.20},
 'TankIvory':{'Roughness':.64,'Metallic':.05},
 'Rubber':{'Roughness':.87,'Metallic':0.},
 'PistonSteel':{'Roughness':.20,'Metallic':.90},
 'WeldSteel':{'Roughness':.62,'Metallic':.55},
 'LHB_PaintRedV2':{'Roughness':.36,'ClearCoat':.26,'CoatRoughness':.32},
 'LHB_PaintGreyV2':{'Roughness':.46,'ClearCoat':.18,'CoatRoughness':.38},
 'LHB_GlassV2':{'Roughness':.10,'CoatRoughness':.07,'GlazingEdgeTint':.10}}
parents={}
for meshname in ['SM_LHB_BodyV2','SM_LHB_BogieV2']:
 mesh=unreal.load_asset('/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/'+meshname);assert mesh
 for i in range(len(mesh.get_editor_property('static_materials'))):
  m=mesh.get_material(i)
  if m and m.get_name() in settings:parents[m.get_name()]=m
assert set(parents)==set(settings),str(set(settings)-set(parents))
for name,values in settings.items():
 assert not unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name),name
 m=b.ASSETS.create_asset(name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew());assert m
 b.MAT.set_material_instance_parent(m,parents[name])
 for key,value in values.items():b.MAT.set_material_instance_scalar_parameter_value(m,key,value)
 b.MAT.update_material_instance(m)
 for key,value in values.items():assert abs(b.MAT.get_material_instance_scalar_parameter_value(m,key)-value)<.0001
 assert unreal.EditorAssetLibrary.save_loaded_asset(m)
 rows.append(dict(source=parents[name].get_path_name(),destination=m.get_path_name(),parameters=values))
receipt.parent.mkdir(parents=True,exist_ok=True)
receipt.write_text(json.dumps(dict(materials=rows,limitations=['Opaque coach glazing retained; no passenger interior claimed.','Representative finish, not measured surface replication.']),indent=2))
unreal.log('JOURNEY_FINISH_V2_READY')
