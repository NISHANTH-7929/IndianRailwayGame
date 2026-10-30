"""Project-native material derivatives. Never overwrite the authored S8 parents.

Run after the packaged journey exits, in one Unreal Python commandlet.
No mesh, UV, collision, simulation or save changes. Retain contact/dirt graphs.
"""
import unreal, json, importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('material_base', ROOT/'07_Documentation/S8/Tools/import_locomotives.py')
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)
DEST = '/Game/IndianRailway/S9/JourneyMaterials'
receipt = ROOT/'07_Documentation/S9/Evidence/JourneyRealism-20261016-01/materials.json'
assert not receipt.exists(), 'Use a fresh revision instead of overwriting evidence'
rows = []

def derivative(source):
    original = unreal.load_asset(source)
    assert original, source
    destination = DEST + '/' + original.get_name()
    assert not unreal.EditorAssetLibrary.does_asset_exist(destination), destination
    material = unreal.EditorAssetLibrary.duplicate_asset(source, destination)
    assert material, destination
    rows.append(dict(source=source, destination=destination))
    return material

cab = '/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/'
# Preserve the baked contact shading and albedo textures. Only the finish is
# adjusted: painted enamel is less chalky than rubber and charcoal panels.
for name, factor in [('DeskEnamel', .72), ('DeskEnamel_StaticAO', .72),
                     ('CabIvory', .88), ('CabIvory_StaticAO', .88),
                     ('PanelCharcoal', .90), ('PanelCharcoal_StaticAO', .90)]:
    if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name):
        rows.append(dict(source=cab+name,destination=DEST+'/'+name,retainedFromPartialImport=True))
        continue
    material = derivative(cab + name)
    node = b.MAT.get_material_property_input_node(material, unreal.MaterialProperty.MP_ROUGHNESS)
    assert node, name
    multiply = b.expression(material, unreal.MaterialExpressionMultiply, const_b=factor)
    assert b.MAT.connect_material_expressions(node, '', multiply, 'A')
    bounded = b.expression(material, unreal.MaterialExpressionClamp, min_default=.24, max_default=.92)
    assert b.MAT.connect_material_expressions(multiply, '', bounded, '')
    b.connect(bounded, material, unreal.MaterialProperty.MP_ROUGHNESS)
    b.MAT.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)

# Preserve clear forward visibility. Grazing panes carry slightly more tint;
# there is no fake opaque windscreen or refractive distortion of signal lights.
for name, base, edge in [('WindowGlass', .025, .115), ('InstrumentGlass', .035, .075)]:
    if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name):
        rows.append(dict(source=cab+name,destination=DEST+'/'+name,retainedFromPartialImport=True))
        continue
    material = derivative(cab + name)
    fresnel = b.expression(material, unreal.MaterialExpressionFresnel)
    scale = b.expression(material, unreal.MaterialExpressionMultiply, const_b=edge)
    offset = b.expression(material, unreal.MaterialExpressionAdd, const_b=base)
    assert b.MAT.connect_material_expressions(fresnel, '', scale, 'A')
    assert b.MAT.connect_material_expressions(scale, '', offset, 'A')
    b.connect(offset, material, unreal.MaterialProperty.MP_OPACITY)
    b.MAT.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)

# Existing coach materials already carry baked dirt and detail. Instance edits
# preserve those graphs while separating coated glass and worn paint finishes.
coach = '/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/Materials/'
# Resolve actual paths from the body slots instead of assuming importer folders.
body = unreal.load_asset('/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/SM_LHB_BodyV2')
assert body
settings = {'LHB_GlassV2': {'Roughness': .12, 'CoatRoughness': .08, 'GlazingEdgeTint': .12},
            'LHB_PaintRedV2': {'Roughness': .34, 'ClearCoat': .30, 'CoatRoughness': .30},
            'LHB_PaintGreyV2': {'Roughness': .48, 'ClearCoat': .20, 'CoatRoughness': .36}}
seen = set()
for slot in range(len(body.get_editor_property('static_materials'))):
    parent = body.get_material(slot)
    name = parent.get_name() if parent else ''
    if name not in settings or name in seen: continue
    seen.add(name)
    assert not unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name)
    instance = b.ASSETS.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    b.MAT.set_material_instance_parent(instance, parent)
    for key, value in settings[name].items():
        b.MAT.set_material_instance_scalar_parameter_value(instance, key, value)
    b.MAT.update_material_instance(instance)
    for key, value in settings[name].items():
        actual=b.MAT.get_material_instance_scalar_parameter_value(instance,key)
        assert abs(actual-value)<.0001, (name,key,actual,value)
    assert unreal.EditorAssetLibrary.save_loaded_asset(instance)
    rows.append(dict(source=parent.get_path_name(), destination=instance.get_path_name(), parameters=settings[name]))
assert seen == set(settings), str(seen)
receipt.write_text(json.dumps(dict(materials=rows, limitation='Representative finish tuning; coach glazing remains opaque without a modelled interior.'), indent=2))
unreal.log('S9_JOURNEY_MATERIALS_READY')
