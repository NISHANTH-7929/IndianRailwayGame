"""Record the origin of each independently authored station module."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];folder=ROOT/'04_Assets/S8/StationKit'
a=json.loads((folder/'assembly.json').read_text())
records=[]
for p in a['parts']:
    records.append({'AssetId':p['asset_id'],'AssetName':p['name'],'Category':'station',
        'Creator':'Project-authored with Codex; procedural Blender geometry and PBR synthesis',
        'Source':'07_Documentation/S8/Tools/build_station_kit.py',
        'SourceReference':'07_Documentation/S8/STATION_WORK_LOG.md',
        'AcquisitionDate':'2026-10-03','License':'Original project output; no marketplace asset bytes incorporated',
        'CommercialUse':'verified','ModificationPermission':'verified',
        'RedistributionRestrictions':'Do not redistribute the separate marketplace preview reference files with this kit',
        'AttributionRequirement':'Reference sellers recorded for provenance; no seller asset is claimed as original',
        'OriginalArchive':'Not applicable: generated locally from the checked-in script',
        'ModifiedAssetReference':'04_Assets/S8/StationKit/'+p['fbx'],
        'VerificationStatus':'pending review',
        'VerificationEvidence':['07_Documentation/S8/Tools/build_station_kit.py','07_Documentation/S8/Evidence/Station/source-hashes.json'],
        'Notes':'Visible real-world features independently recreated at user request. Architectural dimensions/colors estimated. Does not certify exact likeness or railway engineering compliance.'})
(folder/'provenance.json').write_text(json.dumps(records,indent=2))
from validate_s8 import validate_record
for r in records:
    errors=validate_record('provenance',r)
    if errors:raise ValueError(errors)
print('Provenance records validated:',len(records))
