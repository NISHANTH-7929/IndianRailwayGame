"""Create a dry olive/straw ground blend for open land outside railway formation."""
from pathlib import Path
import json
import time
import unreal
ROOT=Path(__file__).resolve().parents[3]
BASE='/Game/IndianRailway/S9/CorridorNatureV1/Materials'
OUT=ROOT/'07_Documentation/S9/Evidence/EnvironmentRealism-20261028-01/outer-ground-import.json'
E=unreal.MaterialEditingLibrary
try:
    assert not OUT.exists()
    path=BASE+'/M_CorridorOuterGround'
    assert not unreal.EditorAssetLibrary.does_asset_exist(path)
    m=unreal.EditorAssetLibrary.duplicate_asset('/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV4/M_DryGround',path)
    assert m
    surface=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
    assert isinstance(surface,unreal.MaterialExpressionLinearInterpolate)
    grass=[n for n in E.get_material_expressions(m) if isinstance(n,unreal.MaterialExpressionTextureSample)
           and n.get_editor_property('texture') and n.get_editor_property('texture').get_name()=='T_DryVerge_BC']
    assert len(grass)==1
    tint=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
    tint.set_editor_property('constant',unreal.LinearColor(.24,.30,.14,1))
    multiply=E.create_material_expression(m,unreal.MaterialExpressionMultiply)
    assert E.connect_material_expressions(grass[0],'',multiply,'A')
    assert E.connect_material_expressions(tint,'',multiply,'B')
    assert E.connect_material_expressions(multiply,'',surface,'B')
    E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    OUT.write_text(json.dumps(dict(passed=True,asset=m.get_path_name(),
        reusedDryVergeTexture=grass[0].get_editor_property('texture').get_path_name(),
        vegetationTint=[.24,.30,.14],sourceImagesModified=False),indent=2))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
