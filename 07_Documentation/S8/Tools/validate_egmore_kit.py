"""Source/import validation for Egmore; deliberately separate from acceptance."""
import json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];source=ROOT/'04_Assets/S8/EgmoreKit';e=ROOT/'07_Documentation/S8/Evidence/Egmore'
a=json.loads((source/'assembly.json').read_text());receipt=json.loads((e/'unreal-import.json').read_text());runtime=receipt['runtime_data']
checks={
 'stable_identity':a['asset_id']=='Station.Egmore.Heritage.01' and a['version']==1,
 'twelve_modules':len(a['parts'])==12 and len({p['asset_id'] for p in a['parts']})==12,
 'source_receipt_matches':bool(receipt.get('sourceHashes')) and all(hashlib.sha256((source/p).read_bytes()).hexdigest()==h for p,h in receipt.get('sourceHashes',{}).items()),
 'three_nonempty_lods':all(len(p['lod_triangles'])==3 and min(p['lod_triangles'])>0 for p in runtime['parts']),
 'imported_metric_basis':all('imported_bounds_m' in p and all(abs(p['imported_bounds_m'][i][j]-p['bounds_m'][i][j])<.001 for i in range(2) for j in range(3)) for p in runtime['parts']),
 'separate_unreal_paths':all(p['asset'].startswith('/Game/IndianRailway/S8/Stations/EgmoreHeritage01/') for p in runtime['parts']),
 'source_runtime_names':{p['name'] for p in a['parts']}=={p['name'] for p in runtime['parts']}}
required={'EgmoreArcade6m','EgmoreTower4m','EgmoreDome','EgmoreEntrance9m',
          'EgmoreCanopy6m','EgmorePlatformBench','EgmoreWaterPoint','EgmoreLitterBins',
          'EgmorePlatformBoard','EgmoreDrain3m','EgmoreCeilingFan','EgmoreBallast3m'}
checks['expected_architecture_and_fittings']={p['name'] for p in a['parts']}==required
checks['pbr_files']=all(set(m['textures'])=={'BC','N','ORM'} and
    all((source/f).is_file() and (source/f).stat().st_size>0 for f in m['textures'].values())
    for m in a['materials'])
checks['finite_positive_dimensions']=all(all(__import__('math').isfinite(v) and v>0 for v in p['dimensions_m']) for p in a['parts'])
records=[]
for p in a['parts']:
 records.append({'AssetId':p['asset_id'],'AssetName':p['name'],'Category':'station',
  'Creator':'Project-authored procedural geometry and PBR','Source':'07_Documentation/S8/Tools/build_egmore_kit.py',
  'SourceReference':'07_Documentation/S8/EGMORE_REFERENCE_REGISTER.md','AcquisitionDate':'2026-10-03',
  'License':'Original geometry; no photographic pixels incorporated','CommercialUse':'verified','ModificationPermission':'verified',
  'RedistributionRestrictions':'Reference photos have separate licenses; do not package them as source textures',
  'AttributionRequirement':'See reference register for separate photographic credit',
  'OriginalArchive':'Locally generated source','ModifiedAssetReference':'04_Assets/S8/EgmoreKit/'+p['fbx'],
  'VerificationStatus':'pending review','VerificationEvidence':['07_Documentation/S8/Evidence/Egmore/unreal-import.json'],
  'Notes':'Estimated architectural study; not survey accurate or visually accepted; Android integration pending.'})
from validate_s8 import validate_record
checks['provenance_schema']=all(not validate_record('provenance',r) for r in records)
(source/'provenance.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
result={'passed':all(checks.values()),'checks':checks,'sourceTriangles':sum(p['triangles'] for p in a['parts']),
 'lodTriangles':[sum(p['lod_triangles'][i] for p in runtime['parts']) for i in range(3)],
 'visualAccepted':False,'androidMeasured':False,'fullStationComplete':False}
(e/'content-validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result,indent=2))
raise SystemExit(0 if result['passed'] else 1)
