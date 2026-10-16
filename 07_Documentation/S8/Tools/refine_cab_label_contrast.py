"""Add restrained label backlighting; retain every vertex, UV and control pivot."""
import bpy,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'04_Assets/S8/Cabs/WAP7'
EVIDENCE=ROOT/'07_Documentation/S8/Evidence/Cab/LabelReadability-20261010'
BASE=EVIDENCE/'Before'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'WAP7_Cab.blend'))
d=json.loads((BASE/'assembly.json').read_text())
def geometry():
    return {o.name:hashlib.sha256(str(([tuple(v.co) for v in o.data.vertices],[[tuple(v.uv) for v in u.data] for u in o.data.uv_layers],tuple(o.matrix_world))).encode()).hexdigest() for o in bpy.data.objects if o.type=='MESH'}
before=geometry()
for name in ['Lettering','Lettering_StaticAO']:
    row=next(m for m in d['materials'] if m['name']==name)
    row.update(instrument_emission=[.72,.76,.69],instrument_emission_strength=.18)
    mat=bpy.data.materials.get(name)
    if mat:
        bsdf=mat.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Emission Color'].default_value=(.72,.76,.69,1)
        bsdf.inputs['Emission Strength'].default_value=.18
d['presentation_revision']='cab-label-contrast-20261010'
d['limitations'].append('Fine printed labels use restrained neutral presentation backlighting for mobile contrast; this is not a verified prototype lighting circuit. Physical type size is retained and fine print may require a closer view.')
assert before==geometry()
(OUT/'assembly.json').write_text(json.dumps(d,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_Cab.blend'))
(EVIDENCE/'source-refinement.json').write_text(json.dumps(dict(geometry_uv_pivots_unchanged=True,geometry_hashes=before,triangles=d['triangles'],parts=len(d['parts']),materials=['Lettering','Lettering_StaticAO'],emission_strength=.18,revision=d['presentation_revision']),indent=2))
print('CAB_LABEL_CONTRAST_SOURCE_READY')
