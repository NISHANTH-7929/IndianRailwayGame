"""Read actual GLB bytes and validate geometry contracts; write staging evidence."""
from pathlib import Path
import json,struct,hashlib,math
import numpy as np
R=Path(__file__).resolve().parents[1]/'v2.0.0'
def glb(path):
    data=path.read_bytes();magic,ver,size=struct.unpack_from('<III',data)
    assert magic==0x46546c67 and ver==2 and size==len(data),path
    length,kind=struct.unpack_from('<II',data,12);assert kind==0x4e4f534a
    j=json.loads(data[20:20+length]);off=20+length
    blen,bkind=struct.unpack_from('<II',data,off);assert bkind==0x004e4942
    return j,data[off+8:off+8+blen]
def accessor(j,bin,i):
    a=j['accessors'][i];v=j['bufferViews'][a['bufferView']]
    types={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1',5122:'<i2',5120:'i1'}
    n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];dt=np.dtype(types[a['componentType']]);offset=v.get('byteOffset',0)+a.get('byteOffset',0)
    return np.ndarray((a['count'],n),dtype=dt,buffer=bin,offset=offset,strides=(v.get('byteStride',dt.itemsize*n),dt.itemsize)).copy()
checks=[]
def check(name,ok,detail):checks.append({'check':name,'pass':bool(ok),'detail':detail})
m=json.loads((R/'manifest.json').read_text());results=[]
for a in m['modules']:
    counts=[]
    for lod in a['lods']:
        p=R/lod['glb'];j,b=glb(p);positions=[];tri=0;uv=True;norm=True;tang=True;finite=True;deg=0
        for mesh in j['meshes']:
            for pr in mesh['primitives']:
                at=pr['attributes'];v=accessor(j,b,at['POSITION']);positions.append(v);finite &= bool(np.isfinite(v).all());uv &= 'TEXCOORD_0' in at;norm &= 'NORMAL' in at
                mat=j.get('materials',[])[pr.get('material',0)]
                if 'normalTexture' in mat:tang &= 'TANGENT' in at
                idx=accessor(j,b,pr['indices']).reshape(-1) if 'indices' in pr else np.arange(len(v));tri+=len(idx)//3
                vv=v[idx.reshape(-1,3)];deg+=int((np.linalg.norm(np.cross(vv[:,1]-vv[:,0],vv[:,2]-vv[:,0]),axis=1)<1e-10).sum())
        v=np.concatenate(positions);bounds=[v.min(0).tolist(),v.max(0).tolist()]
        counts.append(tri)
        check(a['module']+f'.LOD{lod["level"]}.attributes',uv and norm and finite and tang and deg==0,{'uv':uv,'normals':norm,'finite':finite,'tangentsForNormalMaps':tang,'degenerateTriangles':deg})
        check(a['module']+f'.LOD{lod["level"]}.triangles',tri==lod['triangles'],{'exported':tri,'declared':lod['triangles']})
        results.append({'file':lod['glb'],'triangles':tri,'boundsGltfM':bounds,'materials':len(j.get('materials',[])),'images':len(j.get('images',[])),'degenerateTriangles':deg,'tangentsForNormalMaps':tang})
    check(a['module']+'.LODReduction',counts[0]>=counts[1]>=counts[2],counts)
ids=[p['stableId'] for p in m['placements']];check('uniquePlacementIds',len(ids)==len(set(ids)),len(ids))
g=m['geometryContract']
# Measure rail-head primitive vertices from exported glTF; Blender Z becomes glTF Y.
j,b=glb(R/'Modules/Track6m_LOD0.glb');heads=[]
for mesh in j['meshes']:
    for pr in mesh['primitives']:
        if j['materials'][pr['material']]['name'].startswith('RailHead'):
            heads.extend(accessor(j,b,pr['attributes']['POSITION']).tolist())
h=np.array(heads);lat=h[:,2];gap=float(lat[lat>0].min()-lat[lat<0].max())
check('actualInsideHeadGauge',abs(gap-1.676)<1e-5,gap)
check('actualRailTop',abs(float(h[:,1].max()))<1e-5,float(h[:,1].max()))
clearances=[min(abs(edge-t) for t in g['trackCentresY']) for pair in g['platformYIntervals'] for edge in pair]
check('platformEdgeClearance',min(clearances)>=1.675-1e-6,clearances)
check('stairsJoinDeckX',abs(g['bridgeStairEndX']-g['bridgeDeckStartX'])<1e-6,[g['bridgeStairEndX'],g['bridgeDeckStartX']])
check('stairsJoinDeckZ',abs(g['platformTopM']+g['bridgeStairRisers']*g['bridgeStairRiseM']-g['bridgeDeckTopM'])<1e-6,g['bridgeDeckTopM'])
scene_j,_=glb(R/'EgmoreScene.glb');check('sceneMeshNodeCount',sum('mesh' in n for n in scene_j['nodes'])==len(ids),{'actual':sum('mesh' in n for n in scene_j['nodes']),'expected':len(ids)})
check('sourcePreviewCount',len(list((R/'Previews').glob('*.png')))>=5,len(list((R/'Previews').glob('*.png'))))
report={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,'exportInspection':results,'limits':'Static source and binary export checks only. Visual review, Unreal import and device performance are separate gates.'}
(R/'Evidence/binary-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps({'status':report['status'],'checks':len(checks),'failed':[c for c in checks if not c['pass']],'normalMappedFilesMissingTangents':sum(not r['tangentsForNormalMaps'] for r in results),'degenerateTriangles':sum(r['degenerateTriangles'] for r in results)},indent=2))
