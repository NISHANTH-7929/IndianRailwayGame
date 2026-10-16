from pathlib import Path
import json, math
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "v1.0.0" / "textures"
OUT.mkdir(parents=True, exist_ok=True)

SETS = {
    "red_stucco": ((126, 43, 31), 0.78, 0.0, 17),
    "cream_lime": ((224, 207, 166), 0.72, 0.0, 31),
    "platform_tile": ((143, 91, 68), 0.68, 0.0, 47),
    "concrete": ((132, 130, 123), 0.84, 0.0, 59),
    "blue_metal": ((45, 100, 130), 0.42, 0.62, 71),
    "dark_steel": ((55, 58, 60), 0.38, 0.72, 83),
    "ballast": ((91, 83, 74), 0.94, 0.0, 97),
    "soil": ((88, 68, 48), 0.91, 0.0, 109),
    "foliage": ((50, 81, 39), 0.82, 0.0, 127),
    "yellow_edge": ((216, 166, 35), 0.66, 0.0, 139),
}

def smooth(a, rounds=4):
    for _ in range(rounds):
        a = (a + np.roll(a,1,0)+np.roll(a,-1,0)+np.roll(a,1,1)+np.roll(a,-1,1))/5
    return a

records=[]
size=512
y,x=np.mgrid[0:size,0:size]
for name,(rgb,rough,metal,seed) in SETS.items():
    rng=np.random.default_rng(seed)
    fine=rng.normal(0,1,(size,size))
    coarse=smooth(rng.normal(0,1,(size,size)),10)
    pat=0.55*fine+2.8*coarse
    if name == "platform_tile":
        grout=((x%64)<3)|((y%32)<3)
        pat -= grout*18
    elif name == "blue_metal":
        pat += 4*np.sin(2*math.pi*x/18)
    elif name == "ballast":
        pat += (rng.random((size,size))>0.91)*22
    base=np.clip(np.array(rgb)[None,None,:]+pat[:,:,None],0,255).astype(np.uint8)
    bc=OUT/f"T_{name}_BC.png"; Image.fromarray(base,"RGB").save(bc,optimize=True)
    h=smooth(pat,2)
    gy,gx=np.gradient(h)
    n=np.dstack((-gx*0.055,-gy*0.055,np.ones_like(h)))
    n=n/np.linalg.norm(n,axis=2,keepdims=True)
    normal=np.clip((n*0.5+0.5)*255,0,255).astype(np.uint8)
    nm=OUT/f"T_{name}_N.png"; Image.fromarray(normal,"RGB").save(nm,optimize=True)
    ao=np.clip(238-pat*0.7,170,255)
    rr=np.clip(rough*255+pat*0.45,20,250)
    mm=np.full_like(ao,metal*255)
    orm=np.dstack((ao,rr,mm)).astype(np.uint8)
    op=OUT/f"T_{name}_ORM.png"; Image.fromarray(orm,"RGB").save(op,optimize=True)
    records.append({"set":name,"resolution":[size,size],"files":[p.name for p in (bc,nm,op)],"method":"project-authored seeded procedural tiling maps"})

(OUT/"texture_manifest.json").write_text(json.dumps(records,indent=2),encoding="utf-8")
print(f"generated {len(records)} PBR sets in {OUT}")
