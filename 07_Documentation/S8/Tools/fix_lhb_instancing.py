import unreal,json
from pathlib import Path
root=Path('D:/Project/IndianRailwayGame')
for path in unreal.EditorAssetLibrary.list_assets('/Game/IndianRailway/S8/Coaches/LHB3A/Materials'):
 m=unreal.load_asset(path)
 if isinstance(m,unreal.Material):
  unreal.MaterialEditingLibrary.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
  unreal.MaterialEditingLibrary.recompile_material(m)
  unreal.EditorAssetLibrary.save_loaded_asset(m)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={}
for level in ['/Game/IndianRailway/S7/L_RailService','/Game/IndianRailway/S6/L_ChunkA','/Game/IndianRailway/S6/L_ChunkB']:
 levels.load_level(level);rows=[]
 for actor in actors.get_all_level_actors():
  row={'name':actor.get_name(),'class':actor.get_class().get_name()}
  meshes=actor.get_components_by_class(unreal.StaticMeshComponent)
  row['meshes']=[m.static_mesh.get_path_name() for m in meshes if m.static_mesh]
  rows.append(row)
 report[level]=rows
(root/'07_Documentation/S8/Evidence/LHB/map-actor-audit.json').write_text(json.dumps(report,indent=2))
unreal.SystemLibrary.quit_editor()
