"""Create a separate service map without historical static review exhibits."""
import unreal,json
from pathlib import Path
src='/Game/IndianRailway/S7/L_RailService'
dst='/Game/IndianRailway/S8/LHB/L_RailService_LHB'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(src):raise RuntimeError('Map load failed')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);removed=[]
for actor in actors.get_all_level_actors():
 if isinstance(actor,unreal.StaticMeshActor):
  mesh=actor.static_mesh_component.static_mesh
  if mesh and mesh.get_path_name().startswith('/Game/IndianRailway/S8/'):
   removed.append({'actor':actor.get_name(),'mesh':mesh.get_path_name()})
   actors.destroy_actor(actor)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.EditorLoadingAndSavingUtils.save_map(world,dst):raise RuntimeError('Map save failed')
Path('D:/Project/IndianRailwayGame/07_Documentation/S8/Evidence/LHB/service-map-cleanup.json').write_text(json.dumps({'sourceUnchanged':src,'destination':dst,'removedReviewExhibits':removed},indent=2))
unreal.SystemLibrary.quit_editor()
