"""Read-only mesh section/material inventory for the route visual review."""
from pathlib import Path
import json
import unreal
root=Path(__file__).resolve().parents[3]
rows=[]
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for name in ['EgmoreCanopy6m','Platform6x8m','TreeBroadleaf','Track6m','Ballast6m']:
    mesh=unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_'+name)
    rows.append(dict(name=name,slots=[dict(slot=str(s.material_slot_name),imported=str(s.get_editor_property('imported_material_slot_name')),material=s.material_interface.get_path_name() if s.material_interface else None) for s in mesh.static_materials],
        sections=([[editor.get_lod_material_slot(mesh,i,j) for j in range(mesh.get_num_sections(i))] for i in range(editor.get_lod_count(mesh))] if editor else None)))
(root/'07_Documentation/S8/Evidence/EgmoreTambaram/material-slot-audit.json').write_text(json.dumps(rows,indent=2))
