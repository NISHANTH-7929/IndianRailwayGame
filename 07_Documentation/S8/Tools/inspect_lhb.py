"""Read-only inventory of the supplied LHB blend; never save the source."""
import bpy, json, math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431'
bpy.ops.wm.open_mainfile(filepath=str(OUT / 'Original/source/red coach LHB.blend'), load_ui=False, use_scripts=False)
report = {'units': {'system': bpy.context.scene.unit_settings.system,
                    'scale_length': bpy.context.scene.unit_settings.scale_length},
          'objects': [], 'materials': [], 'images': [],
          'actions': [a.name for a in bpy.data.actions], 'evaluated_triangles': 0}
depsgraph = bpy.context.evaluated_depsgraph_get()
all_corners = []
for obj in bpy.context.scene.objects:
    item = {'name': obj.name, 'type': obj.type, 'location': list(obj.location),
            'scale': list(obj.scale), 'rotation': list(obj.rotation_euler),
            'parent': obj.parent.name if obj.parent else None,
            'modifiers': [{'name': m.name, 'type': m.type} for m in obj.modifiers],
            'animation_data': bool(obj.animation_data)}
    if obj.type in {'MESH', 'CURVE', 'FONT'}:
        evaluated = obj.evaluated_get(depsgraph)
        emesh = evaluated.to_mesh()
        emesh.calc_loop_triangles()
        item['evaluated_triangles'] = len(emesh.loop_triangles)
        report['evaluated_triangles'] += len(emesh.loop_triangles)
        evaluated.to_mesh_clear()
        corners = [evaluated.matrix_world @ Vector(c) for c in evaluated.bound_box]
        all_corners.extend(corners)
        item['evaluated_bounds'] = [[min(p[i] for p in corners) for i in range(3)], [max(p[i] for p in corners) for i in range(3)]]
    if obj.type == 'FONT':
        item['text'] = obj.data.body
        item['font'] = obj.data.font.filepath
    if obj.type == 'MESH':
        mesh = obj.data
        mesh.calc_loop_triangles()
        corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
        item.update(vertices=len(mesh.vertices), polygons=len(mesh.polygons),
                    triangles=len(mesh.loop_triangles),
                    ngons=sum(len(p.vertices)>4 for p in mesh.polygons),
                    uv_layers=[u.name for u in mesh.uv_layers],
                    materials=[m.name if m else None for m in mesh.materials],
                    bounds_min=[min(p[i] for p in corners) for i in range(3)],
                    bounds_max=[max(p[i] for p in corners) for i in range(3)])
    report['objects'].append(item)
for mat in bpy.data.materials:
    item = {'name': mat.name, 'diffuse_color': list(mat.diffuse_color), 'nodes': [],
            'object_users': [o.name for o in bpy.context.scene.objects if hasattr(o.data, 'materials') and mat.name in o.data.materials]}
    if mat.use_nodes:
        for node in mat.node_tree.nodes:
            entry = {'type': node.type, 'name': node.name}
            if node.type == 'BSDF_PRINCIPLED':
                entry['inputs'] = {s.name: (list(s.default_value) if hasattr(s.default_value, '__len__') else s.default_value)
                                   for s in node.inputs if hasattr(s, 'default_value') and s.name in
                                   ['Base Color', 'Metallic', 'Roughness', 'Alpha', 'Transmission Weight']}
            if node.type == 'TEX_IMAGE': entry['image'] = node.image.name if node.image else None
            item['nodes'].append(entry)
    report['materials'].append(item)
for im in bpy.data.images:
    report['images'].append({'name': im.name, 'path': im.filepath, 'size': list(im.size), 'packed': bool(im.packed_file)})
report['scene_bounds'] = [[min(p[i] for p in all_corners) for i in range(3)], [max(p[i] for p in all_corners) for i in range(3)]]
(OUT / 'source-inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('LHB_INSPECTED', len(report['objects']), 'objects', sum(o.get('triangles',0) for o in report['objects']), 'base triangles')
# Inspection render only. Source is never saved; camera lives in this process.
scene = bpy.context.scene
scene.render.engine = 'BLENDER_WORKBENCH'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.background_type = 'WORLD'
scene.world.color = (.12,.12,.12)
scene.render.resolution_x = 1600
scene.render.resolution_y = 650
scene.render.resolution_percentage = 100
bpy.ops.object.camera_add(location=(22,-30,18))
camera = bpy.context.object
camera.rotation_euler = (Vector((6,0,1.5))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 32
scene.camera = camera
scene.render.filepath = str(OUT / 'source-inspection.png')
bpy.ops.render.render(write_still=True)
