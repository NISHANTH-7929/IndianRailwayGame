"""Acquire bounded OSM reference extracts; never manufacture missing topology.

Raw XML is retained for provenance and reproducibility. OSM data is ODbL 1.0;
derived data must retain attribution and applicable database obligations.
No textures, imagery or marketplace assets are acquired by this tool.
"""
import hashlib
import json
import urllib.request
import urllib.error
import xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '06_Routes/ChennaiEgmoreTambaram/References/OSM'
# Bounded reference areas only, not a centreline or engineering specification.
BOXES = [
    (80.230,13.065,80.265,13.085),
    (80.225,13.045,80.250,13.065),
    (80.215,13.025,80.240,13.045),
    (80.200,13.005,80.235,13.025),
    (80.185,12.985,80.215,13.005),
    (80.165,12.965,80.200,12.985),
    (80.140,12.945,80.180,12.965),
    (80.115,12.915,80.150,12.945),
    # Pallavaram lies west of the original airport band; close that reference gap.
    (80.150,12.965,80.165,12.980),
]

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    records, nodes, ways = [], {}, {}
    queue = [(f'corridor-{i:02d}',box,0) for i,box in enumerate(BOXES)]
    while queue:
        stem,bbox,depth = queue.pop(0)
        url = 'https://api.openstreetmap.org/api/0.6/map?bbox=' + ','.join(map(str,bbox))
        path = OUT / f'{stem}.osm'
        if not path.exists() and (OUT/f'{stem}a.osm').exists() and (OUT/f'{stem}b.osm').exists():
            west,south,east,north=bbox
            mid=(south+north)/2
            queue[0:0]=[(stem+'a',(west,south,east,mid),depth+1),(stem+'b',(west,mid,east,north),depth+1)]
            continue
        if not path.exists():
            req = urllib.request.Request(url,headers={'User-Agent':'IndianRailwayGame-S8-reference-research/1.0'})
            try:
                with urllib.request.urlopen(req,timeout=60) as response:
                    raw = response.read()
            except urllib.error.HTTPError as error:
                reason=error.read().decode('utf-8',errors='replace')
                if error.code==400 and 'nodes' in reason.lower() and depth<4:
                    west,south,east,north=bbox
                    mid=(south+north)/2
                    queue[0:0]=[(stem+'a',(west,south,east,mid),depth+1),(stem+'b',(west,mid,east,north),depth+1)]
                    print(f'{stem}: split to respect OSM node limit',flush=True)
                    continue
                raise RuntimeError(f'{url}: {error.code} {reason}') from error
            doc = ET.fromstring(raw)
            if doc.tag != 'osm':
                raise ValueError('OSM response required')
            path.write_bytes(raw)
        raw = path.read_bytes()
        doc = ET.fromstring(raw)
        records.append({'url':url,'file':path.name,'sha256':hashlib.sha256(raw).hexdigest(),
                        'osmBase':doc.find('meta').attrib if doc.find('meta') is not None else {}})
        for node in doc.findall('node'):
            nodes[int(node.attrib['id'])] = {'lat':float(node.attrib['lat']),'lon':float(node.attrib['lon']),
                                            'tags':{t.attrib['k']:t.attrib['v'] for t in node.findall('tag')}}
        for way in doc.findall('way'):
            tags = {t.attrib['k']:t.attrib['v'] for t in way.findall('tag')}
            if 'railway' in tags or tags.get('waterway') or tags.get('bridge'):
                ways[int(way.attrib['id'])] = {'tags':tags,'nodes':[int(n.attrib['ref']) for n in way.findall('nd')]}
        print(f'{path.name}: {len(raw)} bytes; {len(ways)} reference ways',flush=True)
    used = {n for w in ways.values() for n in w['nodes']}
    used.update(n for n,value in nodes.items() if value['tags'].get('railway') in ('station','halt'))
    missing = sorted(used - nodes.keys())
    document = {'attribution':'© OpenStreetMap contributors','licence':'ODbL-1.0',
                'copyrightUrl':'https://www.openstreetmap.org/copyright',
                'createdUtc':datetime.now(timezone.utc).isoformat(),'sources':records,
                'nodes':{str(n):nodes[n] for n in sorted(used) if n in nodes},
                'ways':{str(w):ways[w] for w in sorted(ways)},'missingNodes':missing}
    (OUT/'corridor-reference.json').write_text(json.dumps(document,indent=2),encoding='utf-8')
    print(f'Complete: {len(ways)} ways; missing nodes={len(missing)}',flush=True)

if __name__ == '__main__':
    main()
