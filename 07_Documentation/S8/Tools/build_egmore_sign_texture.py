"""Bake shaped station lettering; no reference-photo pixels or font files shipped."""
from pathlib import Path
import sys, json, hashlib
import numpy as np
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).with_name('.art-python')))
import uharfbuzz as hb
import freetype
OUT = ROOT/'04_Assets/S8/EgmoreSignage/Textures'
OUT.mkdir(parents=True, exist_ok=True)
FONT = Path('C:/Windows/Fonts/Nirmala.ttc')
LINES = ['சென்னை எழும்பூர்', 'CHENNAI EGMORE', 'चेन्नै एषुंबूर']
records = []

def shape_line(text):
    face = hb.Face(FONT.read_bytes(), 1)
    font = hb.Font(face); font.scale = (192*64, 192*64)
    hb.ot_font_set_funcs(font)
    buf = hb.Buffer(); buf.add_str(text); buf.guess_segment_properties()
    hb.shape(font, buf)
    ids = [i.codepoint for i in buf.glyph_infos]
    assert ids and 0 not in ids, 'Missing glyph in '+text
    ft = freetype.Face(str(FONT), index=1); ft.set_pixel_sizes(0, 192)
    canvas = Image.new('L', (4000, 600)); cursor = 30
    for info, pos in zip(buf.glyph_infos, buf.glyph_positions):
        ft.load_glyph(info.codepoint, freetype.FT_LOAD_RENDER)
        g = ft.glyph; b = g.bitmap
        if b.width and b.rows:
            pixels = np.asarray(b.buffer, dtype=np.uint8).reshape(b.rows, abs(b.pitch))[:, :b.width]
            tile = Image.fromarray(pixels)
            xy = (round(cursor+pos.x_offset/64+g.bitmap_left), round(330-pos.y_offset/64-g.bitmap_top))
            # Composite coverage so overlapping marks never erase earlier glyphs.
            canvas.paste(255, xy, tile)
        cursor += pos.x_advance/64
    bbox = canvas.getbbox(); assert bbox
    records.append({'text':text,'glyphIds':ids,'script':buf.script,'missingGlyphs':0,'inkBounds':bbox})
    return canvas.crop(bbox)

rng = np.random.default_rng(20261005)
h,w = 1024,2048
y,x = np.mgrid[:h,:w]
grain = rng.normal(0,1.25,(h,w))
streak = 1.8*np.sin(x*.037)+.8*np.sin(x*.131)
edge = np.exp(-np.minimum.reduce([x,w-1-x,y,h-1-y])/18)*20
tone = grain+streak-edge
base = np.clip(np.array([225,178,30])[None,None,:]+tone[:,:,None],0,255).astype('uint8')
image = Image.fromarray(base)
draw = ImageDraw.Draw(image); draw.rectangle((18,18,w-19,h-19),outline=(111,94,38),width=5)
for text,cy in zip(LINES,[195,510,815]):
    ink=shape_line(text)
    factor=min(1790/ink.width,237/ink.height)
    ink=ink.resize((round(ink.width*factor),round(ink.height*factor)),Image.Resampling.LANCZOS)
    image.paste((24,26,25),((w-ink.width)//2,cy-ink.height//2),ink)
image.save(OUT/'T_EgmoreName_BC.png')
Image.new('RGB',(w,h),(128,128,255)).save(OUT/'T_EgmoreName_N.png')
rough=np.full((h,w,3),(255,192,0),dtype=np.uint8)
rough[:,:,1]=np.clip(192+grain*2,175,210).astype('uint8')
Image.fromarray(rough).save(OUT/'T_EgmoreName_ORM.png')
receipt={'reference':'https://commons.wikimedia.org/wiki/File:Chennai_Egmore.jpg',
 'referenceDate':'2011-11-18','referenceAuthor':'Superfast1111','referenceLicense':'CC-BY-SA-3.0',
 'interpretation':'Historical board spelling and language order; newly typeset, not a current-station survey',
 'font':'Windows Nirmala UI Bold, baked raster only; font file not redistributed',
 'fontSha256':hashlib.sha256(FONT.read_bytes()).hexdigest(),
 'shaper':'HarfBuzz '+hb.version_string(),'lines':records,'size':[w,h],
 'sourcePixelsUsed':False}
(OUT.parent/'lettering-receipt.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2),encoding='utf-8')
print('EGMORE_LETTERING_COMPLETE',len(records))
