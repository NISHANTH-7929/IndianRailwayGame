"""Create original S9 material derivatives using the two generated albedos.

No source assets are edited. Fine bump is a restrained luminance approximation,
not a claimed measured height/normal map. All graph connections are checked.
"""
import hashlib
import json
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/'04_Assets/S9/TrackEnvironmentV1/Textures'
DEST='/Game/IndianRailway/S9/TrackEnvironmentV1'
OUT=ROOT/'07_Documentation/S9/Evidence/TrackEnvironment-20261025-01'
E=unreal.MaterialEditingLibrary
A=unreal.AssetToolsHelpers.get_asset_tools()
assert not (OUT/'materials.json').exists(), 'Preserve previous evidence'

def node(m,kind,**props):
    n=E.create_material_expression(m,kind)
    for k,v in props.items(): n.set_editor_property(k,v)
    return n
def link(a,b,slot='',output=''): assert E.connect_material_expressions(a,output,b,slot)
def prop(n,m,p): assert E.connect_material_property(n,'',p)
def scalar(m,v): return node(m,unreal.MaterialExpressionConstant,r=float(v))
def rgb(m,c): return node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*c,1))
def binary(m,kind,a,b):
    n=node(m,kind);link(a,n,'A');link(b,n,'B');return n
def mul(m,a,b): return binary(m,unreal.MaterialExpressionMultiply,a,scalar(m,b) if isinstance(b,(float,int)) else b)
def add(m,a,b): return binary(m,unreal.MaterialExpressionAdd,a,scalar(m,b) if isinstance(b,(float,int)) else b)
def sub(m,a,b): return binary(m,unreal.MaterialExpressionSubtract,a,scalar(m,b) if isinstance(b,(float,int)) else b)
def mask(m,n,channel):
    out=node(m,unreal.MaterialExpressionComponentMask,r=channel=='R',g=channel=='G',b=channel=='B',a=False);link(n,out);return out
def sample(m,tex,uv):
    n=node(m,unreal.MaterialExpressionTextureSample,texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    link(uv,n,'UVs');return n
def world_uv(m,metres):
    p=node(m,unreal.MaterialExpressionWorldPosition)
    xy=node(m,unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(p,xy)
    return mul(m,xy,1./(metres*100))
def lerp(m,a,b,t):
    n=node(m,unreal.MaterialExpressionLinearInterpolate);link(a,n,'A');link(b,n,'B');link(t,n,'Alpha');return n
def clamp(m,x):
    n=node(m,unreal.MaterialExpressionClamp,min_default=0.,max_default=1.);link(x,n);return n
def grain_normal(m,tex,uv,center,strength):
    # Small UV derivatives add grain without per-pixel noise or displacement.
    u=node(m,unreal.MaterialExpressionConstant2Vector,r=.002,g=0.)
    v=node(m,unreal.MaterialExpressionConstant2Vector,r=0.,g=.002)
    dx=mask(m,sample(m,tex,add(m,uv,u)),'R')
    dy=mask(m,sample(m,tex,add(m,uv,v)),'R')
    h=mask(m,center,'R')
    xy=node(m,unreal.MaterialExpressionAppendVector);link(mul(m,sub(m,h,dx),strength),xy,'A');link(mul(m,sub(m,h,dy),strength),xy,'B')
    # The texture is world-XY projected: rotate its gradient into each instance's
    # tangent frame before constructing the tangent-space normal. Dropping tangent
    # Z projects the gradient onto the surface, including the ballast slopes.
    gradient=node(m,unreal.MaterialExpressionAppendVector);link(xy,gradient,'A');link(scalar(m,0),gradient,'B')
    transform=node(m,unreal.MaterialExpressionTransform,
        transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
        transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    link(gradient,transform)
    projected=node(m,unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(transform,projected)
    xyz=node(m,unreal.MaterialExpressionAppendVector);link(projected,xyz,'A');link(scalar(m,1),xyz,'B')
    norm=node(m,unreal.MaterialExpressionNormalize);link(xyz,norm);prop(norm,m,unreal.MaterialProperty.MP_NORMAL)

textures={}; texture_rows=[]
for stem in ('T_TrackBallast_BC','T_DryGround_BC'):
    path=DEST+'/Textures/'+stem
    assert not unreal.EditorAssetLibrary.does_asset_exist(path), path
    task=unreal.AssetImportTask();task.filename=str(SOURCE/(stem+'.png'));task.destination_path=DEST+'/Textures';task.destination_name=stem
    task.automated=True;task.replace_existing=False;task.save=False
    A.import_asset_tasks([task]);t=unreal.load_asset(path);assert isinstance(t,unreal.Texture2D)
    t.set_editor_property('srgb',True)
    t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
    # Generated source images are NPOT. Stretch rather than pad their seamless
    # tiles, and replace the importer's NoMipmaps default for distant surfaces.
    t.set_editor_property('power_of_two_mode',unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    t.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    t.set_editor_property('max_texture_size',1024)
    t.set_editor_property('never_stream',False)
    t.set_editor_property('address_x',unreal.TextureAddress.TA_WRAP);t.set_editor_property('address_y',unreal.TextureAddress.TA_WRAP)
    assert unreal.EditorAssetLibrary.save_loaded_asset(t);textures[stem]=t
    texture_rows.append(dict(asset=path,sourceSha256=hashlib.sha256((SOURCE/(stem+'.png')).read_bytes()).hexdigest(),generated=True,maxTextureSize=1024,powerOfTwoMode='StretchToPowerOfTwo',mipGenSettings='SimpleAverage',neverStream=False))

rows=[]
for name in ('M_TrackBallast','M_TrackStone','M_DryGround','M_RailHead','M_RailRust','M_SleeperConcrete','M_Fastener'):
    path=DEST+'/Materials/'+name;assert not unreal.EditorAssetLibrary.does_asset_exist(path),path
    m=A.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew());assert m
    if name in ('M_TrackBallast','M_TrackStone','M_DryGround'):
        ground=name=='M_DryGround';stone=name=='M_TrackStone'
        tex=textures['T_DryGround_BC' if ground else 'T_TrackBallast_BC']
        uv=world_uv(m,2. if ground else 1.25)
        s=sample(m,tex,uv);surface=mul(m,s,.78 if ground else .90)
        if ground:
            color=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.20,.16,.095,1))
            surface=mul(m,surface,mul(m,color,rgb(m,(5.,6.25,1/.095))))
        if not stone:
            macro=node(m,unreal.MaterialExpressionNoise,scale=.00033 if ground else .006,quality=1,levels=1,output_min=.86,output_max=1.)
            surface=mul(m,surface,macro)
            grain_normal(m,tex,uv,s,1.1 if ground else 2.8)
        if name=='M_TrackBallast':
            # Metric mesh UV0 retains lateral metres, including curved instances.
            uvlocal=node(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0)
            y=mask(m,uvlocal,'G');absolute=node(m,unreal.MaterialExpressionAbs);link(y,absolute)
            edge=mul(m,clamp(m,mul(m,sub(m,absolute,1.65),2.7)),.7)
            soil=mul(m,sample(m,textures['T_DryGround_BC'],world_uv(m,2.)),.78)
            surface=lerp(m,surface,soil,edge)
        prop(surface,m,unreal.MaterialProperty.MP_BASE_COLOR)
        prop(add(m,mul(m,mask(m,s,'R'),.10),.84),m,unreal.MaterialProperty.MP_ROUGHNESS)
        prop(scalar(m,0),m,unreal.MaterialProperty.MP_METALLIC)
        prop(scalar(m,.25),m,unreal.MaterialProperty.MP_SPECULAR)
        samples=1 if stone else 4 if name=='M_TrackBallast' else 3
    else:
        colors={'M_RailHead':(.34,.36,.37),'M_RailRust':(.135,.063,.029),'M_SleeperConcrete':(.28,.27,.235),'M_Fastener':(.055,.047,.037)}
        rough={'M_RailHead':.26,'M_RailRust':.88,'M_SleeperConcrete':.88,'M_Fastener':.65}
        metal={'M_RailHead':.85,'M_RailRust':.15,'M_SleeperConcrete':0.,'M_Fastener':.55}
        s=sample(m,textures['T_DryGround_BC'],world_uv(m,.6 if name=='M_SleeperConcrete' else .3))
        h=mask(m,s,'R');variation=add(m,mul(m,h,.65),.70)
        prop(mul(m,rgb(m,colors[name]),variation),m,unreal.MaterialProperty.MP_BASE_COLOR)
        prop(mul(m,add(m,mul(m,h,.15),.85),rough[name]),m,unreal.MaterialProperty.MP_ROUGHNESS)
        prop(scalar(m,metal[name]),m,unreal.MaterialProperty.MP_METALLIC)
        prop(scalar(m,.5),m,unreal.MaterialProperty.MP_SPECULAR)
        samples=1
    E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    rows.append(dict(asset=path,textureSamples=samples,worldScaleIndependent=True))
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'materials.json').write_text(json.dumps(dict(materials=rows,textures=texture_rows,originalAssetsUnchanged=True,displacement=False,luminanceBumpApproximation=True),indent=2))
unreal.log('TRACK_ENVIRONMENT_MATERIALS_READY')
