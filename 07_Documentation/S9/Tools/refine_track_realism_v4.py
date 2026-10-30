"""Correct FBX's V inversion and replace flat vegetation colour with dry-verge detail."""
from pathlib import Path
import hashlib
import json
import time
import unreal
ROOT=Path(__file__).resolve().parents[3]
BASE='/Game/IndianRailway/S9/TrackEnvironmentV1/'
OUT=ROOT/'07_Documentation/S9/Evidence/TrackRealism-20261027-02/surfaces-v4.json'
E=unreal.MaterialEditingLibrary
A=unreal.AssetToolsHelpers.get_asset_tools()
def node(m,kind,**props):
    n=E.create_material_expression(m,kind)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def link(a,b,pin=''):assert E.connect_material_expressions(a,'',b,pin)
try:
    assert not OUT.exists()
    name='T_DryVerge_BC';source=ROOT/'04_Assets/S9/TrackEnvironmentV1/Textures'/f'{name}.png'
    assert not unreal.EditorAssetLibrary.does_asset_exist(BASE+'TexturesV2/'+name)
    task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=BASE+'TexturesV2';task.destination_name=name;task.automated=True;task.save=False
    A.import_asset_tasks([task]);texture=unreal.load_asset(BASE+'TexturesV2/'+name)
    assert isinstance(texture,unreal.Texture2D)
    texture.set_editor_property('power_of_two_mode',unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property('max_texture_size',1024);texture.set_editor_property('never_stream',False)
    texture.set_editor_property('srgb',True);texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
    texture.set_editor_property('address_x',unreal.TextureAddress.TA_WRAP);texture.set_editor_property('address_y',unreal.TextureAddress.TA_WRAP)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    rows=[]
    for name in ('M_TrackBallast','M_TrackStone','M_DryGround','M_SleeperConcrete','M_RailHead','M_RailRust','M_Fastener'):
        path=BASE+'MaterialsV4/'+name;assert not unreal.EditorAssetLibrary.does_asset_exist(path)
        m=unreal.EditorAssetLibrary.duplicate_asset(BASE+'MaterialsV3/'+name,path);assert m
        fixes=0
        if name in ('M_TrackBallast','M_SleeperConcrete'):
            for n in E.get_material_expressions(m):
                if not isinstance(n,unreal.MaterialExpressionAbs):continue
                inputs=E.get_inputs_for_material_expression(m,n)
                if len(inputs)!=1 or not isinstance(inputs[0],unreal.MaterialExpressionComponentMask):continue
                mask=inputs[0];up=E.get_inputs_for_material_expression(m,mask)
                if len(up)!=1 or not isinstance(up[0],unreal.MaterialExpressionTextureCoordinate):continue
                # FBX UV conversion is U=x, V=1-y. Decode before lateral masks.
                sub=node(m,unreal.MaterialExpressionSubtract,const_a=1.)
                link(mask,sub,'B');link(sub,n);fixes+=1
            assert fixes==1,(name,fixes)
        if name=='M_DryGround':
            base=E.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
            assert isinstance(base,unreal.MaterialExpressionLinearInterpolate)
            p=node(m,unreal.MaterialExpressionWorldPosition)
            xy=node(m,unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(p,xy)
            uv=node(m,unreal.MaterialExpressionMultiply,const_b=1/150.);link(xy,uv,'A')
            sample=node(m,unreal.MaterialExpressionTextureSample,texture=texture,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR);link(uv,sample,'UVs')
            tint=node(m,unreal.MaterialExpressionMultiply,const_b=.30);link(sample,tint,'A');link(tint,base,'B')
        E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        rows.append(dict(material=m.get_path_name(),lateralUVDecodes=fixes))
    # The bed has no runtime override; persist its final surface assignment.
    bed=unreal.load_asset(BASE+'Meshes/SM_Ballast6m_RealismV3')
    bed.set_material(0,unreal.load_asset(BASE+'MaterialsV4/M_TrackBallast'))
    assert unreal.EditorAssetLibrary.save_loaded_asset(bed)
    rail=unreal.load_asset(BASE+'Meshes/SM_Track6m_RealismV2')
    mapping={'RailHead':'M_RailHead','RailRust':'M_RailRust','Concrete':'M_SleeperConcrete'}
    for i,slot in enumerate(rail.get_editor_property('static_materials')):
        rail.set_material(i,unreal.load_asset(BASE+'MaterialsV4/'+mapping[str(slot.get_editor_property('imported_material_slot_name'))]))
    assert unreal.EditorAssetLibrary.save_loaded_asset(rail)
    OUT.write_text(json.dumps(dict(passed=True,materials=rows,texture=texture.get_path_name(),source=str(source.relative_to(ROOT)),sourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest(),generatedSourceUnmodified=True,uvContract='FBX import flips V: recover lateral metres as 1 - TextureCoordinate.G'),indent=2))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
