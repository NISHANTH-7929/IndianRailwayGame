"""Import original station modules and authored PBR maps using the existing pipeline."""
import importlib.util,json,hashlib
from pathlib import Path
import unreal
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'))
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
base.DEST='/Game/IndianRailway/S8/Stations'
source=base.ROOT/'04_Assets/S8/StationKit'
def material(family,d):
    path=f'{base.DEST}/{family}/Materials';label=d['name']
    m=unreal.load_asset(path+'/'+label) or base.ASSETS.create_asset(label,path,unreal.Material,unreal.MaterialFactoryNew())
    base.MAT.delete_all_material_expressions(m)
    base.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    for suffix,file in d['textures'].items():
        t=unreal.AssetImportTask();t.filename=str(source/file);t.destination_path=f'{base.DEST}/{family}/Textures'
        t.automated=True;t.replace_existing=True;t.save=True;base.ASSETS.import_asset_tasks([t])
        texture=unreal.load_asset(t.destination_path+'/'+Path(file).stem)
        texture.set_editor_property('srgb',suffix=='BC')
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if suffix=='N' else unreal.TextureCompressionSettings.TC_MASKS if suffix=='ORM' else unreal.TextureCompressionSettings.TC_DEFAULT)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        node=base.expression(m,unreal.MaterialExpressionTextureSample,texture=texture)
        node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix=='N' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS if suffix=='ORM' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        if suffix=='BC':base.connect(node,m,unreal.MaterialProperty.MP_BASE_COLOR,'RGB')
        elif suffix=='N':base.connect(node,m,unreal.MaterialProperty.MP_NORMAL,'RGB')
        else:
            for channel,prop in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:base.connect(node,m,prop,channel)
    base.MAT.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);return m
base.material_for=material
report=base.import_family('Kit01',source=source,publish_runtime=False)
report['sourceHashes']={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file() and p.suffix in ('.fbx','.png','.json') and p.name not in ('provenance.json','contact-sheet.png')}
for p in report['runtime_data']['parts']:
    mesh=unreal.load_asset(p['asset']);base.EDIT.remove_collisions(mesh)
    bounds=mesh.get_bounding_box()
    actual=[[bounds.min.x/100,bounds.min.y/100,bounds.min.z/100],[bounds.max.x/100,bounds.max.y/100,bounds.max.z/100]]
    if any(abs(actual[i][j]-p['bounds_m'][i][j])>.001 for i in range(2) for j in range(3)):
        raise RuntimeError('Imported metric basis mismatch: '+p['name'])
    p['imported_bounds_m']=actual
    if p['name'] not in ('Ground50m','Ballast3m','Rail3m','Sleeper'):
        base.EDIT.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.BOX)
    p['lod_triangles']=[mesh.get_num_triangles(i) for i in range(3)]
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
out=base.CONTENT/'IndianRailway/Production/stations';out.mkdir(parents=True,exist_ok=True)
(out/'Kit01.json').write_text(json.dumps(report['runtime_data'],indent=2))
e=base.ROOT/'07_Documentation/S8/Evidence/Station';e.mkdir(parents=True,exist_ok=True)
(e/'unreal-import.json').write_text(json.dumps(report,indent=2))
unreal.log('STATION_IMPORT_COMPLETE')
