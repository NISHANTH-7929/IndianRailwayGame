"""Refine the existing ground parent; original local procedural shader only."""
import unreal,importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('base',Path(__file__).with_name('import_locomotives.py'))
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
m=unreal.load_asset('/Game/IndianRailway/S8/Chennai/M_ChennaiGroundSurface')
assert m
b.MAT.delete_all_material_expressions(m)
color=b.expression(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.15,.105,.055,1))
# Centimetre world coordinates: broad damp/dry variation at several metres and
# fine grains at centimetre scale. No displaced balls or camera-facing decals.
macro=b.expression(m,unreal.MaterialExpressionNoise,scale=.0006,quality=1,levels=2,output_min=.84,output_max=1.)
grain=b.expression(m,unreal.MaterialExpressionNoise,scale=.12,quality=1,levels=1,output_min=.82,output_max=1.)
variation=b.expression(m,unreal.MaterialExpressionMultiply)
b.MAT.connect_material_expressions(macro,'',variation,'A');b.MAT.connect_material_expressions(grain,'',variation,'B')
mix=b.expression(m,unreal.MaterialExpressionMultiply)
b.MAT.connect_material_expressions(color,'',mix,'A');b.MAT.connect_material_expressions(variation,'',mix,'B')
b.connect(mix,m,unreal.MaterialProperty.MP_BASE_COLOR)
b.connect(b.expression(m,unreal.MaterialExpressionConstant,r=.96),m,unreal.MaterialProperty.MP_ROUGHNESS)
b.MAT.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
b.MAT.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('GROUND_SURFACE_REFINEMENT_IMPORTED')
