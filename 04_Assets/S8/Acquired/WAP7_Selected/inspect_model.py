import bpy, json, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(ROOT/'Original/uvwag7newWS.glb'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
points=[o.matrix_world@Vector(p) for o in objects for p in o.bound_box]
lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
report={'bounds_min':list(lo),'bounds_max':list(hi),'dimensions':list(hi-lo),'objects':[]}
for o in objects:
    o.data.calc_loop_triangles()
    report['objects'].append({'name':o.name,'dimensions':list(o.dimensions),'location':list(o.location),'triangles':len(o.data.loop_triangles),'materials':[m.name if m else None for m in o.data.materials]})
report['triangles']=sum(o['triangles'] for o in report['objects'])
report['images']=[{'name':i.name,'size':list(i.size),'path':i.filepath} for i in bpy.data.images]
(ROOT/'inspection.json').write_text(json.dumps(report,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'WAP7_Original.blend'))
center=(lo+hi)/2
extent=max(hi-lo)
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.22,.25,.3,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.6
for offset,power,size in [((.3,-.4,1),1500,.7),((-.4,.5,.5),1000,.5)]:
    bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*extent)
    light=bpy.context.object;light.data.energy=power*extent*extent/100;light.data.shape='DISK';light.data.size=extent*size
    light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=center+Vector((.85,-1.1,.65))*extent)
camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=extent*1.2;camera.data.clip_end=extent*20;scene.camera=camera
scene.render.filepath=str(ROOT/'WAP7_Original_preview.png')
bpy.ops.render.render(write_still=True)
print(json.dumps(report))

