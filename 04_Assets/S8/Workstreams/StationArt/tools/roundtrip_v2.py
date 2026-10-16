import bpy,json,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1]/'v2.0.0'
m=json.loads((R/'manifest.json').read_text());checks=[]
def bounds(obs):
    p=[o.matrix_world@v.co for o in obs for v in o.data.vertices]
    return [[min(v[i] for v in p) for i in range(3)],[max(v[i] for v in p) for i in range(3)]]
for a in m['modules']:
    for fmt in ('glb','fbx'):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        p=str(R/a['lods'][0][fmt])
        if fmt=='glb':bpy.ops.import_scene.gltf(filepath=p)
        else:bpy.ops.import_scene.fbx(filepath=p)
        bpy.context.view_layer.update();obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];b=bounds(obs)
        delta=max(abs(b[r][i]-a['boundsM'][r][i]) for r in (0,1) for i in range(3))
        checks.append({'module':a['module'],'format':fmt,'pass':delta<.001,'maxBoundsErrorM':delta,'actualBoundsM':b})
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(R/'EgmoreScene.glb'))
S=bpy.context.scene;S.world=bpy.data.worlds.new('Roundtrip daylight');S.world.use_nodes=True
bg=S.world.node_tree.nodes.get('Background');bg.inputs[0].default_value=(.52,.65,.8,1);bg.inputs[1].default_value=.7
bpy.ops.object.light_add(type='SUN',location=(0,-50,70));sun=bpy.context.object;sun.data.energy=2.8;sun.data.angle=.12;sun.rotation_euler=(.45,-.45,-.3)
bpy.ops.object.camera_add(location=(-50,8.4,2.5));c=bpy.context.object;c.data.lens=32;c.rotation_euler=(Vector((30,8.8,3.1))-c.location).to_track_quat('-Z','Y').to_euler();S.camera=c
S.render.engine='CYCLES';S.cycles.samples=24;S.cycles.use_denoising=True;S.view_settings.view_transform='AgX'
S.view_settings.exposure=.4
S.render.resolution_x=1440;S.render.resolution_y=900;S.render.resolution_percentage=100
S.render.filepath=str(R/'Previews/ImportedGLB_PlatformEye.png');bpy.ops.render.render(write_still=True)
(R/'Evidence/roundtrip.json').write_text(json.dumps({'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,'render':'Previews/ImportedGLB_PlatformEye.png','boundary':'Blender reimport; Unreal coordinate/material handling must be checked separately'},indent=2))
print('ROUNDTRIP',len(checks),'FAILURES',len([c for c in checks if not c['pass']]))
