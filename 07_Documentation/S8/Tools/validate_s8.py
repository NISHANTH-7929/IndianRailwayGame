"""Offline S8 catalog validator. Standard library only; never modifies saves/assets.

The schema evaluator deliberately supports only the keywords used by checked-in
schemas and rejects unknown keywords (it is not a general JSON Schema engine).
"""
import argparse
import datetime
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / '03_UnrealProject/MyProject/Content/IndianRailway/Production'
DOC = ROOT / '07_Documentation/S8'
KEYWORDS = {'$schema','$id','type','properties','required','additionalProperties','items','enum','anyOf',
            'minimum','exclusiveMinimum','minLength','minItems','uniqueItems','pattern'}

def load(path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f'{path}: duplicate JSON key {key}')
            result[key] = value
        return result
    return json.loads(Path(path).read_text(encoding='utf-8-sig'), object_pairs_hook=unique,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError(f'non-finite JSON: {x}')))

def schema_errors(value, schema, path='$'):
    errors = []
    unknown = set(schema) - KEYWORDS
    if unknown:
        raise ValueError(f'Unsupported schema keywords: {unknown}')
    def bad(message): errors.append(f'{path}: {message}')
    if 'anyOf' in schema and not any(not schema_errors(value, option, path) for option in schema['anyOf']):
        bad('does not match any allowed type/shape'); return errors
    if 'enum' in schema and not any(type(value) is type(v) and value == v for v in schema['enum']):
        bad(f'expected one of {schema["enum"]}')
    types = schema.get('type', [])
    if isinstance(types, str): types = [types]
    matches = {'object':type(value) is dict, 'array':type(value) is list, 'string':type(value) is str,
               'integer':type(value) is int, 'number':type(value) in (int,float), 'boolean':type(value) is bool, 'null':value is None}
    if types and not any(matches[t] for t in types):
        bad(f'expected {types}, got {type(value).__name__}'); return errors
    if type(value) is dict:
        for key in schema.get('required', []):
            if key not in value: bad(f'missing required field {key}')
        for key, child in value.items():
            if key in schema.get('properties', {}): errors += schema_errors(child, schema['properties'][key], f'{path}.{key}')
            elif schema.get('additionalProperties') is False: bad(f'unsupported field {key}')
    if type(value) is list:
        if len(value) < schema.get('minItems', 0): bad('too few items')
        if schema.get('uniqueItems') and len({json.dumps(v,sort_keys=True) for v in value}) != len(value): bad('duplicate item')
        for i,child in enumerate(value): errors += schema_errors(child,schema.get('items',{}),f'{path}[{i}]')
    if type(value) is str:
        if len(value) < schema.get('minLength',0): bad('must not be empty')
        if 'pattern' in schema and not re.search(schema['pattern'],value): bad(f'expected pattern {schema["pattern"]}')
    if type(value) in (int,float):
        if not math.isfinite(value): bad('must be finite')
        if 'minimum' in schema and value < schema['minimum']: bad(f'must be >= {schema["minimum"]}')
        if 'exclusiveMinimum' in schema and value <= schema['exclusiveMinimum']: bad(f'must be > {schema["exclusiveMinimum"]}')
    return errors

def validate_record(kind, value, release=False, data=DATA):
    errors=schema_errors(value,load(data/'Schemas'/f'{kind}.schema.json'))
    if errors: return errors
    if kind == 'provenance':
        if value['AcquisitionDate'] is not None:
            try: datetime.date.fromisoformat(value['AcquisitionDate'])
            except ValueError: errors.append('AcquisitionDate: invalid calendar date')
        if release or value['VerificationStatus']=='approved':
            for key,expected in [('VerificationStatus','approved'),('CommercialUse','verified'),('ModificationPermission','verified')]:
                if value[key]!=expected: errors.append(f'{key}: release requires {expected}')
            if not value['VerificationEvidence']: errors.append('VerificationEvidence: verified rights require evidence')
            for key in ['License','Creator','Source','SourceReference','OriginalArchive','RedistributionRestrictions','AttributionRequirement']:
                if value[key].lower() in ('unknown','pending','none',''): errors.append(f'{key}: unresolved provenance')
            if value['AcquisitionDate'] is None: errors.append('AcquisitionDate: required for approval')
    if kind == 'cost' and value['finalAcceptedState']=='accepted' and not value['acceptanceEvidence']:
        errors.append('acceptanceEvidence: required for accepted cost record')
    if kind in ('readiness','repeatability'):
        for key,check in value['checks'].items():
            if check['status']=='passed' and not check['evidence']: errors.append(f'checks.{key}: pass requires evidence')
            if check['status']=='not-applicable' and not check['reason']: errors.append(f'checks.{key}: exemption requires reason')
        if kind=='repeatability':
            if value['firstContent']==value['secondContent']: errors.append('secondContent: must be distinct')
            if value['status']=='passed' and (not value['contentEvidence'] or any(c['status']!='passed' for c in value['checks'].values()) or value['sourceChangeManifest']=='pending'):
                errors.append('repeatability: all checks, source changes and integration evidence required')
    return errors

def compatible(saved, current):
    """Sidecar preflight only; existing S6/S7 binary decoder remains mandatory."""
    errors=[]
    for key in ['snapshotSchema','simulationContentId','serviceId','serviceVersion','serviceConfigurationPolicy','vehicleDefinitions','routeContract']:
        if key not in saved or saved[key] != current['saveCompatibility'][key]: errors.append(f'save.{key}: incompatible or missing; retain checkpoint')
    if saved.get('contentId') != current['contentId']: errors.append('save.contentId: unsupported catalog')
    if saved.get('contentVersion') not in [current['contentVersion']]+current['compatiblePreviousVersions']:
        errors.append('save.contentVersion: no explicit compatibility declaration')
    return errors

def validate_catalog(manifest, graphics, localization, release=False, data=DATA):
    errors=[]; warnings=[]
    for kind,value in [('manifest',manifest),('graphics',graphics),('localization',localization)]:
        errors += [f'{kind}: {e}' for e in validate_record(kind,value,data=data)]
    if errors: return errors,warnings
    def indexed(rows,key,label):
        result={}
        for row in rows:
            if row[key] in result: errors.append(f'{label}: duplicate {key} {row[key]}')
            result[row[key]]=row
        return result
    assets=indexed(manifest['assets'],'assetId','assets')
    chunks=indexed(manifest['chunks'],'id','chunks')
    vehicles=indexed(manifest['vehicleBindings'],'definitionId','vehicles')
    presentations=indexed(manifest['vehicleBindings'],'presentationId','vehicle presentations')
    stations=indexed(manifest['stationBindings'],'stationId','stations')
    audio=indexed(manifest['audioProfiles'],'profileId','audio')
    materials=indexed(manifest['materialLodProfiles'],'profileId','materials')
    indexed(manifest['formationBindings'],'formationId','formations')
    indexed(manifest['routeBindings'],'routeId','routes')
    keys=indexed(localization['strings'],'key','localization')
    all_ids=list(assets)+list(presentations)+list(stations)+list(audio)+list(materials)
    if len(all_ids)!=len(set(all_ids)): errors.append('catalog: logical ID collision across namespaces')
    if manifest['contentVersion'] in manifest['compatiblePreviousVersions']: errors.append('compatiblePreviousVersions: current version is not previous')
    current_tuple=tuple(map(int,manifest['contentVersion'].split('.')))
    if any(tuple(map(int,v.split('.')))>=current_tuple for v in manifest['compatiblePreviousVersions']): errors.append('compatiblePreviousVersions: future version is not previous')
    def reference(value,table,path):
        if value is not None and value not in table: errors.append(f'{path}: unresolved reference {value}')
    expected_chunks={1:('/Game/IndianRailway/S6/L_ChunkA','west'),2:('/Game/IndianRailway/S6/L_ChunkB','east')}
    if set(chunks)!=set(expected_chunks): errors.append('chunks: S6 adapter currently supports exactly 1 and 2')
    for key,row in chunks.items():
        if key in expected_chunks and (row['level'],row['rebindKey'])!=expected_chunks[key]: errors.append(f'chunks.{key}: differs from compiled S6 loading contract')
    for aid,a in assets.items():
        reference(a['chunk'],chunks,f'{aid}.chunk')
        reference(a['materialLodProfile'],materials,f'{aid}.materialLodProfile')
        for d in a['dependencies']: reference(d,assets,f'{aid}.dependencies')
        for d in a['audioDependencies']: reference(d,audio,f'{aid}.audioDependencies')
        for k in a['localizationKeys']: reference(k,keys,f'{aid}.localizationKeys')
        if a['external'] and a['provenance'] is None: errors.append(f'{aid}: external asset requires provenance')
        if a['provenance']:
            if a['provenance']['AssetId']!=aid or a['provenance']['Category']!=a['category']: errors.append(f'{aid}: provenance identity/category mismatch')
            errors += [f'{aid}: {e}' for e in validate_record('provenance',a['provenance'],release or a['state']=='accepted',data)]
            if a['provenance']['VerificationStatus']!='approved': warnings.append(f'{aid}: license {a["provenance"]["VerificationStatus"]}; not release-ready')
        if release and a['state']!='accepted': errors.append(f'{aid}: release requires accepted readiness evidence')
        if a['category'] not in ('audio','ui','formation','animation') and a['lods'][0]!='LOD0': errors.append(f'{aid}.lods: mesh must start with LOD0')
        if a['materialSlotLimit'] is not None and len(a['materials'])>a['materialSlotLimit']: errors.append(f'{aid}.materials: declared slot limit exceeded')
        if a['scaleCmPerUnit']!=1: warnings.append(f'{aid}: importer must bake scale to centimetres before acceptance')
        if len(a['lods'])==1 and a['lods'][0]=='LOD0': warnings.append(f'{aid}: single LOD needs measured justification; not a final budget')
        seen=set()
        for variant in a['variants']:
            pair=(variant['platform'],variant['tier'])
            if pair in seen: errors.append(f'{aid}: duplicate platform/tier variant')
            seen.add(pair)
            if variant['platform'] not in a['platforms'] or variant['tier'] not in a['tiers']: errors.append(f'{aid}: variant outside declared platforms/tiers')
    visiting=set(); visited=set()
    def visit(aid):
        if aid in visiting: errors.append(f'{aid}.dependencies: cycle'); return
        if aid in visited or aid not in assets: return
        visiting.add(aid)
        for d in assets[aid]['dependencies']: visit(d)
        visiting.remove(aid); visited.add(aid)
    for aid in assets: visit(aid)
    for row in vehicles.values():
        for slot in ['exterior','cab','bogie','wheel','lights','animation']: reference(row[slot],assets,f'vehicle.{row["definitionId"]}.{slot}')
        reference(row['audio'],audio,'vehicle.audio'); reference(row['materialLod'],materials,'vehicle.materialLod')
    for row in manifest['formationBindings']:
        for v in row['vehiclePresentationIds']: reference(v,presentations,'formation.vehiclePresentationIds')
    for row in stations.values():
        reference(row['chunk'],chunks,'station.chunk')
        for slot in ['platformVisualSet','signage','canopy','footbridge','furniture','lighting','peopleActivity','regionalKit','landmarks']: reference(row[slot],assets,f'station.{slot}')
        reference(row['ambientAudio'],audio,'station.ambientAudio')
        indexed(row['platforms'],'platformId','station platforms')
        for platform in row['platforms']:
            if platform['fromMetres']>=platform['toMetres']: errors.append('platform: fromMetres must precede toMetres')
    for row in manifest['routeBindings']:
        for c in row['chunks']: reference(c,chunks,'route.chunks')
        for s in row['stationIds']: reference(s,stations,'route.stationIds')
    for row in audio.values():
        for layer,binding in row['layers'].items():
            reference(binding['asset'],assets,f'audio.{layer}')
            if binding['asset'] in assets and assets[binding['asset']]['category']!='audio': errors.append(f'audio.{layer}: requires audio category')
    access=localization['accessibility']
    for k in access['controlLabelKeys']+[access['pauseKey'],access['resultKey'],access['warningKey']]: reference(k,keys,'accessibility')
    definitions=[dict(id=r['definitionId'],version=r['definitionVersion']) for r in vehicles.values()]
    if sorted(definitions,key=lambda x:x['id'])!=sorted(manifest['saveCompatibility']['vehicleDefinitions'],key=lambda x:x['id']): errors.append('saveCompatibility.vehicleDefinitions: disagrees with bindings')
    if not assets: warnings.append('Foundation catalog intentionally has no production assets; not representative-slice acceptance')
    if release and not assets: errors.append('release: empty foundation catalog is not release-ready')
    return errors,warnings

def run(data=DATA,release=False):
    errors,warnings=validate_catalog(load(data/'manifest.json'),load(data/'graphics.json'),load(data/'localization.json'),release,data)
    for kind in ['workload','profile']: errors += validate_record(kind,load(data/f'{kind}.json'),data=data)
    for kind in ['cost','readiness','repeatability','provenance']: errors += validate_record(kind,load(DOC/'Templates'/f'{kind}.json'),data=data)
    if release:
        for asset in load(data/'manifest.json')['assets']:
            path=DOC/'Reviews'/asset['assetId']/'readiness.json'
            if not path.exists(): errors.append(f'{asset["assetId"]}: missing final readiness record {path}'); continue
            record=load(path);record_errors=validate_record('readiness',record,data=data);errors+=record_errors
            if not record_errors:
                if record['assetId']!=asset['assetId']: errors.append(f'{path}: readiness identity mismatch')
                if any(c['status'] not in ('passed','not-applicable') for c in record['checks'].values()): errors.append(f'{path}: unresolved readiness checks')
                if record['checks']['finalApproval']['status']!='passed': errors.append(f'{path}: explicit final approval required')
    return dict(status='FAIL' if errors else 'PASS',mode='release' if release else 'foundation',errors=errors,warnings=warnings)

if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--data',type=Path,default=DATA); parser.add_argument('--release',action='store_true'); parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    try: result=run(args.data,args.release)
    except (ValueError,OSError,KeyError) as error: result=dict(status='FAIL',errors=[str(error)],warnings=[])
    text=json.dumps(result,indent=2,ensure_ascii=False)
    if args.output: args.output.parent.mkdir(parents=True,exist_ok=True); args.output.write_text(text+'\n',encoding='utf-8')
    print(text); raise SystemExit(result['status']!='PASS')
