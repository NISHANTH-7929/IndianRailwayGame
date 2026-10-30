"""Original, world-space corridor finishes; retain the authored S8 parents."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
DEST = '/Game/IndianRailway/S9/CorridorFinishV2'
E = unreal.MaterialEditingLibrary
receipt = ROOT/'07_Documentation/S9/Evidence/Scenery-20261023-01/surfaces-v2.json'
assert not receipt.exists(), 'Retain the previous receipt; use a new revision'
rows = []

def expr(m, kind, **props):
    node = E.create_material_expression(m, kind)
    for key, value in props.items(): node.set_editor_property(key, value)
    return node

def link(a, b, slot):
    assert E.connect_material_expressions(a, '', b, slot)

for name, source, ground in [
    ('M_CorridorGround', 'M_ChennaiGroundSurface', True),
    ('M_CorridorFinish', 'M_SceneryTint', False),
]:
    path = DEST+'/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(path), path
    m = unreal.EditorAssetLibrary.duplicate_asset('/Game/IndianRailway/S8/Chennai/'+source, path)
    assert m
    E.delete_all_material_expressions(m)
    color = expr(m, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(.18,.14,.09,1))
    # World coordinates preserve grain size across differently scaled cubes.
    # Ground keeps the same two noise evaluations as its original parent;
    # painted surfaces use only one broad, low contrast variation.
    macro = expr(m, unreal.MaterialExpressionNoise, scale=.0009 if ground else .008,
                 quality=1, levels=2 if ground else 1,
                 output_min=.58 if ground else .82, output_max=1.08 if ground else 1.)
    variation = macro
    if ground:
        grain = expr(m, unreal.MaterialExpressionNoise, scale=.12, quality=1, levels=1, output_min=.83, output_max=1.)
        variation = expr(m, unreal.MaterialExpressionMultiply)
        link(macro, variation, 'A'); link(grain, variation, 'B')
    surface = color
    if ground:
        # Reuse the existing macro noise: no third ground noise evaluation.
        # Only the broad land family opts in; ballast/silt/grass keep their tint.
        mask = expr(m, unreal.MaterialExpressionSubtract, const_b=.72)
        link(macro, mask, 'A')
        gain = expr(m, unreal.MaterialExpressionMultiply, const_b=4.)
        link(mask, gain, 'A')
        bounded = expr(m, unreal.MaterialExpressionClamp, min_default=0., max_default=1.)
        link(gain, bounded, '')
        coverage = expr(m, unreal.MaterialExpressionScalarParameter, parameter_name='VegetationCoverage', default_value=0.)
        amount = expr(m, unreal.MaterialExpressionMultiply)
        link(bounded, amount, 'A'); link(coverage, amount, 'B')
        grass = expr(m, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(.04,.08,.018,1))
        surface = expr(m, unreal.MaterialExpressionLinearInterpolate)
        link(color, surface, 'A'); link(grass, surface, 'B'); link(amount, surface, 'Alpha')
    mix = expr(m, unreal.MaterialExpressionMultiply)
    link(surface, mix, 'A'); link(variation, mix, 'B')
    assert E.connect_material_property(mix, '', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = expr(m, unreal.MaterialExpressionScalarParameter, parameter_name='Roughness', default_value=.96 if ground else .86)
    assert E.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    E.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    E.recompile_material(m)
    assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    rows.append(dict(asset=path, source=source, ground=ground))
receipt.parent.mkdir(parents=True, exist_ok=True)
receipt.write_text(json.dumps(dict(materials=rows, originalProcedural=True, importedImagery=False), indent=2))
unreal.log('CORRIDOR_FINISH_READY')
