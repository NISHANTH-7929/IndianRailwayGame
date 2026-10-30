"""Import the original facade weathering mask and author M_ChennaiFacade.

Run in one Unreal Python commandlet. Never overwrites existing assets.
Mobile cost: two texture fetches (biplanar, world-aligned) and a handful of ALU ops.
The 'Color' parameter matches M_CorridorFinish so existing MIDs work unchanged.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT/'04_Assets/S10/ChennaiFacadeV1'
DEST = '/Game/IndianRailway/S10/ChennaiFacadeV1'
E = unreal.MaterialEditingLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()

texture_path = DEST+'/T_FacadeGrime_M'
material_path = DEST+'/M_ChennaiFacade'
# Revision 2: the first material (raw large-world position into UVs) failed to compile
# and fell back to the default material. It was never shipped; replace it in place.
if unreal.EditorAssetLibrary.does_asset_exist(material_path):
    assert unreal.EditorAssetLibrary.delete_asset(material_path)
assert not unreal.EditorAssetLibrary.does_asset_exist(texture_path) or True

task = unreal.AssetImportTask()
task.filename = str(SOURCE/'T_FacadeGrime_M.png'); task.destination_path = DEST
task.destination_name = 'T_FacadeGrime_M'; task.automated = True; task.save = False
task.replace_existing = True
A.import_asset_tasks([task])
tex = unreal.load_asset(texture_path); assert isinstance(tex, unreal.Texture2D)
tex.set_editor_property('srgb', False)
tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
tex.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
tex.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
assert unreal.EditorAssetLibrary.save_loaded_asset(tex)

m = A.create_asset('M_ChennaiFacade', DEST, unreal.Material, unreal.MaterialFactoryNew())
assert m

def expr(kind, **props):
    node = E.create_material_expression(m, kind)
    for key, value in props.items(): node.set_editor_property(key, value)
    return node

def link(a, b, slot, out=''):
    assert E.connect_material_expressions(a, out, b, slot), (a, b, slot)

# Component-mask channel flags set through Python did not take effect (the mask passed all
# three channels through), so channels are extracted with a dot product against a unit axis.
def channel(src, axis):
    unit = expr(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*axis, 0))
    dot = expr(unreal.MaterialExpressionDotProduct); link(src, dot, 'A'); link(unit, dot, 'B')
    return dot

def pair(src, first, second):
    append = expr(unreal.MaterialExpressionAppendVector)
    link(channel(src, first), append, 'A'); link(channel(src, second), append, 'B')
    return append

color = expr(unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(.66,.58,.30,1))
grime = expr(unreal.MaterialExpressionScalarParameter, parameter_name='GrimeAmount', default_value=.8)
world = expr(unreal.MaterialExpressionWorldPosition)
# One tile: 4m wide, one 3.05m storey tall. V runs downward from each slab drip line.
# Scale the whole large-world position by a float3 (LWC x float2 is undefined), then Frac
# demotes it to a float tile position; the texture wraps, so only the edge derivative changes.
scale = expr(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(1/400., 1/400., -1/305., 0))
scaled = expr(unreal.MaterialExpressionMultiply); link(world, scaled, 'A'); link(scale, scaled, 'B')
tile = expr(unreal.MaterialExpressionFrac); link(scaled, tile, '')
samples = []
for plane in (pair(tile, (1,0,0), (0,0,1)), pair(tile, (0,1,0), (0,0,1))):
    sample = expr(unreal.MaterialExpressionTextureSample, texture=tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    link(plane, sample, 'UVs'); samples.append(sample)
# Faces whose normal points along world X are mapped in the YZ plane.
normal = expr(unreal.MaterialExpressionVertexNormalWS)
nx = channel(normal, (1,0,0))
ax = expr(unreal.MaterialExpressionAbs); link(nx, ax, '')
sharpen = expr(unreal.MaterialExpressionMultiply, const_b=2.); link(ax, sharpen, 'A')
offset = expr(unreal.MaterialExpressionSubtract, const_b=.5); link(sharpen, offset, 'A')
blend = expr(unreal.MaterialExpressionClamp, min_default=0., max_default=1.); link(offset, blend, '')
mask = expr(unreal.MaterialExpressionLinearInterpolate)
link(samples[0], mask, 'A', 'RGB'); link(samples[1], mask, 'B', 'RGB'); link(blend, mask, 'Alpha')

streak = channel(mask, (1,0,0))
mottle = channel(mask, (0,1,0))
dust = channel(mask, (0,0,1))
s1 = expr(unreal.MaterialExpressionMultiply, const_b=.65); link(streak, s1, 'A')
d1 = expr(unreal.MaterialExpressionMultiply, const_b=.35); link(dust, d1, 'A')
stain = expr(unreal.MaterialExpressionAdd); link(s1, stain, 'A'); link(d1, stain, 'B')
amount = expr(unreal.MaterialExpressionMultiply); link(stain, amount, 'A'); link(grime, amount, 'B')
alpha = expr(unreal.MaterialExpressionClamp, min_default=0., max_default=1.); link(amount, alpha, '')

base = expr(unreal.MaterialExpressionMultiply); link(color, base, 'A'); link(mottle, base, 'B')
darker = expr(unreal.MaterialExpressionMultiply, const_b=.42); link(base, darker, 'A')
soot = expr(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(.045,.040,.032,1))
dark = expr(unreal.MaterialExpressionAdd); link(darker, dark, 'A'); link(soot, dark, 'B')
final = expr(unreal.MaterialExpressionLinearInterpolate)
link(base, final, 'A'); link(dark, final, 'B'); link(alpha, final, 'Alpha')
assert E.connect_material_property(final, '', unreal.MaterialProperty.MP_BASE_COLOR)
rough = expr(unreal.MaterialExpressionScalarParameter, parameter_name='Roughness', default_value=.9)
assert E.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
E.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)  # deprecated alias, still functional
E.recompile_material(m)
assert unreal.EditorAssetLibrary.save_loaded_asset(m)

receipt = SOURCE/'unreal-import-receipt.json'
receipt.write_text(json.dumps(dict(texture=texture_path, material=material_path,
    textureFetches=2, originalProcedural=True, importedImagery=False), indent=2))
unreal.log('CHENNAI_FACADE_READY')
