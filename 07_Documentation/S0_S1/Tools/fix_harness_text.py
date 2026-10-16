"""Correct the stock lit text material for the deliberately unlit S1 fixture."""
import unreal

root = '/Game/IndianRailway/Validation'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(root + '/L_AndroidHarness')
path = root + '/Materials/M_S1_Text'
mat = unreal.load_asset(path)
if mat is None:
    mat = unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque', path)
assert mat
base = unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
assert base, 'Expected stock font base-color input; inspect rather than assume'
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
assert unreal.MaterialEditingLibrary.connect_material_property(base, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
count = 0
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.TextRenderActor):
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)
        actor.text_render.set_text_material(mat)
        count += 1
assert count == 3
assert levels.save_current_level()
unreal.log('S1_TEXT_MATERIAL_FIXED labels=3 shading=unlit')
