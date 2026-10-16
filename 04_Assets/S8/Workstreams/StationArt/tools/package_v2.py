from pathlib import Path
import json,hashlib,shutil
O=Path(__file__).resolve().parents[1];R=O/'v2.0.0';m=json.loads((R/'manifest.json').read_text())
policy=json.loads((O/'policy-v2.json').read_text())
refs=[{'file':'facade-panorama.jpg','creator':'PlaneMad','date':'2009-01-01','license':'CC BY-SA 3.0','url':'https://commons.wikimedia.org/wiki/File:Chennai_Egmore_facade_panorama.jpg'}, {'file':'east.jpg','creator':'Pratyeka','date':'2011-05-22','license':'CC BY-SA 3.0','url':'https://commons.wikimedia.org/wiki/File:Chennai_Egmore_Railway_Station_(view_from_the_east).jpeg'}]
prov={'assetFamilyId':m['assetFamilyId'],'version':m['version'],'origin':'project-authored and adapted from existing project-authored Egmore source meshes','license':'LicenseRef-Project-Original','licenseMeaning':'Project-owned original geometry/materials; this label grants no new public redistribution license. Reference photographs retain separate licenses.','paidProviderUsed':False,'costUSD':0,'photoPixelsUsed':False,'sourceReceipts':m['sourceReceipts'],'references':refs,'referenceRegister':'07_Documentation/S8/EGMORE_REFERENCE_REGISTER.md','changes':['Coherent four-track representative arrangement','Track head section and 1.676m inside gauge','8m platforms with 1.675m edge setback','Bridge stairs and guardrail entry gaps','Reduced reusable modules and actual LOD exports','Original track/OHE/duct/signal housing/context meshes','Clean triangulation, UVs, normals and tangents'],'dimensionalUncertainty':'Station layout and architecture are visually estimated, not surveyed. The named-station identity is architectural; no claim of full yard or current redevelopment accuracy.','noProviderPrompt':'No remote generation; deterministic staging scripts are the production specification.'}
(R/'provenance.json').write_text(json.dumps(prov,indent=2))
mapping={'newFamily':m['assetFamilyId'],'existingCanonicalIDsModified':False,'rejectedFamily':'Station.Egmore.Scene.01 v1.0.0','modules':[{'module':a['module'],'newAssetId':a['assetId'],'sourceAssetId':a['sourceAssetId'],'lodFiles':a['lods']} for a in m['modules']],'placements':'manifest.json placements[].stableId'}
(R/'stable-id-map.json').write_text(json.dumps(mapping,indent=2))
metadata={'name':'Chennai Egmore Station Art','version':'2.0.0','license':'LicenseRef-Project-Original','category':'architecture','description':'Representative Chennai Egmore heritage station source scene, validated geometry and modular LOD exports. Source acceptance only; engine and Android acceptance pending.','previewPath':str(R/'Previews/HeritageStreet.png'),'policy':policy,'provenance':prov}
(O/'package-v2-request.json').write_text(json.dumps(metadata,indent=2))
metadata.pop('previewPath',None) # CLI package preview slot only accepts USDZ; PNGs stay in source deliverable.
(O/'package-v2-request.json').write_text(json.dumps(metadata,indent=2))
(R/'Reproduce').mkdir(exist_ok=True)
for name in ['build_v2.py','audit_v2.py','clearance_v2.py','roundtrip_v2.py','lookdev_v2.py','package_v2.py']:
    shutil.copy2(O/'tools'/name,R/'Reproduce'/name)
shutil.copy2(O/'policy-v2.json',R/'Reproduce/policy.json')
print(json.dumps({'modules':len(m['modules']),'placements':len(m['placements']),'triangles':m['expandedLODTriangles'],'request':str(O/'package-v2-request.json')}))
