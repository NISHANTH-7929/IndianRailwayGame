"""Export a small hardware derivative of the repaired WAP-7 cab.

Run with Blender --background --factory-startup --python <this file>.
Only 04_Assets/S9/CabHardwareV1 is written. The existing cab is the input,
not the historical generator, so its repaired contact/detail UVs survive.
Fittings are inferred presentation details, not a certified fleet layout.
"""
import bpy
import hashlib
import json
import math
import struct
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / '04_Assets/S8/Cabs/WAP7'
OUT = ROOT / '04_Assets/S9/CabHardwareV1'
OUT.mkdir(parents=True, exist_ok=True)
SOURCE_BLEND = SOURCE / 'WAP7_Cab.blend'
SOURCE_ASSEMBLY = SOURCE / 'assembly.json'
source_hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in (SOURCE_BLEND, SOURCE_ASSEMBLY)}
manifest = json.loads(SOURCE_ASSEMBLY.read_text())
bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
core = bpy.data.objects['SM_WAP7_Cab_StaticCore']
assert [uv.name for uv in core.data.uv_layers][:2] == ['CabContactUV', 'DetailUV']
core.data.calc_loop_triangles()
baseline_triangles = len(core.data.loop_triangles)
assert baseline_triangles == next(p['triangles'] for p in manifest['parts'] if p['name'] == 'StaticCore')
baseline_cab_triangles = manifest['triangles']
baseline_materials = [m.name for m in core.data.materials]
original_vertices = len(core.data.vertices)
original_polygons = len(core.data.polygons)
original_loops = len(core.data.loops)
original_matrix = [tuple(row) for row in core.matrix_world]


def uv_hash(mesh, name, count):
    digest = hashlib.sha256()
    for value in mesh.uv_layers[name].data[:count]:
        digest.update(struct.pack('<2f', *value.uv))
    return digest.hexdigest()


def geometry_hash(obj, vertices=None, polygons=None):
    digest = hashlib.sha256()
    for vertex in obj.data.vertices[:vertices]:
        digest.update(struct.pack('<3f', *vertex.co))
    for polygon in obj.data.polygons[:polygons]:
        digest.update(struct.pack('<I', len(polygon.vertices)))
        digest.update(struct.pack('<' + 'I' * len(polygon.vertices), *polygon.vertices))
        digest.update(obj.data.materials[polygon.material_index].name.encode())
        digest.update(bytes([polygon.use_smooth]))
    return digest.hexdigest()


uv_before = {uv.name: uv_hash(core.data, uv.name, original_loops) for uv in core.data.uv_layers}
geometry_before = geometry_hash(core)
unchanged_parts = {o.name: (geometry_hash(o), [tuple(row) for row in o.matrix_world])
                   for o in bpy.data.objects if o.type == 'MESH' and o != core}
extras = []
additions = []


def finish(obj, feature, material):
    assert material in ('Steel', 'Rubber')
    obj.data.materials.clear()
    obj.data.materials.append(bpy.data.materials[material])
    # Plain materials have no baked AO contract. Their new polygons sample the
    # same simple box-projected detail coordinates in both UV channels.
    for name in uv_before:
        layer = obj.data.uv_layers.get(name) or obj.data.uv_layers.new(name=name)
        for polygon in obj.data.polygons:
            normal = polygon.normal
            drop = max(range(3), key=lambda axis: abs(normal[axis]))
            axes = [axis for axis in range(3) if axis != drop]
            for loop_index in polygon.loop_indices:
                point = obj.data.vertices[obj.data.loops[loop_index].vertex_index].co
                layer.data[loop_index].uv = (point[axes[0]] * 10, point[axes[1]] * 10)
    obj.data.calc_loop_triangles()
    additions.append(dict(name=obj.name, feature=feature, material=material,
                          triangles=len(obj.data.loop_triangles),
                          centre_metres=list(obj.location)))
    extras.append(obj)
    return obj


def box(name, location, dimensions, feature, material='Steel'):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, feature, material)


def rod(name, start, end, radius, feature, material='Steel', sides=8):
    a, b = Vector(start), Vector(end)
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius, depth=(b-a).length,
                                       location=(a+b)/2)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = (b-a).to_track_quat('Z', 'Y').to_euler()
    for polygon in obj.data.polygons:
        polygon.use_smooth = len(polygon.vertices) == 4
    return finish(obj, feature, material)


def slot(name, x, y, z, feature, width=.0036, height=.0007):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(0, -width/2, -height/2), (0, width/2, -height/2),
                      (0, width/2, height/2), (0, -width/2, height/2)], [], [(3, 2, 1, 0)])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.location = (x, y, z)
    return finish(obj, feature, 'Rubber')


# Original generator: visor centre (9.39, +/-0.66, 3.86), size .025/1.01/.20;
# header centre (9.52,0,4), size .17/2.76/.14. All mounts stay at the visor's
# upper edge, behind the existing silhouette, never across the clear pane.
for side in (-1, 1):
    for offset in (-.31, .31):
        y = side * .66 + offset
        box('Visor upper mounting strap', (9.415, y, 3.982), (.080, .050, .012), 'visor_mounts')
        rod('Visor hinge barrel', (9.382, y-.035, 3.958), (9.382, y+.035, 3.958), .010, 'visor_mounts')
        rod('Visor mounting screw', (9.371, y, 3.981), (9.376, y, 3.981), .0036,
            'visor_mounts', sides=6)
        slot('Visor screw slot', 9.3708, y, 3.981, 'visor_mounts')

# Original recorder housing centre (9.22,.12,3.51), dimensions .12/.215/.36.
# The bracket stays behind its lower edge. The conduit follows its underside
# into the existing console top rim at x9.245,z3.202; it adds no indication.
box('Recorder lower support', (9.282, .12, 3.345), (.024, .16, .035), 'recorder_mount')
conduit = [(9.270,.12,3.334), (9.270,.12,3.295), (9.250,.12,3.270), (9.250,.12,3.202)]
for i, (a,b) in enumerate(zip(conduit, conduit[1:])):
    rod('Recorder protected conduit %d' % i, a, b, .008, 'recorder_mount', 'Rubber')
for x,z in [(9.270,3.326), (9.250,3.218)]:
    rod('Recorder conduit ferrule', (x,.12,z-.008), (x,.12,z+.008), .010,
        'recorder_mount', sides=8)
for y in (.045,.195):
    rod('Recorder lower face fastener', (9.1575,y,3.345), (9.1615,y,3.345), .0035,
        'recorder_mount', sides=6)
    slot('Recorder fastener slot', 9.1573,y,3.345,'recorder_mount')

# Only add dark recesses on the existing captive heads. Follow the exact
# original panel warp, rather than moving the panel, lettering or instruments.
for centre,width in [(1.12,.43),(.48,.79),(-.43,1.00)]:
    for y in (centre-width/2+.02,centre+width/2-.02):
        for z in (2.805,3.175):
            x = 9.1013 + (z-2.78)*.24
            if y > .91:
                x -= .10*(y-.91)
            elif y < -.04:
                x += .10*(y+.04)
            slot('Instrument panel screw slot',x,y,z,'panel_fastener_recesses')

added_triangles = sum(row['triangles'] for row in additions)
assert added_triangles <= 650, added_triangles
bpy.ops.object.select_all(action='DESELECT')
for obj in [core] + extras:
    obj.select_set(True)
bpy.context.view_layer.objects.active = core
bpy.ops.object.join()
core.data.calc_loop_triangles()
assert geometry_hash(core, original_vertices, original_polygons) == geometry_before
assert [tuple(row) for row in core.matrix_world] == original_matrix
uv_after = {name: uv_hash(core.data, name, original_loops) for name in uv_before}
assert uv_after == uv_before, 'Existing contact/detail UVs changed'
assert [m.name for m in core.data.materials][:len(baseline_materials)] == baseline_materials
assert len(core.data.materials) == len(baseline_materials), 'New material slot created'
for name, (digest, matrix) in unchanged_parts.items():
    obj = bpy.data.objects[name]
    assert geometry_hash(obj) == digest and [tuple(row) for row in obj.matrix_world] == matrix, name
final_triangles = len(core.data.loop_triangles)
assert final_triangles == baseline_triangles + added_triangles
final_cab_triangles = baseline_cab_triangles + added_triangles
assert final_cab_triangles < 50000

core.name = 'SM_WAP7_Cab_StaticCore_HardwareV1'
fbx = OUT / (core.name + '.fbx')
saved_location = core.location.copy()
core.location = (0,0,0)
bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'MESH'},
                         axis_forward='X', axis_up='Z', apply_unit_scale=True,
                         add_leaf_bones=False, use_triangles=True,
                         mesh_smooth_type='FACE', path_mode='STRIP')
core.location = saved_location
# Resolve existing textures absolutely; no original source assets are saved.
for image in bpy.data.images:
    if image.source == 'FILE':
        candidate = SOURCE / 'Textures' / Path(bpy.path.abspath(image.filepath)).name
        if candidate.exists():
            image.filepath = str(candidate)
bpy.context.preferences.filepaths.save_version = 0
blend = OUT / 'WAP7_Cab_HardwareV1.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
for relative, digest in source_hashes.items():
    assert hashlib.sha256((ROOT/relative).read_bytes()).hexdigest() == digest, 'Source changed'
receipt = dict(revision='cab-hardware-v1-20261023', source_sha256=source_hashes,
               source_object='SM_WAP7_Cab_StaticCore', replacement_object=core.name,
               baseline_static_triangles=baseline_triangles, final_static_triangles=final_triangles,
               baseline_cab_triangles=baseline_cab_triangles, final_cab_triangles=final_cab_triangles,
               added_triangles=added_triangles, added_runtime_components=0,
               original_geometry_preserved=True, original_transforms_preserved=True,
               other_parts_preserved=True, original_uvs_preserved=True,
               original_uv_sha256=uv_before, materials_added=[],
               additions=additions, fbx=str(fbx.relative_to(ROOT)), blend=str(blend.relative_to(ROOT)),
               output_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (fbx,blend)},
               limitations=['Small fittings are inferred presentation details; not a measured fleet reproduction.',
                            'No new controls, gauge channels, lights, collisions or moving parts.'])
(OUT/'receipt.json').write_text(json.dumps(receipt, indent=2))
print('CAB_HARDWARE_V1_READY', json.dumps({k:receipt[k] for k in ('added_triangles','final_cab_triangles','original_uvs_preserved')}))
