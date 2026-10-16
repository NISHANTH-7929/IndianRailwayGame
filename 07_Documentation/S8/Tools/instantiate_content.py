"""Create pending review/cost/provenance or repeatability records; no assets created."""
import argparse
import copy
import json
import re
from pathlib import Path
from validate_s8 import DOC,validate_record,load

def instantiate(asset_id,category,output,second=None):
    if not re.fullmatch(r'[A-Z][A-Za-z0-9]*(\.[A-Za-z0-9]+){2,}',asset_id): raise ValueError('Invalid stable AssetId')
    records={}
    for kind in ['cost','readiness','provenance']:
        record=copy.deepcopy(load(DOC/'Templates'/f'{kind}.json'))
        record['AssetId' if kind=='provenance' else 'assetId']=asset_id
        if kind=='cost': record['category']=category
        if kind=='provenance': record['Category']=category
        errors=validate_record(kind,record)
        if errors: raise ValueError('; '.join(errors))
        records[kind]=record
    if second:
        record=load(DOC/'Templates/repeatability.json');record.update(firstContent=asset_id,secondContent=second)
        errors=validate_record('repeatability',record)
        if errors: raise ValueError('; '.join(errors))
        records['repeatability']=record
    output=Path(output)
    for kind in records:
        if (output/f'{kind}.json').exists(): raise ValueError(f'{kind}.json already exists; refusing to overwrite review evidence')
    output.mkdir(parents=True,exist_ok=True)
    for kind,record in records.items(): (output/f'{kind}.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    return records

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('asset_id');p.add_argument('category');p.add_argument('output',type=Path);p.add_argument('--second');a=p.parse_args()
    try: instantiate(a.asset_id,a.category,a.output,a.second)
    except (ValueError,OSError) as error: p.exit(1,str(error)+'\n')
