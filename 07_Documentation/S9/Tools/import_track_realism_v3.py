"""Create the V3 realism finishes and wider, embedded ballast bed.
Reuses the original generated images; no source PNG or prior asset is changed.
Run in a full editor. Companion rail importer runs in this same editor session.
"""
from pathlib import Path
import hashlib
import json
import runpy
import time
import unreal
ROOT=Path(__file__).resolve().parents[3]
DEST='/Game/IndianRailway/S9/TrackEnvironmentV1'
OUT=ROOT/'07_Documentation/S9/Evidence/TrackRealism-20261027-02'
E=unreal.MaterialEditingLibrary
A=unreal.AssetToolsHelpers.get_asset_tools()
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S, 'Full editor required'
OUT.mkdir(parents=True,exist_ok=True)
assert not (OUT/'surfaces-v3.json').exists(), 'Preserve prior receipt'
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

def absnode(m,x):
    n=node(m,unreal.MaterialExpressionAbs);link(x,n);return n

def macro(m,scale):
    return node(m,unreal.MaterialExpressionNoise,scale=scale,quality=1,levels=2,output_min=0.,output_max=1.)

try:
    textures={name:unreal.load_asset(DEST+'/TexturesV2/'+name) for name in ('T_TrackBallast_BC','T_DryGround_BC')}
    assert all(isinstance(t,unreal.Texture2D) for t in textures.values())
    names=('M_TrackBallast','M_TrackStone','M_DryGround','M_SleeperConcrete','M_RailHead','M_RailRust','M_Fastener')
    for name in names:assert not unreal.EditorAssetLibrary.does_asset_exist(DEST+'/MaterialsV3/'+name),name
    rows=[];materials={}
    for name in names:
        m=A.create_asset(name,DEST+'/MaterialsV3',unreal.Material,unreal.MaterialFactoryNew());assert m
        ground=name=='M_DryGround';bed=name=='M_TrackBallast';stone=name=='M_TrackStone'
        tex=textures['T_TrackBallast_BC' if bed or stone else 'T_DryGround_BC']
        uv=world_uv(m,1.15 if bed or stone else 1.9 if ground else .42)
        s=sample(m,tex,uv);h=mask(m,s,'R')
        if bed or stone or ground:
            surface=mul(m,s,.48 if bed or stone else .30)
            n=macro(m,.0012 if ground else .004)
            surface=mul(m,surface,add(m,mul(m,n,.42),.72))
            if ground:
                color=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.20,.16,.095,1))
                surface=mul(m,surface,mul(m,color,rgb(m,(5.,6.25,1/.095))))
                coverage=node(m,unreal.MaterialExpressionScalarParameter,parameter_name='VegetationCoverage',default_value=0.)
                # Threshold, not a blanket tint. A zero parameter produces soil;
                # broad ground opts into world-continuous olive/dry-grass patches.
                cover=clamp(m,mul(m,sub(m,n,sub(m,scalar(m,1),coverage)),4.0))
                grass=mul(m,rgb(m,(.045,.064,.023)),add(m,mul(m,h,.85),.65))
                surface=lerp(m,surface,grass,cover)
            elif bed:
                uvlocal=node(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0)
                lateral=absnode(m,mask(m,uvlocal,'G'))
                # The worn outer apron becomes the same dirt as the terrain.
                edge=clamp(m,add(m,mul(m,sub(m,lateral,1.84),2.3),mul(m,sub(m,n,.5),.15)))
                soil=mul(m,sample(m,textures['T_DryGround_BC'],world_uv(m,1.9)),.30)
                surface=lerp(m,surface,soil,edge)
                seat=clamp(m,sub(m,scalar(m,1),mul(m,absnode(m,sub(m,lateral,.873)),3.5)))
                surface=mul(m,surface,sub(m,scalar(m,1),mul(m,seat,.13)))
            if not stone:grain_normal(m,tex,uv,s,1.2 if ground else 4.0)
            prop(surface,m,unreal.MaterialProperty.MP_BASE_COLOR)
            prop(add(m,mul(m,h,.08),.88),m,unreal.MaterialProperty.MP_ROUGHNESS)
            prop(scalar(m,0),m,unreal.MaterialProperty.MP_METALLIC)
            prop(scalar(m,.20),m,unreal.MaterialProperty.MP_SPECULAR)
        else:
            colors={'M_RailHead':(.29,.31,.32),'M_RailRust':(.095,.041,.020),'M_SleeperConcrete':(.11,.105,.089),'M_Fastener':(.038,.032,.025)}
            rough={'M_RailHead':.27,'M_RailRust':.89,'M_SleeperConcrete':.93,'M_Fastener':.78}
            metal={'M_RailHead':.88,'M_RailRust':.10,'M_SleeperConcrete':0.,'M_Fastener':.45}
            surface=mul(m,rgb(m,colors[name]),add(m,mul(m,h,.65),.78))
            if name=='M_SleeperConcrete':
                n=macro(m,.008)
                surface=mul(m,surface,add(m,mul(m,n,.22),.83))
                local=node(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0)
                seat=clamp(m,sub(m,scalar(m,1),mul(m,absnode(m,sub(m,absnode(m,mask(m,local,'G')),.873)),4.5)))
                surface=mul(m,surface,sub(m,scalar(m,1),mul(m,seat,.22)))
            prop(surface,m,unreal.MaterialProperty.MP_BASE_COLOR)
            prop(mul(m,add(m,mul(m,h,.15),.85),rough[name]),m,unreal.MaterialProperty.MP_ROUGHNESS)
            prop(scalar(m,metal[name]),m,unreal.MaterialProperty.MP_METALLIC)
            prop(scalar(m,.45),m,unreal.MaterialProperty.MP_SPECULAR)
        E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
        E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        materials[name]=m
        expressions=E.get_material_expressions(m)
        rows.append(dict(asset=m.get_path_name(),textureSamples=sum(isinstance(x,unreal.MaterialExpressionTextureSample) for x in expressions),
            vegetationParameter=any(isinstance(x,unreal.MaterialExpressionScalarParameter) and str(x.get_editor_property('parameter_name'))=='VegetationCoverage' for x in expressions)))
    assert next(r for r in rows if r['asset'].endswith('M_DryGround.M_DryGround'))['vegetationParameter']
    source=ROOT/'04_Assets/S9/TrackEnvironmentV1/MeshesV3Final/mesh-receipt.json'
    receipt=json.loads(source.read_text());records=receipt['records']
    name='SM_Ballast6m_RealismV3';path=DEST+'/Meshes/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(path)
    for r in records:assert hashlib.sha256((ROOT/r['fbx']).read_bytes()).hexdigest()==r['sha256']
    task=unreal.AssetImportTask();task.filename=str(ROOT/records[0]['fbx']);task.destination_path=DEST+'/Meshes';task.destination_name=name;task.automated=True;task.save=False
    options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False
    options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
    d=options.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=False;d.import_mesh_lo_ds=False
    d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.import_uniform_scale=1.;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options=options;A.import_asset_tasks([task]);mesh=unreal.load_asset(path);assert isinstance(mesh,unreal.StaticMesh)
    assert len(mesh.get_editor_property('static_materials'))==1
    mesh.set_material(0,materials['M_TrackBallast']);assert S.set_lod_screen_sizes(mesh,[1.])
    for level in (1,2):assert S.import_lod(mesh,level,str(ROOT/records[level]['fbx']))==level
    S.remove_collisions(mesh);assert S.set_lod_screen_sizes(mesh,[1.,.2,.06])
    counts=[mesh.get_num_triangles(i) for i in range(3)];assert counts==[r['triangles'] for r in records]
    assert all(mesh.get_num_sections(i)==1 for i in range(3)) and S.get_simple_collision_count(mesh)==0
    bounds=mesh.get_bounding_box();assert abs(bounds.max.z+23.5)<.01
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    (OUT/'surfaces-v3.json').write_text(json.dumps(dict(passed=True,materials=rows,bed=path,bedTriangles=counts,
        bedTopMetres=-.235,flatHalfWidthMetres=1.73,outerHalfWidthMetres=2.32,sourceReceipt=str(source.relative_to(ROOT)),
        reusedTextures=[t.get_path_name() for t in textures.values()],sourceImagesChanged=False),indent=2))
    # The companion importer validates/saves the rail mesh and schedules exit.
    runpy.run_path(str(Path(__file__).with_name('import_track_rails_v2.py')),run_name='__main__')
except Exception:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
    raise

