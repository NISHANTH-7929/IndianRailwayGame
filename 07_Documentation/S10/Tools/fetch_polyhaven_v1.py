"""Download the CC0 Poly Haven texture set used by the S10 corridor materials.

Writes only 04_Assets/S10/PolyHaven/<asset>/ plus provenance.json. 1K JPG maps keep the
Android texture budget small: colour, DirectX normal and packed AO/Roughness/Metal (ARM).
Poly Haven assets are CC0 (https://polyhaven.com/license); attribution is recorded anyway.
"""
import hashlib, json, urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S10/PolyHaven'
ASSETS = {
    'red_laterite_soil_stones': 'Corridor backland and verges: Tamil Nadu red laterite soil',
    'sparse_grass': 'Weedy grass layer blended over the laterite',
    'painted_plaster_wall': 'Detail for painted residential walls (tinted per building)',
    'concrete_wall_008': 'Boundary walls, drains and platform faces',
    'asphalt_02': 'Service roads alongside the railway',
}
MAPS = {'Diffuse': 'diff', 'nor_dx': 'nor_dx', 'arm': 'arm'}
RESOLUTION = '1k'
UA = {'User-Agent': 'IndianRailwayGame-asset-fetch/1.0'}

def get(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=120) as r:
        return r.read()

records = []
for asset, purpose in ASSETS.items():
    files = json.loads(get(f'https://api.polyhaven.com/files/{asset}'))
    info = json.loads(get(f'https://api.polyhaven.com/info/{asset}'))
    folder = OUT / asset; folder.mkdir(parents=True, exist_ok=True)
    maps = {}
    for key, short in MAPS.items():
        entry = files[key][RESOLUTION]['jpg']
        data = get(entry['url'])
        assert len(data) == entry['size'], (asset, key)
        assert entry.get('md5') is None or hashlib.md5(data).hexdigest() == entry['md5'], (asset, key)
        path = folder / f'{asset}_{short}_{RESOLUTION}.jpg'
        path.write_bytes(data)
        maps[short] = dict(file=str(path.relative_to(ROOT)), url=entry['url'], bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    records.append(dict(asset=asset, purpose=purpose, source=f'https://polyhaven.com/a/{asset}', license='CC0 1.0',
                        authors=list(info.get('authors', {}).keys()), real_size_m=[d/1000 for d in info.get('dimensions', [])],
                        resolution=RESOLUTION, maps=maps))
    print('fetched', asset)
(OUT / 'provenance.json').write_text(json.dumps(dict(source='Poly Haven', license='CC0 1.0 (https://polyhaven.com/license)', assets=records), indent=2))
print('POLYHAVEN_READY', len(records))
