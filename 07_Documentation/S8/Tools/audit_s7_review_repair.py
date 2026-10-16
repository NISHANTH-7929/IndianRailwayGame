"""Compare retained S7 base actors to the pre-existing clean LHB map."""
import unreal,json
from pathlib import Path
root=Path(__file__).resolve().parents[3];levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def inspect(path):
    if not levels.load_level(path):raise RuntimeError('Load failed: '+path)
    result={}
    for a in actors.get_all_level_actors():
        if isinstance(a,(unreal.WorldSettings,unreal.LevelScriptActor)):continue
        if isinstance(a,unreal.StaticMeshActor):
            mesh=a.static_mesh_component.static_mesh
            if mesh and mesh.get_path_name().startswith('/Game/IndianRailway/S8/'):continue
        l=a.get_actor_location();r=a.get_actor_rotation();s=a.get_actor_scale3d()
        result[a.get_name()]={'class':a.get_class().get_name(),'location':[l.x,l.y,l.z],'rotation':[r.pitch,r.yaw,r.roll],'scale':[s.x,s.y,s.z],'label':a.get_actor_label()}
    return result
s7=inspect('/Game/IndianRailway/S7/L_RailService');lhb=inspect('/Game/IndianRailway/S8/LHB/L_RailService_LHB')
report={'passed':s7==lhb,'s7BaseActors':s7,'lhbBaseActors':lhb,'note':'LHB map predates this review and was created by removing only the 28 static S8 exhibits from S7.'}
(root/'07_Documentation/S8/Evidence/Egmore/s7-base-actor-audit.json').write_text(json.dumps(report,indent=2))
if not report['passed']:raise RuntimeError('S7 base actor mismatch')
unreal.log('S7_REPAIR_BASE_ACTORS_MATCH')
