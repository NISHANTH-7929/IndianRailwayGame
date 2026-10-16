"""Recompile imported signage using real RHI; preserve geometry and identity."""
import unreal,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
mesh=unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreSignage01/SM_Station_EgmoreNameboard4m')
if not mesh:
    data=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/stations/EgmoreSignage01.json').read_text())
    mesh=unreal.load_asset(data['parts'][0]['asset'])
rows=[]
for slot in mesh.get_editor_property('static_materials'):
    mat=slot.get_editor_property('material_interface')
    rows.append({'slot':str(slot.get_editor_property('imported_material_slot_name')),'material':mat.get_path_name() if mat else None})
    assert mat
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
(ROOT/'07_Documentation/S8/Evidence/Egmore/Signage/material-audit.json').write_text(json.dumps(rows,indent=2))
exec(compile(Path(__file__).with_name('review_egmore_sign.py').read_text(),str(Path(__file__).with_name('review_egmore_sign.py')),'exec'))
