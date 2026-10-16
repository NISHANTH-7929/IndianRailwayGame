"""Separate Unreal art-review map. No railway service identity or save writes."""
import unreal,json,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path='/Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study'
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    if not levels.load_level(map_path):raise RuntimeError('Review map load failed')
else:
    if not levels.new_level(map_path):raise RuntimeError('Review map creation failed')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name()!=map_path+'.L_EgmoreHeritage_Study':raise RuntimeError('Wrong world; refusing edits')
for actor in actors.get_all_level_actors():
    if isinstance(actor,(unreal.StaticMeshActor,unreal.DirectionalLight,unreal.SkyLight,unreal.CameraActor,unreal.SceneCapture2D)):actors.destroy_actor(actor)
data=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/stations/EgmoreHeritage01.json').read_text())
meshes={p['name']:unreal.load_asset(p['asset']) for p in data['parts']}
signage=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/stations/EgmoreSignage01.json').read_text())
meshes.update({p['name']:unreal.load_asset(p['asset']) for p in signage['parts']})
kit=json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/stations/Kit01.json').read_text())
kit_meshes={p['name']:unreal.load_asset(p['asset']) for p in kit['parts']}

def yard_material():
    # Ground-only XY projection preserves two-metre texture scale regardless
    # of the review slab size. Do not apply this to vertical architecture.
    folder='/Game/IndianRailway/S8/Stations/EgmoreHeritage01/Materials'
    mat=unreal.load_asset(folder+'/M_EgmoreYardGround')
    if not mat:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_EgmoreYardGround',folder,unreal.Material,unreal.MaterialFactoryNew())
    edit=unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(mat)
    def node(cls,**props):
        n=edit.create_material_expression(mat,cls)
        for key,value in props.items():n.set_editor_property(key,value)
        return n
    position=node(unreal.MaterialExpressionWorldPosition)
    xy=node(unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False)
    assert edit.connect_material_expressions(position,'',xy,str(edit.get_material_expression_input_names(xy)[0]))
    uv=node(unreal.MaterialExpressionDivide,const_b=200.0)
    assert edit.connect_material_expressions(xy,'',uv,str(edit.get_material_expression_input_names(uv)[0]))
    for suffix in ['BC','N','ORM']:
        texture=unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreHeritage01/Textures/T_Soil_'+suffix)
        if not texture:raise RuntimeError('Missing soil texture '+suffix)
        sample=node(unreal.MaterialExpressionTextureSample,texture=texture)
        sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix=='N' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS if suffix=='ORM' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        assert edit.connect_material_expressions(uv,'',sample,str(edit.get_material_expression_input_names(sample)[0]))
        if suffix=='BC':assert edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        elif suffix=='N':assert edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_NORMAL)
        else:
            for channel,prop in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:assert edit.connect_material_property(sample,channel,prop)
    edit.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat

def add_kit(name,x,y,z,yaw=0):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,y*100,z*100),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label('Egmore context '+name);a.static_mesh_component.set_static_mesh(kit_meshes[name])
    a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a
def add(name,x,y,z,yaw=0):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,y*100,z*100),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label(name);a.static_mesh_component.set_static_mesh(meshes[name]);a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
for x in [-30,-24,-18,-12,12,18,24,30]:add('EgmoreArcade6m',x,0,0)
for x in [-7,7]:add('EgmoreTower4m',x,0,0);add('EgmoreDome',x,1.5,12.2)
add('EgmoreEntrance9m',0,0,0)
for x in range(-30,31,6):add('EgmoreCanopy6m',x,10,.84)
for x in [-24,-12,0,12,24]:
    add('EgmorePlatformBench',x,14,.84)
    add('EgmoreCeilingFan',x,14,5.80)
for x in [-18,18]:add('EgmoreLitterBins',x,16,.84)
add('EgmoreWaterPoint',-6,12,.84,180)
add('EgmorePlatformBoard',6,17,2.9,180)
add('EgmoreNameboard4m',26,17,.84,180)
for x in range(-30,33,3):add('EgmoreDrain3m',x,10.25,.84)
# An estimated straight review corridor, not a claim of Egmore's surveyed yard.
# Platform edge y=20; logical rail plane z=0; centre offset 1.675 m.
track_centres=[21.675,27.175]
for centre in track_centres:
    for x in range(-45,48,3):
        add_kit('Rail3m',x,centre,-.1825)
        add('EgmoreBallast3m',x,centre,0)
        add_kit('ContactWire3m',x,centre,5.5)
    for i in range(155):add_kit('Sleeper',-45+i*.60,centre,-.18)
for x in [-40,0,40]:
    add_kit('OHEMast',x,30.475,0,180)
    add_kit('OHEMast',x,18.375,0)
for x in [-36,36]:add_kit('ElectricalCabinet',x,31.7,0,180)
platform_mesh=unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Meshes/SM_Station_Platform5m')
apron_mesh=unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Meshes/SM_Station_Apron5m')
if not platform_mesh or not apron_mesh:raise RuntimeError('Platform mesh missing from architectural review')
for x in range(-30,35,5):
    for y in [10,15]:
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,y*100,0))
        a.set_actor_label('Egmore review platform');a.static_mesh_component.set_static_mesh(platform_mesh if y==15 else apron_mesh)
ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-75))
ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
ground.set_actor_scale3d(unreal.Vector(500,500,.3))
ground.set_actor_label('Egmore continuous yard ground')
ground.static_mesh_component.set_material(0,yard_material())
# Estimated service access along the outer track; modular edge detail avoids
# the former isolated rectangular soil patch. It is not a surveyed boundary.
for x in range(-48,49,3):add_kit('Railing3m',x,35,-.6)
walk=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(150,3635,-60))
walk.set_actor_label('Egmore maintenance walkway')
walk.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
walk.set_actor_scale3d(unreal.Vector(99,2.5,.14))
walk.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreHeritage01/Materials/Concrete'))
for x in [-40,0,40]:
    for y in [30.475,18.375]:
        if y<20 and -30<=x<=35:continue # Within the existing platform foundation.
        support=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,y*100,-30))
        support.set_actor_label('Egmore mast foundation extension')
        support.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
        support.set_actor_scale3d(unreal.Vector(.9,.9,.6))
        support.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreHeritage01/Materials/Concrete'))
for x in [-36,36]:
    support=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x*100,3170,-30))
    support.set_actor_label('Egmore cabinet foundation extension')
    support.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    support.set_actor_scale3d(unreal.Vector(1.0,.9,.6))
    support.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreHeritage01/Materials/Concrete'))
foundation=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(250,1500,-30))
foundation.set_actor_label('Egmore platform foundation')
foundation.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
foundation.set_actor_scale3d(unreal.Vector(65,10,.60))
foundation.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Materials/Concrete'))
forecourt=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-30))
forecourt.set_actor_label('Egmore review forecourt')
forecourt.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
forecourt.set_actor_scale3d(unreal.Vector(75,20,.60))
forecourt.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Materials/Concrete'))
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-40,yaw=145,roll=0))
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property('intensity',10.0)
sun.light_component.set_editor_property('dynamic_shadow_distance_movable_light',12000.0)
sun.light_component.set_editor_property('dynamic_shadow_cascades',2)
sun.light_component.set_editor_property('cascade_distribution_exponent',1.5)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,2000))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/EngineResources/GrayTextureCube'))
sky.light_component.set_editor_property('intensity',0.45)
backdrop=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
backdrop.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
backdrop.static_mesh_component.set_material(0,unreal.load_asset('/Game/IndianRailway/S8/Stations/Kit01/Materials/M_DaySky'))
backdrop.static_mesh_component.set_editor_property('cast_shadow',False)
backdrop.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
backdrop.set_actor_scale3d(unreal.Vector(1500,1500,1500))
camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(2500,-7500,1500))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,600)),False)
runtime_camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(4500,4600,1000))
runtime_camera.set_actor_label('Egmore Android review camera')
runtime_camera.camera_component.set_editor_property('field_of_view',58.0)
runtime_camera.camera_component.set_editor_property('constrain_aspect_ratio',False)
runtime_camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(runtime_camera.get_actor_location(),unreal.Vector(0,1750,200)),False)
runtime_camera.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
review_exposure=unreal.PostProcessSettings()
review_exposure.set_editor_property('override_auto_exposure_method',True)
review_exposure.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
review_exposure.set_editor_property('override_auto_exposure_bias',True)
review_exposure.set_editor_property('auto_exposure_bias',1.5)
review_exposure.set_editor_property('override_auto_exposure_apply_physical_camera_exposure',True)
review_exposure.set_editor_property('auto_exposure_apply_physical_camera_exposure',False)
runtime_camera.camera_component.set_editor_property('post_process_settings',review_exposure)
levels.save_current_level()
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,camera.get_actor_location(),camera.get_actor_rotation())
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1800,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('post_process_settings',review_exposure)
component.set_editor_property('post_process_blend_weight',1.0)
# Continuous capture preserves exposure history during the timed review views.
# This actor is created after saving and never enters the packaged map.
component.set_editor_property('capture_every_frame',True)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('fov_angle',58)
placement={'kind':'estimated architectural review; not a surveyed Egmore layout',
 'platformEdgeYMetres':20,'trackCentreYMetres':track_centres,
 'platformOffsetMetres':track_centres[0]-20,'railGaugeMetres':1.676,
 'railTopZMetres':0,'contactWireZMetres':5.5,
 'groundMaterial':ground.static_mesh_component.get_material(0).get_path_name(),
 'groundTexturePeriodMetres':2.0,'skyFillIntensity':sky.light_component.get_editor_property('intensity'),
 'maintenanceRailingYMetres':35,'maintenanceWalkwayWidthMetres':2.5,
 'shadowDistanceMetres':120,'shadowCascades':2,'manualExposureBias':1.5,
 'stationIdentity':data['asset_id'] if 'asset_id' in data else 'Station.Egmore.Heritage.01'}
(ROOT/'07_Documentation/S8/Evidence/Egmore/review-corridor-placement.json').write_text(json.dumps(placement,indent=2))
start=time.monotonic();front_exported=False;canopy_exported=False
def finish(delta):
    global front_exported,canopy_exported
    if time.monotonic()-start<40:return
    if not front_exported:
        component.capture_scene()
        unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'07_Documentation/S8/Evidence/Egmore'),'unreal-study.png')
        capture.set_actor_location(unreal.Vector(1700,2350,400),False,False)
        capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(0,1450,330)),False)
        front_exported=True
        return
    if time.monotonic()-start<65:return
    if not canopy_exported:
        component.capture_scene()
        unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'07_Documentation/S8/Evidence/Egmore'),'unreal-canopy-study.png')
        capture.set_actor_location(unreal.Vector(4500,4600,1000),False,False)
        capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(0,1750,200)),False)
        canopy_exported=True
        return
    if time.monotonic()-start<90:return
    component.capture_scene()
    unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'07_Documentation/S8/Evidence/Egmore'),'unreal-corridor-study.png')
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
