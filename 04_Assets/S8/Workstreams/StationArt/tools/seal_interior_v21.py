import json,hashlib,shutil
from pathlib import Path
O=Path(__file__).resolve().parents[1];R=O/'v2.1.0-interior'
assert not (R/'READY.lock').exists()
audit=json.loads((R/'Evidence/audit.json').read_text());assert audit['status']=='PASS'
m=json.loads((R/'manifest.json').read_text());m['status']='ready-additive-source-review';m['expandedTriangles']=sum(next(a for a in m['modules'] if a['module']==p['module'])['lods'][0]['triangles'] for p in m['placements'])
(R/'manifest.json').write_text(json.dumps(m,indent=2))
materials=[{'name':'SignBlue','baseColourLinear':[.018,.055,.12],'roughness':.65},{'name':'SignWhite','baseColourLinear':[.86,.86,.77],'roughness':.65},{'name':'ExitGreen','baseColourLinear':[.025,.16,.08],'roughness':.65},{'name':'Housing','baseColourLinear':[.25,.27,.28],'roughness':.5},{'name':'LampLens','baseColourLinear':[.94,.88,.71],'roughness':.35,'emissionColourLinear':[.94,.88,.71],'emissionStrengthSource':2}]
(R/'material-bindings.json').write_text(json.dumps(materials,indent=2))
prov={'creator':'project-authored geometry and lettering','license':'LicenseRef-Project-Original','externalPixelsUsed':False,'paidGeneration':False,'baseVersion':'2.0.0','baseSceneSha256':m['baseSceneSha256'],'newGeometry':'Double-sided mesh lettering and sign housings, light fixtures. No new exterior work.','font':'Blender built-in Bfont converted to polygons; no font files distributed.','limits':'Representative English wayfinding and platform numbers must be bound to actual scenario; not a survey of real signage. Existing multilingual station nameboards remain in the base.'}
(R/'provenance.json').write_text(json.dumps(prov,indent=2))
(R/'Reproduce').mkdir(exist_ok=True)
for f in ('interior_v21.py','audit_interior_v21.py','seal_interior_v21.py'):shutil.copy2(O/'tools'/f,R/'Reproduce'/f)
(R/'READY.lock').write_text('Immutable additive station-interior source supplement, 2026-10-07. Runtime integration and user visual acceptance pending.\n')
files=[{'path':p.relative_to(R).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(R.rglob('*')) if p.is_file() and p.name!='SHA256SUMS.json']
(R/'SHA256SUMS.json').write_text(json.dumps(files,indent=2))
for f in files:assert hashlib.sha256((R/f['path']).read_bytes()).hexdigest()==f['sha256']
print(json.dumps({'hashFiles':len(files),'checks':len(audit['checks']),'placements':len(m['placements']),'triangles':m['expandedTriangles'],'deltaSha256':hashlib.sha256((R/'InteriorDelta.glb').read_bytes()).hexdigest()}))
