"""Portable module exports for independent inspection; do not mutate .blend."""
import bpy,json,hashlib,sys,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
source=ROOT/'04_Assets/S8/EgmoreKit'
parser=argparse.ArgumentParser()
parser.add_argument('--revision',default='v2-ballast')
options=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
if not options.revision.replace('-','').isalnum():raise ValueError('Invalid revision folder')
out=ROOT/'04_Assets/S8/EgmorePortable'/options.revision
out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(source/'EgmoreKit.blend'))
records=[]
for part in json.loads((source/'assembly.json').read_text())['parts']:
    bpy.ops.object.select_all(action='DESELECT')
    obj=bpy.data.objects['SM_Station_'+part['name']]
    obj.hide_set(False);obj.hide_render=False;obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    # Resolve ngons before tangent calculation. Changes remain in this process;
    # the authored .blend and Unreal FBX are not saved or replaced here.
    modifier=obj.modifiers.new('Portable tangent triangulation','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    destination=out/(part['name']+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(destination),export_format='GLB',use_selection=True,
                              export_yup=True,export_animations=False,export_tangents=True)
    records.append({'id':part['asset_id'],'file':destination.name,
                    'sha256':hashlib.sha256(destination.read_bytes()).hexdigest(),
                    'units':'metres','source':str(source/'EgmoreKit.blend'),
                    'estimatedDimensions':True})
    obj.hide_set(True)
(out/'manifest.json').write_text(json.dumps({'sourceSha256':hashlib.sha256((source/'EgmoreKit.blend').read_bytes()).hexdigest(),
    'modules':records,'status':'inspection candidate; Unreal FBX remains runtime source'},indent=2))
print('PORTABLE_EXPORT_COMPLETE',len(records))
