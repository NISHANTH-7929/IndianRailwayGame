"""Author bounded, tileable ballast shoulders and a small angular stone.

Run with Blender --background --factory-startup --python <this file>.
Writes new assets only below 04_Assets/S9/TrackEnvironmentV1/MeshesV3Final. Metric
geometry is a presentation estimate, not a surveyed railway specification.
The existing track rails, sleepers and all S8 sources remain untouched.
"""
import bpy
import bmesh
import hashlib
import json
import math
import random
from pathlib import Path
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S9/TrackEnvironmentV1/MeshesV3Final'
if OUT.exists() and any(OUT.iterdir()):
    raise RuntimeError('Refusing to overwrite the track mesh revision: ' + str(OUT))
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = ROOT / '04_Assets/S8/Workstreams/StationArt/v2.0.0/Reproduce/build_v2.py'
SOURCE_DIGEST = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
material = bpy.data.materials.new('M_TrackBallast')
material.diffuse_color = (.24, .23, .20, 1)
material.use_nodes = True
shader = material.node_tree.nodes.get('Principled BSDF')
shader.inputs['Base Color'].default_value = material.diffuse_color
shader.inputs['Roughness'].default_value = .95
records = []


def finish(name, vertices, faces):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    assert all(edge.is_manifold for edge in bm.edges), 'Open mesh: ' + name
    assert bm.calc_volume(signed=True) > 0, 'Inverted mesh: ' + name
    bm.to_mesh(mesh)
    bm.free()
    uv = mesh.uv_layers.new(name='MetricUV')
    for polygon in mesh.polygons:
        # The bed material uses UV0.G as lateral metres for soil blending.
        # Keep XY on EVERY bed face: dominant-axis projection would change G
        # to height on steep shoulders and create repeated unblended patches.
        drop = max(range(3), key=lambda i: abs(polygon.normal[i]))
        axes = [0, 1] if name.startswith('SM_Ballast') else [i for i in range(3) if i != drop]
        for loop_index in polygon.loop_indices:
            co = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv.data[loop_index].uv = (co[axes[0]], co[axes[1]])
    for polygon in mesh.polygons:
        polygon.use_smooth = name.startswith('SM_Ballast') and abs(polygon.normal.x) < .95
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    return obj


def profile(x):
    # A 0.5 m straight collar on both ends guarantees matching seam positions
    # and face normals for any LOD combination. No random per-instance seam.
    edge_distance = min(x, 6.0-x)
    envelope = min(1.0, max(0.0, (edge_distance-.5)/.5))
    output = []
    for side in (-1, 1):
        phase = .65 if side == -1 else 2.32
        wave = (.053*math.sin(x*math.tau/3+phase)
                + .027*math.sin(x*math.tau/.75+phase*.71)) * envelope
        outer_y = side * (2.32+wave)
        outer_z = -.67 + envelope*.011*math.sin(x*math.tau/1.5+phase)
        mid_y = side * (1.96+wave*.56)
        mid_z = -.37 + envelope*.018*math.sin(x*math.tau/1.2+phase*.63)
        output.append((outer_y, outer_z, mid_y, mid_z))
    left, right = output
    return [(left[0],left[1]), (left[2],left[3]), (-1.73,-.235),
            (0.0,-.235), (1.73,-.235), (right[2],right[3]),
            (right[0],right[1]), (right[0],-.70), (left[0],-.70)]


def ballast(level, divisions):
    verts = [(6.0*i/divisions,y,z) for i in range(divisions+1)
             for y,z in profile(6.0*i/divisions)]
    width = len(profile(0))
    faces = []
    for i in range(divisions):
        for j in range(width):
            faces.append((i*width+j, i*width+(j+1)%width,
                          (i+1)*width+(j+1)%width, (i+1)*width+j))
    faces += [tuple(reversed(range(width))),
              tuple(divisions*width+j for j in range(width))]
    return finish('SM_Ballast6m_RealismV3_LOD%d' % level, verts, faces)


def stone(level):
    # A naturally faceted convex solid, not a smoothed sphere or a cube. The
    # runtime's centimetre-scale instance transforms supply the final size.
    if level == 2:
        return finish('SM_AngularBallastStoneV1_LOD2',
            [(-.5,0,.02),(.5,.03,-.04),(0,-.5,-.02),(.04,.5,0),
             (.02,-.03,.5),(-.06,.02,-.5)],
            [(0,2,4),(2,1,4),(1,3,4),(3,0,4),
             (2,0,5),(1,2,5),(3,1,5),(0,3,5)])
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1)
    temporary = bpy.context.object
    rng = random.Random(12683)
    for vertex in temporary.data.vertices:
        vertex.co *= rng.uniform(.83, 1.12)
    if level == 1:
        modifier = temporary.modifiers.new('Retain angular silhouette', 'DECIMATE')
        modifier.ratio = .61
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    minimum = [min(v.co[i] for v in temporary.data.vertices) for i in range(3)]
    maximum = [max(v.co[i] for v in temporary.data.vertices) for i in range(3)]
    vertices = [tuple((v.co[i]-minimum[i])/(maximum[i]-minimum[i])-.5
                      for i in range(3)) for v in temporary.data.vertices]
    faces = [tuple(p.vertices) for p in temporary.data.polygons]
    bpy.data.objects.remove(temporary, do_unlink=True)
    return finish('SM_AngularBallastStoneV1_LOD%d' % level, vertices, faces)


def export(obj, level, family):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.data.calc_loop_triangles()
    triangles = len(obj.data.loop_triangles)
    bounds = [[round(fn(v.co[i] for v in obj.data.vertices), 7)
               for i in range(3)] for fn in (min, max)]
    assert triangles <= 2000
    if family == 'ballast':
        uv_error = max(abs(obj.data.uv_layers[0].data[loop.index].uv[axis]
                           -obj.data.vertices[loop.vertex_index].co[axis])
                       for loop in obj.data.loops for axis in (0,1))
        assert uv_error < 1e-6, 'Every bed UV must encode longitudinal/lateral metres'
        assert bounds[0][0] == 0 and bounds[1][0] == 6
        assert max(abs(bounds[0][1]),abs(bounds[1][1])) <= 2.44
        assert abs(bounds[1][2]+.235) < 1e-6
        assert abs(bounds[0][2]+.70) < 1e-6, 'Bed skirt must overlap terrain top at -0.65 m'
        start = [(round(v.co.y,7),round(v.co.z,7)) for v in obj.data.vertices if abs(v.co.x) < 1e-7]
        end = [(round(v.co.y,7),round(v.co.z,7)) for v in obj.data.vertices if abs(v.co.x-6) < 1e-7]
        assert start == end, 'Tile seam differs'
    else:
        uv_error = None
        start = end = None
        assert bounds == [[-.5,-.5,-.5],[.5,.5,.5]]
    # Match prepare_egmore_scene02.py: compensate for the UE force-front-X
    # importer handedness, leaving authored Blender coordinates unchanged.
    original = obj.data
    obj.data = original.copy()
    obj.data.transform(Matrix.Diagonal((1,-1,1,1)))
    obj.data.flip_normals()
    obj.data.update()
    target = OUT / (obj.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True,
        object_types={'MESH'}, axis_forward='X', axis_up='Z', use_triangles=True,
        apply_unit_scale=True, path_mode='STRIP', add_leaf_bones=False,
        mesh_smooth_type='FACE')
    exported = obj.data
    obj.data = original
    bpy.data.meshes.remove(exported)
    records.append(dict(family=family, name=obj.name, lod=level, triangles=triangles,
                        bounds_m=bounds, fbx=str(target.relative_to(ROOT)),
                        sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                        material_slots=['M_TrackBallast'], closed_manifold=True,
                        seam_cross_section_m=start, seam_matches=(start == end),
                        all_faces_metric_xy_uv=(family == 'ballast'),
                        maximum_metric_xy_uv_error=uv_error))


for level, divisions in enumerate((24,12,6)):
    export(ballast(level,divisions), level, 'ballast')

for family in ('ballast',):
    counts = [r['triangles'] for r in records if r['family'] == family]
    assert counts[0] > counts[1] > counts[2], counts
assert all(row['seam_cross_section_m'] == records[0]['seam_cross_section_m']
           for row in records if row['family'] == 'ballast')
bpy.context.preferences.filepaths.save_version = 0
blend = OUT/'TrackBedV3.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == SOURCE_DIGEST
receipt = dict(revision='track-environment-mesh-v3', source_reference=str(SOURCE.relative_to(ROOT)),
    source_sha256=SOURCE_DIGEST, original_sources_unchanged=True,
    units='metres; UE import with convert_scene_unit=True yields centimetres',
    basis='X longitudinal, Y lateral, Z up; FBX X-forward with Y-reflection compensation',
    uv_contract='Bed MetricUV channel 0 is XY metres on every face; stone uses dominant-axis metric UVs',
    geometry_contract=dict(length_m=6, rail_top_m=0, inner_flat_half_width_m=1.73,
                           inner_flat_height_m=-.235, nominal_outer_half_width_m=2.32,
                           nominal_outer_height_m=-.67, underside_height_m=-.70,
                           ground_overlap_m=.05, seam_collar_m=.5,
                           maximum_half_width_m=2.44, collision=False),
    records=records, blend=str(blend.relative_to(ROOT)),
    blend_sha256=hashlib.sha256(blend.read_bytes()).hexdigest(),
    limitations=['Presentation estimate, not surveyed railway construction.',
                 'Ballast surface raised to embed PSC sleepers; gauge and rail height unchanged.',
                 'Unreal import, runtime material and in-game checks are separate.'])
(OUT/'mesh-receipt.json').write_text(json.dumps(receipt,indent=2))
print('TRACK_BED_V3_READY',json.dumps([dict(name=r['name'],triangles=r['triangles']) for r in records]))
