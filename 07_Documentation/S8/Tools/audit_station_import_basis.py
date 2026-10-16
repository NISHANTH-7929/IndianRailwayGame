"""Read imported bounds to check asymmetric module axis conversion."""
import unreal,json
from pathlib import Path
root=Path(__file__).resolve().parents[3];rows=[]
for family,folder in [('Kit01','StationKit'),('EgmoreHeritage01','EgmoreKit')]:
    a=json.loads((root/'04_Assets/S8'/folder/'assembly.json').read_text())
    for p in a['parts']:
        mesh=unreal.load_asset('/Game/IndianRailway/S8/Stations/'+family+'/Meshes/SM_Station_'+p['name'])
        b=mesh.get_bounding_box()
        rows.append({'family':family,'name':p['name'],'source':p['bounds_m'],'imported':[[b.min.x/100,b.min.y/100,b.min.z/100],[b.max.x/100,b.max.y/100,b.max.z/100]]})
(root/'07_Documentation/S8/Evidence/Egmore/import-basis-audit.json').write_text(json.dumps(rows,indent=2))
