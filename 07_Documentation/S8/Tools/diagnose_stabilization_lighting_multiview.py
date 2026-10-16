"""Six matched desktop captures, never save scene/assets; disposable editor only.

Platform/stairs coordinates: review_stabilization_station.py.
Cab-height approach coordinate: review_egmore_scene02.py. This does not spawn a
locomotive or simulate its actual driver camera and is not a cab-interior pass.
"""
import hashlib, json, time
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[3]
CONTENT=ROOT/'03_UnrealProject/MyProject/Content'
OUT=ROOT/'07_Documentation/S8/Evidence/StabilizationDesktop-20261011/LightingMultiView'
OUT.mkdir(parents=True,exist_ok=True)
assert not list(OUT.glob('*.png')) and not (OUT/'receipt.json').exists(), 'Do not overwrite earlier run'

def hashes():
    return {p.relative_to(CONTENT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in CONTENT.rglob('*.umap')}

def properties(component):
    result={}
    for name in ['intensity','visible','affects_world','source_type','cubemap','mobility','real_time_capture','cast_shadows','lower_hemisphere_is_black']:
        value=component.get_editor_property(name)
        result[name]=value if isinstance(value,(str,int,float,bool)) or value is None else str(value)
    return result

before=hashes()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/IndianRailway/S8/Egmore/L_RailService_Egmore')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_RailService_Egmore'
lights=sorted([a for a in actors.get_all_level_actors() if isinstance(a,unreal.SkyLight)],key=lambda a:a.get_actor_label())
assert len(lights)==4 and sum(a.get_actor_label()=='SkyLight3' for a in lights)==1
original=[(a.light_component,a.light_component.get_editor_property('visible'),a.light_component.get_editor_property('affects_world')) for a in lights]
views=[('platform',(9000,-460,250),(3,180,0),78),('stairs',(4900,-560,250),(12,0,0),78),('cab_height_approach',(9713,-65,335),(-4,0,0),95)]
stages=[(name,view) for view in views for name in ('saved_setup','only_SkyLight3')]
report={'status':'RUNNING','desktopOnly':True,'androidValidated':False,'assetsSaved':False,'errors':[],
        'views':views,'resolution':[1600,900],'manualExposureBias':.5,'captures':[],
        'lightsBefore':[{'label':a.get_actor_label(),'properties':properties(a.light_component)} for a in lights],
        'limitations':['Cab-height approach is a station-view proxy, not furnished runtime cab validation.',
                       'Both matched setups are recaptured. Original cache-only baseline was assessed separately in Lighting.',
                       'This diagnostic cannot approve packaged Android or select a saved correction automatically.']}
(OUT/'initial-state.json').write_text(json.dumps(report,indent=2))
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector())
c=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
for k,v in [('capture_every_frame',False),('capture_on_movement',False),('always_persist_rendering_state',True),('post_process_blend_weight',1.0)]:c.set_editor_property(k,v)
target=unreal.RenderingLibrary.create_render_target2d(world,1600,900,unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
c.set_editor_property('texture_target',target)
c.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
settings=c.get_editor_property('post_process_settings')
for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',.5),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False)]:settings.set_editor_property(k,v)
c.set_editor_property('post_process_settings',settings)

def configure(stage):
    state,(_,position,rotation,fov)=stage
    for actor,(component,visible,affects) in zip(lights,original):
        selected=actor.get_actor_label()=='SkyLight3'
        component.set_visibility(visible if state=='saved_setup' else selected)
        component.set_editor_property('affects_world',affects if state=='saved_setup' else selected)
    for component,_,_ in original:
        if component.get_editor_property('visible') and component.get_editor_property('affects_world'):component.recapture_sky()
    capture.set_actor_location(unreal.Vector(*position),False,False)
    capture.set_actor_rotation(unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]),False)
    c.set_editor_property('fov_angle',fov)

index=0;pending=None;started=time.monotonic();ready=started+40
configure(stages[0])

def finish(error=None):
    if error:report['errors'].append(str(error))
    for component,visible,affects in original:
        try:
            component.set_visibility(visible);component.set_editor_property('affects_world',affects)
        except Exception as e:report['errors'].append('restore: '+str(e))
    actors.destroy_actor(capture)
    after=hashes()
    report.update(mapsUnchanged=before==after,mapHashesBefore=before,mapHashesAfter=after,
                  elapsedSeconds=time.monotonic()-started,status='PASS' if not report['errors'] and before==after else 'FAIL')
    (OUT/'receipt.json').write_text(json.dumps(report,indent=2))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()

def tick(dt):
    global index,pending,ready
    try:
        now=time.monotonic()
        if now-started>300:raise RuntimeError('300-second diagnostic bound exceeded')
        if now<ready:return
        if pending is None:c.capture_scene();pending=now;return
        if now-pending<3:return
        state,view=stages[index];name=view[0]+'__'+state
        unreal.RenderingLibrary.export_render_target(world,target,str(OUT),name+'.png')
        path=OUT/(name+'.png')
        assert path.is_file() and path.stat().st_size>1024,'Missing capture '+name
        report['captures'].append({'name':name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                                   'view':view,'recaptured':True,
                                   'lights':[{'label':a.get_actor_label(),'properties':properties(a.light_component)} for a in lights]})
        index+=1;pending=None
        if index==len(stages):finish();return
        configure(stages[index]);ready=now+14
    except Exception as e:finish(e)

handle=unreal.register_slate_post_tick_callback(tick)
