"""New cab finish assets. Preserve contact UVs, original assets and lettering."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
DEST = '/Game/IndianRailway/S9/CabRealismV1/Materials'
SOURCE = '/Game/IndianRailway/S8/Locomotives/WAP7/Cab'
OUT = ROOT/'07_Documentation/S9/Evidence/CabRealism-20261023-01'
E = unreal.MaterialEditingLibrary
rows = []
settings = {
    'DeskEnamel': ((.43,.45,.40), .38, 0., .035),
    'CabIvory': ((.52,.52,.46), .55, 0., .025),
    'PanelCharcoal': ((.030,.038,.034), .56, 0., .025),
    'Rubber': ((.016,.018,.017), .79, 0., .07),
    'Floor': ((.055,.060,.055), .91, 0., .11),
    'SeatBlue': ((.055,.12,.16), .88, 0., .12),
    'Steel': ((.32,.34,.33), .30, .85, 0.),
    'Brass': ((.43,.28,.09), .35, .78, 0.),
    'InstrumentBezel': ((.28,.30,.28), .24, .85, 0.),
}

def node(m, kind, **props):
    n = E.create_material_expression(m, kind)
    for k, v in props.items(): n.set_editor_property(k, v)
    return n

def link(a, b, slot, output=''):
    assert E.connect_material_expressions(a, output, b, slot)

def prop(n, m, p):
    assert E.connect_material_property(n, '', p)

def constant(m, value): return node(m, unreal.MaterialExpressionConstant, r=value)

def mul(m, a, factor):
    n = node(m, unreal.MaterialExpressionMultiply, const_b=factor)
    link(a,n,'A'); return n

def add(m, a, value):
    n = node(m, unreal.MaterialExpressionAdd, const_b=value)
    link(a,n,'A'); return n

def sample(m, texture, uv):
    n = node(m, unreal.MaterialExpressionTextureSample, texture=texture,
             sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    link(uv,n,'UVs'); return n

definitions = json.loads((ROOT/'04_Assets/S8/Cabs/WAP7/assembly.json').read_text())['materials']
assert not (OUT/'materials.json').exists(), 'Preserve existing evidence'
for d in definitions:
    name=d['name']; base=name.removesuffix('_StaticAO')
    if base not in settings and name not in ('WindowGlass','InstrumentGlass'): continue
    path=DEST+'/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(path), path
    m=unreal.EditorAssetLibrary.duplicate_asset(SOURCE+'/Materials/'+name,path)
    assert m
    E.delete_all_material_expressions(m)
    if 'Glass' in name:
        m.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE)
        # Surface lighting supplies a grazing highlight. Central sightlines
        # stay almost clear, with no refraction, scratches or false rain film.
        color=node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.17,.22,.20,1))
        prop(color,m,unreal.MaterialProperty.MP_BASE_COLOR)
        fresnel=node(m,unreal.MaterialExpressionFresnel,exponent=4.,base_reflect_fraction=.04)
        prop(add(m,mul(m,fresnel,.12 if name=='WindowGlass' else .07),.018),m,unreal.MaterialProperty.MP_OPACITY)
        prop(constant(m,.065 if name=='WindowGlass' else .09),m,unreal.MaterialProperty.MP_ROUGHNESS)
        prop(constant(m,.5),m,unreal.MaterialProperty.MP_SPECULAR)
        normal_samples=0
    else:
        rgb,rough,metal,strength=settings[base]
        color=node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*rgb,1))
        surface=color; normal_samples=0
        uv_index=int(d.get('detail_uv',0))
        if d.get('detail_texture'):
            tex=unreal.load_asset(SOURCE+'/Textures/'+Path(d['detail_texture']).stem)
            assert tex
            uv=node(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=uv_index)
            uvfine=mul(m,uv,18. if base=='SeatBlue' else 12.)
            fine=sample(m,tex,uvfine)
            # Texture RGB is grayscale; explicit R masks keep scalar math scalar.
            height=node(m,unreal.MaterialExpressionComponentMask,r=True,g=False,b=False,a=False)
            link(fine,height,'')
            variation=add(m,mul(m,height,.06),.94)
            surface=node(m,unreal.MaterialExpressionMultiply);link(color,surface,'A');link(variation,surface,'B')
            prop(mul(m,add(m,mul(m,height,.08),.92),rough),m,unreal.MaterialProperty.MP_ROUGHNESS)
            # Two decorrelated samples produce subtle tangent-space micrograin.
            # Reuse the existing texture instead of per-pixel procedural noise.
            offset=node(m,unreal.MaterialExpressionConstant2Vector,r=.37,g=.61)
            shifted=node(m,unreal.MaterialExpressionAdd);link(uvfine,shifted,'A');link(offset,shifted,'B')
            fine2=sample(m,tex,shifted)
            height2=node(m,unreal.MaterialExpressionComponentMask,r=True,g=False,b=False,a=False);link(fine2,height2,'')
            xy=node(m,unreal.MaterialExpressionAppendVector)
            link(mul(m,add(m,height,-.5),strength),xy,'A');link(mul(m,add(m,height2,-.5),strength),xy,'B')
            xyz=node(m,unreal.MaterialExpressionAppendVector);link(xy,xyz,'A');link(constant(m,1.),xyz,'B')
            norm=node(m,unreal.MaterialExpressionNormalize);link(xyz,norm,'');prop(norm,m,unreal.MaterialProperty.MP_NORMAL)
            normal_samples=2
        else: prop(constant(m,rough),m,unreal.MaterialProperty.MP_ROUGHNESS)
        if d.get('ao_texture'):
            ao=unreal.load_asset(SOURCE+'/Textures/CabContact');assert ao
            uv=node(m,unreal.MaterialExpressionTextureCoordinate,coordinate_index=int(d['ao_uv']))
            s=sample(m,ao,uv)
            h=node(m,unreal.MaterialExpressionComponentMask,r=True,g=False,b=False,a=False);link(s,h,'')
            contact=add(m,mul(m,h,.45),.55)
            shaded=node(m,unreal.MaterialExpressionMultiply);link(surface,shaded,'A');link(contact,shaded,'B');surface=shaded
        prop(surface,m,unreal.MaterialProperty.MP_BASE_COLOR)
        prop(constant(m,metal),m,unreal.MaterialProperty.MP_METALLIC)
        prop(constant(m,.5),m,unreal.MaterialProperty.MP_SPECULAR)
    E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    E.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    rows.append(dict(name=name,asset=path,detailUv=d.get('detail_uv',0),contactUv=d.get('ao_uv'),normalTextureSamples=normal_samples))
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'materials.json').write_text(json.dumps(dict(materials=rows,originalAssetsUnchanged=True,controlChannelsUnchanged=True),indent=2))
unreal.log('CAB_REALISM_MATERIALS_READY')
