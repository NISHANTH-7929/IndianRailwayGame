import unreal,json,importlib.util,os,datetime
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('loco',Path(__file__).with_name('import_locomotives.py'))
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
source=ROOT/'04_Assets/S8/Cabs/WAP7'
original_material=base.material_for
texture_cache={}
def cab_material(family,definition):
 m=original_material(family,definition)
 if definition.get('detail_texture'):
  path=source/definition['detail_texture'];key=path.stem
  if key not in texture_cache:
   task=unreal.AssetImportTask();task.filename=str(path);task.destination_path=base.DEST+'/'+family+'/Textures';task.destination_name=key;task.automated=True;task.replace_existing=True;task.save=True;base.ASSETS.import_asset_tasks([task]);texture_cache[key]=unreal.load_asset(task.destination_path+'/'+key);texture_cache[key].set_editor_property('srgb',False);unreal.EditorAssetLibrary.save_loaded_asset(texture_cache[key])
  sample=base.expression(m,unreal.MaterialExpressionTextureSample,texture=texture_cache[key]);sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
  uv=base.expression(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=int(definition.get('detail_uv',0)));base.MAT.connect_material_expressions(uv,'',sample,'Coordinates')
  rgba=definition['base_color'];color=base.expression(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*rgba[:3],1));multiply=base.expression(m,unreal.MaterialExpressionMultiply);base.MAT.connect_material_expressions(sample,'RGB',multiply,'A');base.MAT.connect_material_expressions(color,'',multiply,'B');base.connect(multiply,m,unreal.MaterialProperty.MP_BASE_COLOR);rough=base.expression(m,unreal.MaterialExpressionMultiply);factor=base.expression(m,unreal.MaterialExpressionConstant,r=float(definition.get('roughness',.7)));base.MAT.connect_material_expressions(sample,'R',rough,'A');base.MAT.connect_material_expressions(factor,'',rough,'B');base.connect(rough,m,unreal.MaterialProperty.MP_ROUGHNESS);base.MAT.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
 if definition.get('ao_texture'):
  key='CabContact'
  if key not in texture_cache:
   task=unreal.AssetImportTask();task.filename=str(source/definition['ao_texture']);task.destination_path=base.DEST+'/'+family+'/Textures';task.destination_name=key;task.automated=True;task.replace_existing=True;task.save=True;base.ASSETS.import_asset_tasks([task]);texture_cache[key]=unreal.load_asset(task.destination_path+'/'+key);texture_cache[key].set_editor_property('srgb',False);unreal.EditorAssetLibrary.save_loaded_asset(texture_cache[key])
  original=base.MAT.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
  sample=base.expression(m,unreal.MaterialExpressionTextureSample,texture=texture_cache[key]);sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
  uv=base.expression(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=int(definition.get('ao_uv',1)));base.MAT.connect_material_expressions(uv,'',sample,'Coordinates')
  scale=base.expression(m,unreal.MaterialExpressionMultiply);base.MAT.connect_material_expressions(sample,'R',scale,'A');scale.set_editor_property('const_b',.45)
  offset=base.expression(m,unreal.MaterialExpressionAdd);base.MAT.connect_material_expressions(scale,'',offset,'A');offset.set_editor_property('const_b',.55)
  product=base.expression(m,unreal.MaterialExpressionMultiply);base.MAT.connect_material_expressions(original,'',product,'A');base.MAT.connect_material_expressions(offset,'',product,'B');base.connect(product,m,unreal.MaterialProperty.MP_BASE_COLOR)
  base.MAT.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
 if definition.get('instrument_emission'):
  color=base.expression(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*definition['instrument_emission'],1))
  strength=base.expression(m,unreal.MaterialExpressionScalarParameter,parameter_name='InstrumentBacklight',default_value=float(definition.get('instrument_emission_strength',.16)))
  emission=base.expression(m,unreal.MaterialExpressionMultiply)
  base.MAT.connect_material_expressions(color,'',emission,'A');base.MAT.connect_material_expressions(strength,'',emission,'B')
  base.connect(emission,m,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  base.MAT.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
 return m
base.material_for=cab_material
report=base.import_family('WAP7',source=source,asset_family='WAP7/Cab',publish_runtime=False)
cab=report.pop('runtime_data')
runtime=base.CONTENT/'IndianRailway/Production/locomotives/WAP7.json'
exterior=json.loads(runtime.read_text())
exterior['parts']=[p for p in exterior['parts'] if not p.get('cab_only')]+cab['parts']
for key in ['driver_camera','instrument_display','cab_light_location_cm','cab_available']:
 exterior[key]=cab[key]
exterior['cab_reference_limits']=cab['limitations']
exterior['cab_presentation_revision']=cab.get('presentation_revision','legacy-cab')
runtime.write_text(json.dumps(exterior,indent=2))
# Importing a cab must not load, relight or save a gameplay fixture.
base.unreal.EditorAssetLibrary.save_directory(base.DEST+'/WAP7/Cab',only_if_is_dirty=True,recursive=True)
receipt=Path(os.environ.get('RAIL_CAB_IMPORT_RECEIPT',str(ROOT/'07_Documentation/S8/Evidence/Cab'/('import-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json'))))
receipt.parent.mkdir(parents=True,exist_ok=True)
receipt.write_text(json.dumps(report,indent=2))
unreal.log('WAP7_CAB_IMPORTED')
if not os.environ.get('RAIL_CAB_KEEP_EDITOR'):unreal.SystemLibrary.quit_editor()
