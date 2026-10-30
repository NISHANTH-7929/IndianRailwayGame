"""Import the CC0 Poly Haven maps and author the S10 corridor surface materials.

Run as an Unreal Python commandlet after fetch_polyhaven_v1.py. Re-running replaces the
materials (never the S8/S9 assets). Mobile budget per material is noted beside each.

World-aligned UVs: large-world positions are scaled, Frac'd over a long period (64 tiles)
and multiplied back, so the double->float conversion only creates a mip seam every 64
tiles instead of on every tile. Channels are extracted with dot products (component-mask
flags set through Python did not take effect in this engine build).
"""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S10/PolyHaven'
DEST = '/Game/IndianRailway/S10/PolyHavenV1'
E = unreal.MaterialEditingLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
GRIME = unreal.load_asset('/Game/IndianRailway/S10/ChennaiFacadeV1/T_FacadeGrime_M')
assert GRIME

def texture(asset, short):
    name = f'T_{asset}_{short}'; path = f'{DEST}/Textures/{name}'
    task = unreal.AssetImportTask(); task.filename = str(SOURCE/asset/f'{asset}_{short}_1k.jpg')
    task.destination_path = DEST + '/Textures'; task.destination_name = name
    task.automated = True; task.replace_existing = True; task.save = False
    A.import_asset_tasks([task]); t = unreal.load_asset(path); assert isinstance(t, unreal.Texture2D), path
    if short == 'nor_dx':
        t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP); t.set_editor_property('srgb', False)
    elif short == 'arm':
        t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS); t.set_editor_property('srgb', False)
    t.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP); t.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
    assert unreal.EditorAssetLibrary.save_loaded_asset(t); return t

T = {a: {s: texture(a, s) for s in ('diff', 'nor_dx', 'arm')} for a in
     ('red_laterite_soil_stones', 'sparse_grass', 'painted_plaster_wall', 'concrete_wall_008', 'asphalt_02')}

class Graph:
    def __init__(self, folder, name):
        path = f'{folder}/{name}'
        if unreal.EditorAssetLibrary.does_asset_exist(path): assert unreal.EditorAssetLibrary.delete_asset(path), path
        self.m = A.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()); assert self.m
        self.path = path
    def x(self, kind, **props):
        n = E.create_material_expression(self.m, kind)
        for k, v in props.items(): n.set_editor_property(k, v)
        return n
    def link(self, a, b, slot, out=''): assert E.connect_material_expressions(a, out, b, slot), (a, b, slot)
    def op(self, kind, a, b=None, **props):
        n = self.x(kind, **props); self.link(a, n, 'A' if b is not None or kind in (unreal.MaterialExpressionMultiply, unreal.MaterialExpressionAdd, unreal.MaterialExpressionSubtract) else '')
        if b is not None: self.link(b, n, 'B')
        return n
    def mul(self, a, b): return self.op(unreal.MaterialExpressionMultiply, a, b)
    def add(self, a, b): return self.op(unreal.MaterialExpressionAdd, a, b)
    def const(self, v): return self.x(unreal.MaterialExpressionConstant, r=v)
    def vec3(self, *v): return self.x(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*v, 0))
    def lerp(self, a, b, alpha):
        n = self.x(unreal.MaterialExpressionLinearInterpolate); self.link(a, n, 'A'); self.link(b, n, 'B'); self.link(alpha, n, 'Alpha'); return n
    def sat(self, a): n = self.x(unreal.MaterialExpressionClamp, min_default=0., max_default=1.); self.link(a, n, ''); return n
    def absv(self, a): n = self.x(unreal.MaterialExpressionAbs); self.link(a, n, ''); return n
    def frac(self, a): n = self.x(unreal.MaterialExpressionFrac); self.link(a, n, ''); return n
    def dot(self, a, axis): return self.op(unreal.MaterialExpressionDotProduct, a, self.vec3(*axis))
    def pair(self, a, first, second):
        n = self.x(unreal.MaterialExpressionAppendVector); self.link(self.dot(a, first), n, 'A'); self.link(self.dot(a, second), n, 'B'); return n
    def sample(self, tex, uv, kind=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
        n = self.x(unreal.MaterialExpressionTextureSample, texture=tex, sampler_type=kind); self.link(uv, n, 'UVs'); return n
    def param(self, name, v): return self.x(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=v)
    def colour(self, name, v): return self.x(unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(*v, 1))
    def tiles(self, tile_cm, scale=(1, 1, 1), period=64):
        """World position in tile units, wrapped every `period` tiles as a float3."""
        world = self.x(unreal.MaterialExpressionWorldPosition)
        k = self.vec3(*(s/(tile_cm*period) for s in scale))
        return self.mul(self.frac(self.mul(world, k)), self.const(period))
    def finish(self, base, rough=None, normal=None):
        assert E.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        assert E.connect_material_property(rough if rough is not None else self.const(.9), '', unreal.MaterialProperty.MP_ROUGHNESS)
        if normal is not None: assert E.connect_material_property(normal, '', unreal.MaterialProperty.MP_NORMAL)
        E.set_material_usage(self.m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
        E.recompile_material(self.m); assert unreal.EditorAssetLibrary.save_loaded_asset(self.m)
    def side_uv(self, wall):
        """One projection per face (no blend): YZ for faces facing world X, else XZ."""
        nrm = self.x(unreal.MaterialExpressionVertexNormalWS)
        sel = self.sat(self.add(self.mul(self.add(self.absv(self.dot(nrm, (1, 0, 0))), self.mul(self.absv(self.dot(nrm, (0, 1, 0))), self.const(-1))), self.const(1000)), self.const(.5)))
        return self.lerp(self.pair(wall, (1, 0, 0), (0, 0, 1)), self.pair(wall, (0, 1, 0), (0, 0, 1)), sel), nrm

rows = []
MASKS = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL

# ---- Ground: Tamil Nadu red laterite with weedy grass, broken up at 32 m and 12.8 m.
#      5 textures / 6 samples. VegetationCoverage matches the existing corridor MIDs.
g = Graph(DEST, 'M_ChennaiGround')
tile = g.tiles(200)                           # both source maps cover 2 m
uv = g.pair(tile, (1, 0, 0), (0, 1, 0))
macro1 = g.dot(g.sample(GRIME, g.mul(uv, g.const(1/16)), MASKS), (0, 1, 0))   # 32 m
macro2 = g.dot(g.sample(GRIME, g.mul(uv, g.const(1/6.4)), MASKS), (0, 1, 0))  # 12.8 m
m = g.sat(g.mul(g.add(g.mul(g.add(macro1, macro2), g.const(.5)), g.const(-.8)), g.const(5)))
coverage = g.param('VegetationCoverage', .5)
grass = g.sat(g.add(g.mul(g.add(coverage, g.mul(m, g.const(-1))), g.const(3)), g.const(.5)))
soil, weeds = T['red_laterite_soil_stones'], T['sparse_grass']
base = g.lerp(g.sample(soil['diff'], uv), g.sample(weeds['diff'], uv), grass)
shade = g.add(g.mul(m, g.const(.25)), g.const(.85))
base = g.mul(g.mul(base, shade), g.colour('Tint', (1, 1, 1)))
normal = g.lerp(g.sample(soil['nor_dx'], uv, NORMAL), g.sample(weeds['nor_dx'], uv, NORMAL), grass)
g.finish(base, g.const(.95), normal); rows.append(dict(material=g.path, textures=5, samples=6))

# ---- Facade (replaces M_ChennaiFacade in place): painted plaster detail tinted by Color,
#      monsoon streaks/dust from the S10 grime mask. 2 textures / 2 samples.
f = Graph('/Game/IndianRailway/S10/ChennaiFacadeV1', 'M_ChennaiFacade')
wall = f.tiles(1, (1/400, 1/400, -1/305), period=16)     # grime tile 4 m x one storey
wuv, _ = f.side_uv(wall)
grime = f.sample(GRIME, wuv, MASKS)
plaster = f.sample(T['painted_plaster_wall']['diff'], f.mul(wuv, f.const(2)))   # plaster covers 2 m
detail = f.mul(f.dot(plaster, (.3, .59, .11)), f.const(1.55))
colour = f.colour('Color', (.66, .58, .30))
paint = f.mul(f.mul(colour, f.dot(grime, (0, 1, 0))), detail)
stain = f.sat(f.mul(f.add(f.mul(f.dot(grime, (1, 0, 0)), f.const(.65)), f.mul(f.dot(grime, (0, 0, 1)), f.const(.35))), f.param('GrimeAmount', .8)))
dark = f.add(f.mul(paint, f.const(.42)), f.vec3(.045, .040, .032))
f.finish(f.lerp(paint, dark, stain), f.param('Roughness', .9)); rows.append(dict(material=f.path, textures=2, samples=2))

# ---- Concrete: boundary walls, slabs, posts, drains. One projection per face incl. tops.
#      2 textures / 2 samples.
c = Graph(DEST, 'M_ChennaiConcrete')
cwall = c.tiles(270)                                     # source covers 2.7 m
side, nrm = c.side_uv(cwall)
top = c.sat(c.add(c.mul(c.add(c.absv(c.dot(nrm, (0, 0, 1))), c.const(-.7)), c.const(1000)), c.const(.5)))
cuv = c.lerp(side, c.pair(cwall, (1, 0, 0), (0, 1, 0)), top)
conc = T['concrete_wall_008']
cbase = c.mul(c.sample(conc['diff'], cuv), c.colour('Color', (.92, .90, .86)))
c.finish(cbase, c.dot(c.sample(conc['arm'], cuv, MASKS), (0, 1, 0))); rows.append(dict(material=c.path, textures=2, samples=2))

# ---- Asphalt service roads: top projection, 3 m tiles. 2 textures / 2 samples.
r = Graph(DEST, 'M_ChennaiAsphalt')
ruv = r.pair(r.tiles(300), (1, 0, 0), (0, 1, 0))
asph = T['asphalt_02']
r.finish(r.mul(r.sample(asph['diff'], ruv), r.colour('Color', (.85, .85, .85))), r.const(.88), r.sample(asph['nor_dx'], ruv, NORMAL))
rows.append(dict(material=r.path, textures=2, samples=2))

(SOURCE / 'unreal-materials.json').write_text(json.dumps(dict(materials=rows), indent=2))
unreal.log_warning('POLYHAVEN_MATERIALS_READY %d' % len(rows))
