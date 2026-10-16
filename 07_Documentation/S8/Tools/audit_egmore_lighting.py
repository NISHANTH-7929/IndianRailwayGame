"""Read-only audit of the saved Egmore review map; never save this world."""
import unreal,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path='/Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study'
if not levels.load_level(path):raise RuntimeError('Review map unavailable')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def props(obj,names):
    result={}
    for name in names:
        try:
            value=obj.get_editor_property(name)
            result[name]=value if isinstance(value,(int,float,str,bool)) else str(value)
        except Exception as error:result[name]={'unavailable':str(error)}
    return result
report={'lights':[],'cameras':[],'meshes':[]}
for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    if isinstance(actor,unreal.DirectionalLight):
        report['lights'].append({'label':label,**props(actor.light_component,['intensity','mobility','cast_shadows','dynamic_shadow_distance_movable_light','dynamic_shadow_cascades','cascade_distribution_exponent','shadow_bias','shadow_slope_bias'])})
    if isinstance(actor,unreal.CameraActor):
        camera=actor.camera_component
        pp=camera.get_editor_property('post_process_settings')
        report['cameras'].append({'label':label,**props(camera,['field_of_view','post_process_blend_weight']), 'exposure':props(pp,['override_auto_exposure_method','auto_exposure_method','override_auto_exposure_bias','auto_exposure_bias','auto_exposure_apply_physical_camera_exposure','override_auto_exposure_apply_physical_camera_exposure'])})
    if isinstance(actor,unreal.StaticMeshActor) and ('Canopy' in label or 'platform' in label.lower()):
        report['meshes'].append({'label':label,**props(actor.static_mesh_component,['mobility','cast_shadow','cast_dynamic_shadow','receive_mobile_csm_shadows'])})
out=ROOT/'07_Documentation/S8/Evidence/Egmore/lighting-audit-v19.json'
out.write_text(json.dumps(report,indent=2))
unreal.log('EGMORE_LIGHTING_AUDIT_COMPLETE')
