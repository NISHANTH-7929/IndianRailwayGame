"""Pixel measurements only; no image edits or renderer actions."""
from PIL import Image
import numpy as np
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/StabilizationDesktop-20261011/Lighting'
regions={'whole':(0,0,1600,900),'canopy':(380,90,1000,280),
         'sign':(735,349,860,397),'floor':(830,700,1100,870)}
base=np.asarray(Image.open(OUT/'baseline_saved.png').convert('RGB'),dtype=float)
report={'units':'Rec709 weighted encoded sRGB values, 0..255; NOT physical luminance',
        'regions_xyxy':regions,'images':{}}
for path in sorted(OUT.glob('*.png')):
    image=np.asarray(Image.open(path).convert('RGB'),dtype=float)
    row={'mean_absolute_rgb_difference_from_baseline':float(np.abs(image-base).mean()),
         'identical_pixels_to_baseline':bool(np.array_equal(base,image)), 'mean_luma':{}}
    for name,(x1,y1,x2,y2) in regions.items():
        row['mean_luma'][name]=float((image[y1:y2,x1:x2]@np.array([.2126,.7152,.0722])).mean())
    report['images'][path.stem]=row
(OUT/'pixel-metrics.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
