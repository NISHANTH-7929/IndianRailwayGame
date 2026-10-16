"""Create a canonical UE export copy from a sealed station-art handoff (Blender)."""
import bpy,json,hashlib,struct,shutil,os
from pathlib import Path
from mathutils import Matrix
ROOT=Path(__file__).resolve().parents[3]
INTERIOR=os.environ.get('RAIL_EGMORE_INTERIOR')=='1'
SOURCE=ROOT/'04_Assets/S8/Workstreams/StationArt'/('v2.1.0-interior' if INTERIOR else 'v2.0.0')
FAMILY='EgmoreInterior01' if INTERIOR else 'EgmoreScene02'
OUT=ROOT/'04_Assets/S8'/FAMILY/('2.1.0' if INTERIOR else '2.0.0')
E=ROOT/'07_Documentation/S8/Evidence'/FAMILY
for row in json.loads((SOURCE/'SHA256SUMS.json').read_text()):
    assert hashlib.sha256((SOURCE/row['path']).read_bytes()).hexdigest()==row['sha256'],row['path']
E.mkdir(parents=True,exist_ok=True);(OUT/'Parts').mkdir(parents=True,exist_ok=True);(OUT/'Textures').mkdir(exist_ok=True)
manifest=json.loads((SOURCE/'manifest.json').read_text())
materials={};report=[]
for module in manifest['modules']:
    raw=(SOURCE/module['lods'][0]['glb']).read_bytes();n=struct.unpack_from('<I',raw,12)[0]
    doc=json.loads(raw[20:20+n]);binary=raw[28+n:]
    def tex(info):
        result=dict(info);im=doc['images'][doc['textures'][result.pop('index')]['source']]
        view=doc['bufferViews'][im['bufferView']];data=binary[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']]
        name=hashlib.sha256(data).hexdigest()[:20]+('.png' if im['mimeType']=='image/png' else '.jpg')
        (OUT/'Textures'/name).write_bytes(data);result['uri']='Textures/'+name;return result
    for mat in doc.get('materials',[]):
        mat=json.loads(json.dumps(mat))
        for container in [mat,mat.get('pbrMetallicRoughness',{})]:
            for key,value in list(container.items()):
                if key.endswith('Texture'):container[key]=tex(value)
        if mat['name'] in materials and materials[mat['name']]!=mat:raise RuntimeError('Conflicting shared material '+mat['name'])
        materials[mat['name']]=mat
    for lod in module['lods']:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(SOURCE/lod['glb']))
        objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
        bpy.ops.object.select_all(action='DESELECT')
        for ob in objects:ob.select_set(True)
        bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();ob=bpy.context.object
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        bounds=[[min(v.co[i] for v in ob.data.vertices) for i in range(3)],[max(v.co[i] for v in ob.data.vertices) for i in range(3)]]
        ob.data.calc_loop_triangles();triangles=len(ob.data.loop_triangles)
        assert triangles==lod['triangles'],(module['module'],lod['level'],triangles,lod['triangles'])
        if lod['level']==0:assert max(abs(bounds[i][j]-module['boundsM'][i][j]) for i in range(2) for j in range(3))<.001
        # Lettering needs readable faces after the handedness conversion.
        # Keep sealed GLBs intact; only the canonical FBX copy is corrected.
        if INTERIOR and module['module']!='CanopyLight':
            ob.data.transform(Matrix.Diagonal((-1,1,1,1)));ob.data.flip_normals()
        for poly in ob.data.polygons:
            if ob.data.materials[poly.material_index].name=='EgmoreName':
                for index in poly.loop_indices:
                    uv=ob.data.uv_layers.active.data[index].uv;uv.x=1-uv.x
        # Match the already verified canonical station FBX import convention.
        ob.data.transform(Matrix.Diagonal((1,-1,1,1)));ob.data.flip_normals();ob.data.update()
        path=OUT/'Parts'/f"{module['module']}_LOD{lod['level']}.fbx"
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',use_triangles=True,path_mode='STRIP',add_leaf_bones=False)
        report.append(dict(module=module['module'],lod=lod['level'],boundsM=bounds,triangles=triangles,fbx=str(path.relative_to(OUT)),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2));(OUT/'materials.json').write_text(json.dumps(list(materials.values()),indent=2))
for filename in (['provenance.json','README.md','lighting-bindings.json','material-bindings.json'] if INTERIOR else ['provenance.json','stable-id-map.json','SOURCE_REVIEW.md']):
    destination=OUT/filename
    if destination.exists():
        assert destination.read_bytes()==(SOURCE/filename).read_bytes(),'Canonical provenance changed'
    else:shutil.copy2(SOURCE/filename,destination)
(E/'preparation.json').write_text(json.dumps(dict(source_manifest_sha256=hashlib.sha256((SOURCE/'manifest.json').read_bytes()).hexdigest(),source_untouched=True,modules=len(manifest['modules']),exports=report,materials=len(materials),scope='canonical import copy; not yet Unreal/Android accepted'),indent=2))
print('EGMORE_SCENE02_PREPARED',len(report),len(materials))
