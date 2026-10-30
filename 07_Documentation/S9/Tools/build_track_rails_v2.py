"""Author shaped rails, tapered PSC sleepers and curved elastic fastenings.

Blender --background --factory-startup --python <this file>.
This is a bounded presentation model, not a certified engineering drawing.
New sources only: RailsV2. S8 assets and the previous revision stay untouched.
"""
import hashlib
import json
import math
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector


ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S9/TrackEnvironmentV1/RailsV2'
assert not OUT.exists() or not any(OUT.iterdir()), 'Preserve the previous rail revision'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
materials = []
for name, color, metallic, roughness in (
        ('RailHead', (.37, .39, .40, 1), .85, .29),
        ('RailRust', (.15, .075, .038, 1), .20, .84),
        ('Concrete', (.36, .35, .31, 1), 0., .91)):
    material = bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    shader = material.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = color
    shader.inputs['Metallic'].default_value = metallic
    shader.inputs['Roughness'].default_value = roughness
    materials.append(material)


class Geometry:
    def __init__(self):
        self.vertices = []
        self.faces = []
        self.slots = []
        self.smooth = []

    def add(self, vertices, faces, slot=0, smooth=False):
        offset = len(self.vertices)
        self.vertices.extend(vertices)
        self.faces.extend(tuple(offset + index for index in face) for face in faces)
        self.slots.extend([slot] * len(faces) if isinstance(slot, int) else slot)
        self.smooth.extend([smooth] * len(faces))

    def box(self, low, high, slot):
        x0, y0, z0 = low
        x1, y1, z1 = high
        self.add([(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
                  (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)],
                 [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
                  (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)], slot)

    def tube(self, points, radius=.009, sides=5):
        # The rod follows an actual bent path. Its shallow crown and toe are
        # deliberately below the rail head, with the heel rooted in its boss.
        vertices = []
        points = [Vector(p) for p in points]
        for index, point in enumerate(points):
            tangent = (points[min(index + 1, len(points) - 1)] -
                       points[max(index - 1, 0)]).normalized()
            normal = tangent.cross(Vector((0, 0, 1))).normalized()
            if normal.length < .5:
                normal = tangent.cross(Vector((1, 0, 0))).normalized()
            binormal = tangent.cross(normal).normalized()
            for side in range(sides):
                angle = 2 * math.pi * side / sides
                vertices.append(tuple(point + radius * (
                    math.cos(angle) * normal + math.sin(angle) * binormal)))
        faces = []
        for ring in range(len(points) - 1):
            for side in range(sides):
                other = (side + 1) % sides
                faces.append((ring * sides + side, ring * sides + other,
                              (ring + 1) * sides + other, (ring + 1) * sides + side))
        faces += [tuple(range(sides - 1, -1, -1)),
                  tuple((len(points) - 1) * sides + i for i in range(sides))]
        self.add(vertices, faces, 1, True)


def add_rail(g, centre, lod):
    # Cross section in Y/Z: crown, head cheeks, neck, web, sloping foot and
    # underside. A single connected profile replaces the old three boxes.
    if lod == 0:
        section = [(-.024, 0), (.024, 0), (.032, -.003), (.035, -.011),
                   (.034, -.035), (.028, -.043), (.010, -.052), (.008, -.128),
                   (.023, -.144), (.075, -.153), (.075, -.167),
                   (-.075, -.167), (-.075, -.153), (-.023, -.144),
                   (-.008, -.128), (-.010, -.052), (-.028, -.043),
                   (-.034, -.035), (-.035, -.011), (-.032, -.003)]
    else:
        section = [(-.035, 0), (.035, 0), (.035, -.039), (.009, -.052),
                   (.008, -.137), (.075, -.153), (.075, -.167),
                   (-.075, -.167), (-.075, -.153), (-.008, -.137),
                   (-.009, -.052), (-.035, -.039)]
    count = len(section)
    vertices = [(x, centre + y, z) for x in (0., 6.) for y, z in section]
    faces, slots = [], []
    for i, (y, z) in enumerate(section):
        j = (i + 1) % count
        faces.append((i, j, count + j, count + i))
        # Only the rolling band and its bevels are polished. Cheeks, web and
        # foot retain oxidised steel; the polished top does not wrap the foot.
        slots.append(0 if min(z, section[j][1]) >= -.011 else 1)
    faces += [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    slots += [1, 1]
    g.add(vertices, faces, slots)


def sleeper_section(y, detail):
    a = abs(y)
    if a >= 1.374:
        top, upper, lower = -.190, .110, .135
    elif a >= 1.15:
        top, upper, lower = -.185, .115, .138
    elif a >= .744:
        top, upper, lower = -.175, .125, .140
    elif a >= .52:
        top, upper, lower = -.205, .107, .130
    else:
        top, upper, lower = -.217, .100, .125
    if detail:
        b = .006
        return [(-upper + b, top), (upper - b, top), (upper, top - b),
                (lower, -.329), (lower - b, -.335), (-lower + b, -.335),
                (-lower, -.329), (-upper, top - b)]
    return [(-upper, top), (upper, top), (lower, -.335), (-lower, -.335)]


def add_sleeper(g, x, lod):
    ys = ([-1.375, -1.16, -1.005, -.745, -.53, 0., .53, .745, 1.005, 1.16, 1.375]
          if lod == 0 else [-1.375, -1.005, -.745, 0., .745, 1.005, 1.375])
    if lod == 2:
        # Retain the real tapered outline, centre dip and rail-seat humps,
        # but omit hidden buried faces at this range.
        vertices = [(x + dx, y, sleeper_section(y, False)[0][1])
                    for y in ys for dx in (-sleeper_section(y, False)[1][0],
                                           sleeper_section(y, False)[1][0])]
        faces = [(i * 2, i * 2 + 1, i * 2 + 3, i * 2 + 2) for i in range(len(ys) - 1)]
        g.add(vertices, faces, 2)
        return
    sections = [sleeper_section(y, lod == 0) for y in ys]
    n = len(sections[0])
    vertices = [(x + dx, y, z) for y, section in zip(ys, sections) for dx, z in section]
    faces = [(j * n + k, j * n + (k + 1) % n,
              (j + 1) * n + (k + 1) % n, (j + 1) * n + k)
             for j in range(len(ys) - 1) for k in range(n)]
    faces += [tuple(range(n - 1, -1, -1)), tuple(range((len(ys) - 1) * n, len(ys) * n))]
    g.add(vertices, faces, 2)


def fastening_path(x, rail, side):
    return [(x - .046, rail + side * .118, -.164),
            (x - .033, rail + side * .129, -.132),
            (x + .022, rail + side * .121, -.124),
            (x + .047, rail + side * .087, -.133),
            (x + .016, rail + side * .055, -.151)]


def add_fasteners(g, x, lod):
    if lod == 2:
        return
    for rail in (-.873, .873):
        if lod == 0:
            g.box((x - .105, rail - .089, -.175), (x + .105, rail + .089, -.167), 1)
        else:
            g.add([(x - .105, rail - .089, -.167), (x + .105, rail - .089, -.167),
                   (x + .105, rail + .089, -.167), (x - .105, rail + .089, -.167)],
                  [(0, 1, 2, 3)], 1)
        for side in (-1, 1):
            path = fastening_path(x, rail, side)
            if lod == 0:
                # Small anchor shoulder beneath the bent rod; it is mostly
                # occluded and cannot read as a row of oversized grey chips.
                cy = rail + side * .119
                g.box((x - .062, cy - .018, -.177), (x - .025, cy + .018, -.149), 1)
                g.tube(path)
            else:
                # A narrow ribbon keeps the curved toe/heel silhouette at
                # middle range for eight triangles per fastening.
                vertices = [(px, py, pz + dz) for px, py, pz in path for dz in (-.006, .006)]
                faces = [(i * 2, i * 2 + 1, i * 2 + 3, i * 2 + 2) for i in range(4)]
                g.add(vertices, faces, 1)


records = []
for lod in range(3):
    g = Geometry()
    for centre in (-.873, .873):
        add_rail(g, centre, lod)
    for index in range(10):
        x = index * .6 + .3
        add_sleeper(g, x, lod)
        add_fasteners(g, x, lod)
    mesh = bpy.data.meshes.new('SM_Track6m_RealismV2_LOD' + str(lod))
    mesh.from_pydata(g.vertices, [], g.faces)
    for material in materials:
        mesh.materials.append(material)
    for polygon, slot, smooth in zip(mesh.polygons, g.slots, g.smooth):
        polygon.material_index = slot
        polygon.use_smooth = smooth
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    if lod == 0:
        assert all(edge.is_manifold for edge in bm.edges), 'Close geometry must be closed'
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    uv = mesh.uv_layers.new(name='MetricUV')
    for polygon in mesh.polygons:
        axes = [i for i in range(3) if i != max(range(3), key=lambda j: abs(polygon.normal[j]))]
        for index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[index].vertex_index].co
            uv.data[index].uv = (vertex[axes[0]], vertex[axes[1]])
    mesh.calc_loop_triangles()
    count = len(mesh.loop_triangles)
    assert count <= (5000, 1000, 300)[lod], (lod, count)
    bounds = [[min(v.co[i] for v in mesh.vertices) for i in range(3)],
              [max(v.co[i] for v in mesh.vertices) for i in range(3)]]
    assert abs(bounds[0][0]) < 1e-6 and abs(bounds[1][0] - 6.) < 1e-6
    assert abs(bounds[0][1] + 1.375) < 1e-6 and abs(bounds[1][1] - 1.375) < 1e-6
    assert abs(bounds[1][2]) < 1e-6
    if lod < 2:
        assert abs(bounds[0][2] + .335) < 1e-6
    # Match the established UE FBX conversion and handedness contract.
    mesh.transform(Matrix.Diagonal((1, -1, 1, 1)))
    mesh.flip_normals()
    mesh.update()
    obj = bpy.data.objects.new(mesh.name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = OUT / (mesh.name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                            axis_forward='X', axis_up='Z', use_triangles=True,
                            apply_unit_scale=True, path_mode='STRIP', add_leaf_bones=False,
                            mesh_smooth_type='FACE')
    records.append(dict(lod=lod, triangles=count, boundsMetres=bounds,
                        fbx=str(path.relative_to(ROOT)), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                        shapedRailHeadWebFootPreserved=True, taperedSleeperTopPreserved=True,
                        fastening='curved 18 mm rods' if lod == 0 else 'curved silhouettes' if lod == 1 else 'omitted'))
    obj.hide_set(True)
    obj.select_set(False)

receipt = dict(revision='track-rails-v2', records=records, gaugeInsideM=1.676,
               railCentresM=[-.873, .873], railHeadMaxWidthM=.070, railFootWidthM=.150,
               railTopM=0., railBaseM=-.167, sleeperRailSeatM=-.175, railSeatPadThicknessM=.008,
               sleeperCentreTopM=-.217, sleeperEndTopM=-.190, sleeperBottomM=-.335,
               sleeperLengthM=2.75, sleeperSpacingM=.6, sleepersPerTile=10, lengthM=6.,
               recommendedBallastTopM=-.215, elasticClipRodDiameterM=.018,
               materialSlots=['RailHead', 'RailRust', 'Concrete'],
               estimatedPresentation=True, originalS8AssetsModified=False)
(OUT / 'rails-receipt.json').write_text(json.dumps(receipt, indent=2))
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'TrackRailsV2.blend'))
print(json.dumps(receipt, indent=2))
