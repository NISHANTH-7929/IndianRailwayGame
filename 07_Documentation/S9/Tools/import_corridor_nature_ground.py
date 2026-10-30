"""Import authored verge relief and a texture/vertex blended dry terrain finish.

Reuses the saved V4 soil and original dry-verge texture; no source image edits.
Run in the full editor after import_corridor_vegetation_v1.py.
"""
from pathlib import Path
import json
import runpy
import time
import unreal

ROOT=Path(__file__).resolve().parents[3]
BASE='/Game/IndianRailway/S9/CorridorNatureV1'
TRACK='/Game/IndianRailway/S9/TrackEnvironmentV1'
OUT=ROOT/'07_Documentation/S9/Evidence/EnvironmentRealism-20261028-01/ground-import.json'
E=unreal.MaterialEditingLibrary

def node(m,kind,**props):
    n=E.create_material_expression(m,kind)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def link(a,b,pin='',out=''):assert E.connect_material_expressions(a,out,b,pin)
def mul(m,a,value):
    n=node(m,unreal.MaterialExpressionMultiply,const_b=value);link(a,n,'A');return n
def blend(m,a,b,t):
    n=node(m,unreal.MaterialExpressionLinearInterpolate)
    link(a,n,'A');link(b,n,'B');link(t,n,'Alpha');return n
def channel(m,n,name):
    mask=node(m,unreal.MaterialExpressionComponentMask,r=name=='R',g=name=='G',b=name=='B',a=False)
    link(n,mask);return mask

try:
    assert not OUT.exists()
    path=BASE+'/Materials/M_CorridorNatureGround'
    assert not unreal.EditorAssetLibrary.does_asset_exist(path)
    m=unreal.EditorAssetLibrary.duplicate_asset(TRACK+'/MaterialsV4/M_DryGround',path)
    assert m
    soil=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
    assert soil
    vertices=node(m,unreal.MaterialExpressionVertexColor)
    position=node(m,unreal.MaterialExpressionWorldPosition)
    xy=node(m,unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(position,xy)
    texture=unreal.load_asset(TRACK+'/TexturesV2/T_DryVerge_BC');assert texture
    grass=node(m,unreal.MaterialExpressionTextureSample,texture=texture,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    link(mul(m,xy,1/150.),grass,'UVs')
    # The authored masks taper to zero on tile boundaries. World projection
    # preserves continuous grain across curves and streaming-cell boundaries.
    surface=blend(m,soil,mul(m,grass,.30),mul(m,channel(m,vertices,'B'),.82))
    surface=blend(m,surface,mul(m,soil,1.12),channel(m,vertices,'R'))
    surface=blend(m,surface,mul(m,soil,.62),mul(m,channel(m,vertices,'G'),.65))
    assert E.connect_material_property(surface,'',unreal.MaterialProperty.MP_BASE_COLOR)
    E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    importer=runpy.run_path(str(Path(__file__).with_name('import_corridor_ground_v1.py')))
    mesh=importer['import_assets'](m)
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(dict(passed=True,mesh=mesh,material=m.get_path_name(),
        vertexMasks={'R':'worn maintenance path','G':'drainage depression','B':'dry low vegetation'},
        texturesReused=True,sourceImagesModified=False),indent=2))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
