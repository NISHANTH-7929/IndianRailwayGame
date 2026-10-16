"""Validate separate signage source, shaped text and imported metric geometry."""
import json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
source=ROOT/'04_Assets/S8/EgmoreSignage'
e=ROOT/'07_Documentation/S8/Evidence/Egmore/Signage'
a=json.loads((source/'assembly.json').read_text())
r=json.loads((e/'unreal-import.json').read_text())
t=json.loads((source/'lettering-receipt.json').read_text(encoding='utf-8'))
checks={
 'independent_identity':a['asset_id']=='Station.Egmore.Signage.01' and a['version']==1,
 'one_named_module':len(a['parts'])==1 and a['parts'][0]['asset_id']=='Station.Kit.EgmoreNameboard4m',
 'source_hashes':bool(r['sourceHashes']) and all(hashlib.sha256((source/p).read_bytes()).hexdigest()==h for p,h in r['sourceHashes'].items()),
 'three_languages_shaped':len(t['lines'])==3 and all(x['missingGlyphs']==0 and 0 not in x['glyphIds'] for x in t['lines']),
 'no_reference_pixels':t['sourcePixelsUsed'] is False,
 'imported_bounds':all(all(abs(p['imported_bounds_m'][i][j]-p['bounds_m'][i][j])<.001 for i in range(2) for j in range(3)) for p in r['runtime_data']['parts']),
 'three_lods':all(len(p['lod_triangles'])==3 and min(p['lod_triangles'])>0 for p in r['runtime_data']['parts']),
 'separate_paths':all('/EgmoreSignage01/' in p['asset'] for p in r['runtime_data']['parts'])}
result={'passed':all(checks.values()),'checks':checks,'parts':r['runtime_data']['parts'],'visualAccepted':False}
(e/'validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps({'passed':result['passed'],'checks':checks},indent=2))
raise SystemExit(0 if result['passed'] else 1)
