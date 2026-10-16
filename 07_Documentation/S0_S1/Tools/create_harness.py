"""Editor-only S1 fixture authoring. No runtime Python or railway systems.

Run using UnrealEditor-Cmd -EnablePlugins=PythonScriptPlugin
-run=pythonscript -script=<this file>. Refuses to overwrite the map.
"""
import json
from pathlib import Path
import unreal

ROOT = '/Game/IndianRailway/Validation'
MAP = ROOT + '/L_AndroidHarness'
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError('Harness already exists; validate it instead of overwriting it.')

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.new_level(MAP, False), 'Failed to create ordinary non-partitioned level'
tools = unreal.AssetToolsHelpers.get_asset_tools()

def material(name, color):
    asset = tools.create_asset(name, ROOT + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    assert asset is not None
    asset.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    value = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant3Vector)
    value.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(value, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset

floor_mat = material('M_S1_Floor', (0.035, 0.055, 0.085))
left_mat = material('M_S1_Left', (0.04, 0.55, 0.3))
right_mat = material('M_S1_Right', (0.7, 0.18, 0.025))
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
assert cube

def box(name, position, scale, mat):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
    actor.set_actor_label(name)
    component = actor.static_mesh_component
    component.set_static_mesh(cube)
    component.set_material(0, mat)
    component.set_editor_property('cast_shadow', False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

box('S1_Floor', (0, 0, -25), (30, 30, 0.5), floor_mat)
box('S1_LeftMarker', (350, -300, 100), (1, 1, 2), left_mat)
box('S1_RightMarker', (350, 300, 100), (1, 1, 2), right_mat)
box('S1_DepthMarker', (900, 0, 100), (1, 1, 2), left_mat)

def label(name, text, height, size):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(500, 0, height), unreal.Rotator(pitch=0, yaw=180, roll=0))
    actor.set_actor_label(name)
    component = actor.text_render
    component.set_text(text)
    component.set_world_size(size)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)

label('S1_Title', 'INDIAN RAILWAY | S1 ANDROID HARNESS', 340, 23)
label('S1_InputGuide', 'LEFT STICK: MOVE    RIGHT STICK: LOOK', 290, 17)
label('S1_Scope', 'ENGINEERING BASELINE ONLY - NO RAILWAY GAMEPLAY', 250, 13)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-600, 0, 160), unreal.Rotator(0, 0, 0))
start.set_actor_label('S1_PlayerStart')
assert levels.save_current_level(), 'Map save failed'

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
summary = {
    'map': MAP,
    'authoring': 'LevelEditorSubsystem.new_level(..., False)',
    'actors': [{'name': a.get_actor_label(), 'class': a.get_class().get_name()} for a in actors.get_all_level_actors()],
    'runtime': 'Engine GameModeBase / DefaultPawn; stock virtual joysticks; no runtime Python',
    'result': 'CREATED_NOT_DEVICE_VALIDATED',
}
evidence = Path(__file__).resolve().parent.parent / 'Evidence' / 'harness-map.json'
evidence.write_text(json.dumps(summary, indent=2), encoding='utf-8')
unreal.log('S1_HARNESS_CREATED ' + json.dumps(summary))
