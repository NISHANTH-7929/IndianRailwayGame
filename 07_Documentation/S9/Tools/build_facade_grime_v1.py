"""Original procedural weathering mask for painted Chennai facades (no imported imagery).

R: monsoon rain streaks hanging from slab/parapet lines (1 = stained).
G: sun-faded paint mottling (multiplier, ~0.8-1.0).
B: soot/dust accumulation toward the bottom of each storey band.
Tiles in both directions. One tile spans 4 m wide x 3.05 m (one storey) tall in-world.
"""
import json, hashlib
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT/'04_Assets/S10/ChennaiFacadeV1'
W, H = 512, 512
rng = np.random.default_rng(20261030)

def tile_noise(octaves, base, persistence=.5):
    """Tileable value noise by summing wrapped bicubic-upsampled random grids."""
    acc = np.zeros((H, W)); amp = 1.; total = 0.
    for o in range(octaves):
        n = base * 2**o
        g = rng.random((n, n))
        img = Image.fromarray((g*255).astype(np.uint8))
        # Wrap by tiling 3x3 then cropping the centre so the edges agree.
        big = Image.new('L', (n*3, n*3))
        for i in range(3):
            for j in range(3): big.paste(img, (i*n, j*n))
        up = big.resize((W*3, H*3), Image.BICUBIC)
        acc += amp*np.asarray(up, float)[H:2*H, W:2*W]/255.; total += amp; amp *= persistence
    return acc/total

# Rain streaks: columns with random strength and length, starting at the top of the band
# (under the slab drip line), fading downward; broken by noise so they are not ruled lines.
streak = np.zeros((H, W))
x = np.arange(W)
for _ in range(70):
    cx = rng.uniform(0, W); width = rng.uniform(2, 14); strength = rng.uniform(.25, 1.)
    length = rng.uniform(.15, .95)*H
    d = np.minimum(np.abs(x-cx), W-np.abs(x-cx))
    col = np.exp(-(d/width)**2)*strength
    y = np.arange(H)[:, None]
    fade = np.clip(1-y/length, 0, 1)**1.4
    streak = np.maximum(streak, col[None, :]*fade)
breakup = tile_noise(5, 4)
streak = np.clip(streak*(.55+.9*breakup)*1.1, 0, 1)
# Drip line: a soft dark band just under the slab edge.
y = np.arange(H)[:, None]/H
streak = np.clip(streak + .35*np.exp(-(y/.035)**2)*(.6+.4*breakup), 0, 1)

mottle = .80 + .20*tile_noise(6, 3, .55)
dust = np.clip((y-.62)/.38, 0, 1)**2 * (.5+.5*tile_noise(4, 6))
dust = np.broadcast_to(dust, (H, W))

rgb = np.stack([streak, mottle, dust], -1)
OUT.mkdir(parents=True, exist_ok=True)
png = OUT/'T_FacadeGrime_M.png'
Image.fromarray((np.clip(rgb, 0, 1)*255).astype(np.uint8), 'RGB').save(png)
receipt = dict(revision='chennai-facade-v1', originalProcedural=True, importedImagery=False,
               channels=dict(R='rain streaks', G='paint mottle multiplier', B='storey-base dust'),
               tileMetres=[4.0, 3.05], seed=20261030,
               sha256=hashlib.sha256(png.read_bytes()).hexdigest())
(OUT/'texture-receipt.json').write_text(json.dumps(receipt, indent=2))
print('FACADE_GRIME_READY', png)
