"""Correct world-projected grain on the two new S9 materials only."""
from pathlib import Path
import json
import runpy
import time
import unreal

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S9/Evidence/TrackEnvironment-20261025-01/material-normal-correction.json'
assert not OUT.exists(), 'Preserve previous correction receipt'
E=unreal.MaterialEditingLibrary
rows=[]
try:
    for name in ('M_TrackBallast','M_DryGround'):
        path='/Game/IndianRailway/S9/TrackEnvironmentV1/Materials/'+name
        m=unreal.load_asset(path)
        normal=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_NORMAL)
        assert isinstance(normal,unreal.MaterialExpressionNormalize), path
        xyz=E.get_inputs_for_material_expression(m,normal)[0]
        assert isinstance(xyz,unreal.MaterialExpressionAppendVector)
        xy,one=E.get_inputs_for_material_expression(m,xyz)
        assert isinstance(xy,unreal.MaterialExpressionAppendVector)
        def node(kind,**props):
            n=E.create_material_expression(m,kind)
            for key,value in props.items():n.set_editor_property(key,value)
            return n
        def link(a,b,pin=''):assert E.connect_material_expressions(a,'',b,pin)
        gradient=node(unreal.MaterialExpressionAppendVector)
        zero=node(unreal.MaterialExpressionConstant,r=0.)
        link(xy,gradient,'A');link(zero,gradient,'B')
        transform=node(unreal.MaterialExpressionTransform,
            transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
            transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
        link(gradient,transform)
        projected=node(unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False)
        link(transform,projected);link(projected,xyz,'A')
        E.recompile_material(m)
        assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        rows.append(dict(asset=path,worldGradientTransformedToTangent=True,textureSamplesAdded=0))
    OUT.write_text(json.dumps(dict(passed=True,materials=rows),indent=2))
    runpy.run_path(str(Path(__file__).with_name('import_track_environment_meshes.py')),run_name='__main__')
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
