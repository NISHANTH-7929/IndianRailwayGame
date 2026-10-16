import bpy,json,hashlib
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parents[1];R=O/'v2.1.0-interior';B=O/'v2.0.0'
m=json.loads((R/'manifest.json').read_text());checks=[]
def check(name,passed,detail):checks.append({'check':name,'pass':bool(passed),'detail':detail})
check('baseGLBUnchanged',hashlib.sha256((B/'EgmoreScene.glb').read_bytes()).hexdigest()==m['baseSceneSha256'],m['baseSceneSha256'])
check('baseBlendUnchanged',hashlib.sha256((B/'EgmoreScene.blend').read_bytes()).hexdigest()==m['baseSourceSha256'],m['baseSourceSha256'])
for a in m['modules']:
    for lod in a['lods']:
        for fmt in ('glb','fbx'):
            bpy.ops.wm.read_factory_settings(use_empty=True)
            if fmt=='glb':bpy.ops.import_scene.gltf(filepath=str(R/lod[fmt]))
            else:bpy.ops.import_scene.fbx(filepath=str(R/lod[fmt]))
            bpy.context.view_layer.update();obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];vs=[o.matrix_world@v.co for o in obs for v in o.data.vertices]
            bounds=[[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]]
            err=max(abs(bounds[r][i]-a['boundsM'][r][i]) for r in (0,1) for i in range(3))
            count=0;deg=0
            for o in obs:
                o.data.calc_loop_triangles();count+=len(o.data.loop_triangles)
                for t in o.data.loop_triangles:
                    v=[o.data.vertices[i].co for i in t.vertices]
                    if (v[1]-v[0]).cross(v[2]-v[0]).length<1e-10:deg+=1
            check(lod[fmt],err<.001 and count==lod['triangles'] and deg==0,{'maxBoundsErrorM':err,'triangles':count,'degenerateTriangles':deg})
keys={a['module']:a for a in m['modules']}
for p in m['placements']:
    low=p['positionM'][2]+keys[p['module']]['boundsM'][0][2]
    check(p['stableId']+'.headClearance',low-.84>=2.5,{'heightAbovePlatformM':low-.84})
ids=[p['stableId'] for p in m['placements']];check('uniqueIDs',len(ids)==len(set(ids)),len(ids))
check('previews',len(list((R/'Previews').glob('*.png')))>=4,len(list((R/'Previews').glob('*.png'))))
report={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,'scope':'Additive interior source assets; Blender reimports all module formats/LODs and measures head clearance. Runtime remains untested.'}
(R/'Evidence/audit.json').write_text(json.dumps(report,indent=2));print('AUDIT',report['status'],len(checks),[c for c in checks if not c['pass']])
