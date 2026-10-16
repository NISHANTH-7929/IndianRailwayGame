"""Remove only the documented accidental review additions from the S7 editor map."""
import unreal,json
from pathlib import Path
root=Path(__file__).resolve().parents[3]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/IndianRailway/S7/L_RailService'):raise RuntimeError('S7 load failed')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name()!='/Game/IndianRailway/S7/L_RailService.L_RailService':raise RuntimeError('Wrong world')
removed=[];kept=[]
for a in actors.get_all_level_actors():
    remove=False
    if isinstance(a,unreal.StaticMeshActor):
        c=a.static_mesh_component;m=c.static_mesh
        remove=bool(m and '/Stations/EgmoreHeritage01/' in m.get_path_name())
        remove=remove or any(c.get_material(i) and '/Stations/EgmoreHeritage01/' in c.get_material(i).get_path_name() for i in range(c.get_num_materials()))
    remove=remove or a.get_name() in ['DirectionalLight_7','SkyLight_3','CameraActor_3']
    row={'name':a.get_name(),'label':a.get_actor_label(),'class':a.get_class().get_name()}
    if remove:removed.append(row);actors.destroy_actor(a)
    else:kept.append(row)
if len(removed)!=28:raise RuntimeError(f'Unexpected repair count {len(removed)}; not saving')
if not levels.save_current_level():raise RuntimeError('Repair save failed')
(root/'07_Documentation/S8/Evidence/Egmore/s7-review-repair.json').write_text(json.dumps({'removed':removed,'retained':kept,'note':'Removed only review additions; semantic restoration, original serialized hash not claimed.'},indent=2))
