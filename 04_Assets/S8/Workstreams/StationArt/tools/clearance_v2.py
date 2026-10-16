import bpy,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1]/'v2.0.0';m=json.loads((R/'manifest.json').read_text())
tracks=m['geometryContract']['trackCentresY'];violations=[]
# Check every triangle AABB against a conservative stationary vehicle envelope.
# Exclude intended rails/ballast; platforms are tested above coping height.
for p in m['placements']:
    if p['module'] in ('Track6m','Ballast6m','Wire18m','GroundTile24m'):continue
    o=bpy.data.objects[p['stableId']];o.data.calc_loop_triangles()
    for t in o.data.loop_triangles:
        vs=[o.matrix_world@o.data.vertices[i].co for i in t.vertices]
        low=[min(v[i] for v in vs) for i in range(3)];high=[max(v[i] for v in vs) for i in range(3)]
        if high[2]<=.90 or low[2]>=4.80:continue
        for y in tracks:
            if low[1]<y+1.625 and high[1]>y-1.625:
                violations.append({'id':p['stableId'],'module':p['module'],'trackY':y,'triangleBounds':[low,high]});break
        if violations and violations[-1]['id']==p['stableId']:break
result={'status':'PASS' if not violations else 'FAIL','vehicleEnvelope':{'halfWidthM':1.625,'zRangeM':[.90,4.80]},'violations':violations,'limit':'Conservative triangle-AABB stationary clearance test; actual vehicle, curves and motion require integration validation.'}
(R/'Evidence/clearance.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
