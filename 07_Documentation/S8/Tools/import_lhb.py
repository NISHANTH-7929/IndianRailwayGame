"""Import LHB candidate with existing S8 importer, into its own coach namespace."""
import importlib.util,json
from pathlib import Path
import unreal
spec=importlib.util.spec_from_file_location('s8_import',Path(__file__).with_name('import_locomotives.py'))
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
base.DEST='/Game/IndianRailway/S8/Coaches'
original_material=base.material_for
def varied_material(family,definition):
 material=original_material(family,definition)
 base.MAT.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
 # Source window panes/frames contain inward-facing surfaces (inspection evidence).
 if definition['name'] in ('Material.012','Material.019'):
  material.set_editor_property('two_sided',True)
 vertex=base.expression(material,unreal.MaterialExpressionVertexColor)
 rough=base.expression(material,unreal.MaterialExpressionMultiply)
 value=base.expression(material,unreal.MaterialExpressionConstant,r=float(definition.get('roughness',.6)))
 base.MAT.connect_material_expressions(vertex,'A',rough,'A')
 base.MAT.connect_material_expressions(value,'',rough,'B')
 base.connect(rough,material,unreal.MaterialProperty.MP_ROUGHNESS)
 base.MAT.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material)
 return material
base.material_for=varied_material
source=base.ROOT/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/Prepared'
report=base.import_family('LHB3A',source=source,publish_runtime=False)
runtime=report['runtime_data']
order=['Body','Bogie0','Bogie1','Wheel0','Wheel1','Wheel2','Wheel3']
runtime['parts'].sort(key=lambda p:order.index(p['name']))
for part in runtime['parts']:
 mesh=unreal.load_asset(part['asset'])
 if part['name']=='Body':
  base.EDIT.remove_collisions(mesh)
  base.EDIT.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.BOX)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 runtime.setdefault('validation',[]).append({'asset':part['asset'],'lod_count':base.EDIT.get_lod_count(mesh),
   'lod_triangles':[mesh.get_num_triangles(i) for i in range(base.EDIT.get_lod_count(mesh))]})
runtime['provenance']='04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431/provenance.json'
runtime['streaming_owner']='S6 chunk actor; instance identity follows logical vehicle; no simulation authority'
runtime['acceptance']='pending desktop/device validation'
target=base.CONTENT/'IndianRailway/Production/coaches';target.mkdir(parents=True,exist_ok=True)
(target/'LHB3A.json').write_text(json.dumps(runtime,indent=2),encoding='utf-8')
evidence=base.ROOT/'07_Documentation/S8/Evidence/LHB'
(evidence/'unreal-import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.EditorAssetLibrary.save_directory(base.DEST,only_if_is_dirty=True,recursive=True)
unreal.log('S8_LHB_IMPORT_COMPLETE')
unreal.SystemLibrary.quit_editor()
