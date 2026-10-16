"""Non-release LHB cleanup candidate; retain source units for comparison.

No source overwrite, inferred dimensional corrections, or runtime binding.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431'
OUT = SOURCE / 'Review'
OUT.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(SOURCE/'Original/source/red coach LHB.blend'), load_ui=False, use_scripts=False)
report = {'status': 'UNACCEPTED: source-scale cleanup study only', 'changes': [], 'objects': []}
dg = bpy.context.evaluated_depsgraph_get()
keep = []
for obj in list(bpy.context.scene.objects):
    if obj.type not in {'MESH', 'CURVE', 'FONT'}:
        continue
    bounds = [obj.matrix_world @ Vector(c) for c in obj.evaluated_get(dg).bound_box]
    center = (min(p.x for p in bounds)+max(p.x for p in bounds))/2
    if center >= 6.33:
        continue
    keep.append(obj)
    for mod in obj.modifiers:
        if mod.type == 'SUBSURF':
            report['changes'].append({'object': obj.name, 'change': 'disable render subdivision for comparison',
                                      'levels_before': mod.levels, 'render_levels_before': mod.render_levels})
            mod.show_viewport = False
            mod.show_render = False
for obj in list(bpy.context.scene.objects):
    if obj not in keep:
        bpy.data.objects.remove(obj, do_unlink=True)
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = keep[0]
bpy.ops.object.convert(target='MESH')
for obj in list(bpy.context.scene.objects):
    if obj.type != 'MESH': continue
    mesh = obj.data
    mesh.calc_loop_triangles()
    before = len(mesh.loop_triangles)
    if obj.name.startswith('Bolt'):
        bpy.context.view_layer.objects.active = obj
        modifier = obj.modifiers.new('Coplanar cleanup', 'DECIMATE')
        modifier.decimate_type = 'DISSOLVE'
        modifier.angle_limit = math.radians(3)
        modifier.delimit = {'NORMAL', 'MATERIAL', 'UV'}
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.calc_loop_triangles()
    report['objects'].append({'name': obj.name, 'before': before, 'after': len(obj.data.loop_triangles)})
# Drop orphan third-party image/material datablocks from this candidate only.
for mat in list(bpy.data.materials):
    if mat.users == 0: bpy.data.materials.remove(mat)
for im in list(bpy.data.images):
    if im.users == 0: bpy.data.images.remove(im)
report['triangles'] = sum(o['after'] for o in report['objects'])
report['dimensions_corrected'] = False
report['lods_prepared'] = False
report['runtime_integrated'] = False
report['notes'] = ['Single coach selected by inspected bounds; review selection before production.',
                   'Font fallback is visible in this study; source font missing, lettering not accepted.',
                   'No final triangle budget. No dimensions or bogie pivots invented.']
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LHB_CleanupStudy.blend'))
(OUT/'cleanup-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=16
scene.render.resolution_x=1400
scene.render.resolution_y=700
scene.render.resolution_percentage=100
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.35,.35,.35,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
bpy.ops.object.light_add(type='AREA',location=(0,-5,10))
bpy.context.object.data.energy=1800
bpy.context.object.data.shape='DISK'
bpy.context.object.data.size=8
bpy.ops.object.camera_add(location=(10,-17,8))
camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,1.5))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'
camera.data.ortho_scale=16
scene.camera=camera
scene.render.filepath=str(OUT/'cleanup-study.png')
bpy.ops.render.render(write_still=True)
print('LHB_CLEANUP_STUDY',report['triangles'])
