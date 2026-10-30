"""Author small folded leaves, curved verge grass and continuous branch LODs.

Blender --background --factory-startup --python <this script>. All meshes are
original project geometry, not botanical scans. Existing S8 assets are untouched.
The envelope contracts deliberately match convenient one-metre HISM instances.
"""
import bpy
import hashlib
import json
import math
import random
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S9/CorridorNatureV1'
assert not (OUT/'Meshes').exists(), 'Preserve previously authored vegetation meshes'
assert not (OUT/'mesh-receipt.json').exists(), 'Preserve vegetation receipt'
assert not (OUT/'CorridorNatureV1.blend').exists(), 'Preserve vegetation project'
OUT.mkdir(parents=True,exist_ok=True)
MESHES = OUT / 'Meshes'
MESHES.mkdir()
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
MATERIALS = {}
for name, color in [('M_CorridorLeaf', (.085,.14,.035,1)),
                    ('M_CorridorDryGrass', (.22,.235,.085,1)),
                    ('M_CorridorBark', (.10,.075,.046,1))]:
    material = bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    shader = material.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = color
    shader.inputs['Roughness'].default_value = .86
    MATERIALS[name] = material

records = []


def export_mesh(family, name, lod, verts, faces, colors, uvs, envelope, material):
    # Every LOD shares the exact envelope, origin and base contract. This also
    # makes ground contact independent of screen size and static-mesh reduction.
    low = [min(v[j] for v in verts) for j in range(3)]
    high = [max(v[j] for v in verts) for j in range(3)]
    verts = [tuple(envelope[0][j] + (v[j]-low[j])/(high[j]-low[j]) *
                   (envelope[1][j]-envelope[0][j]) for j in range(3)) for v in verts]
    mesh = bpy.data.meshes.new(name+'_LOD'+str(lod))
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    mesh.materials.append(MATERIALS[material])
    uv = mesh.uv_layers.new(name='LeafAndBladeUV')
    attr = mesh.color_attributes.new(name='NatureVariation', type='FLOAT_COLOR', domain='CORNER')
    for polygon in mesh.polygons:
        for loop_index in polygon.loop_indices:
            vertex = mesh.loops[loop_index].vertex_index
            uv.data[loop_index].uv = uvs[vertex]
            attr.data[loop_index].color = colors[vertex]
    mesh.calc_loop_triangles()
    triangles = len(mesh.loop_triangles)
    obj = bpy.data.objects.new(mesh.name, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    # Coordinate conversion follows the project track import path.
    mesh.transform(Matrix.Diagonal((1,-1,1,1)))
    mesh.flip_normals()
    mesh.update()
    path = MESHES / (mesh.name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'MESH'}, axis_forward='X', axis_up='Z', use_triangles=True,
        apply_unit_scale=True, path_mode='STRIP', mesh_smooth_type='FACE',
        colors_type='LINEAR', add_leaf_bones=False)
    measured = [[min(v.co[j] for v in mesh.vertices) for j in range(3)],
                [max(v.co[j] for v in mesh.vertices) for j in range(3)]]
    assert max(abs(measured[i][j]-envelope[i][j]) for i in range(2) for j in range(3)) < 1e-6
    records.append(dict(family=family, name=name, lod=lod, triangles=triangles,
        vertices=len(verts), sections=1, boundsMetres=measured, material=material,
        fbx=str(path.relative_to(ROOT)), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        originalAuthoredLod=True, vertexColorPreserved=True))
    obj.hide_set(True)
    obj.select_set(False)


def leaves(lod):
    rng = random.Random(73213)
    count = (168, 110, 46)[lod]
    verts, faces, colors, uvs = [], [], [], []
    for k in range(count):
        # Stratified directions prevent bare quadrants. The envelope consists
        # of several overlapping shoots, with no flat, planar canopy sheet.
        phi = k * 2.3999632297
        z = 1 - 2 * (k+.5) / count
        radius = math.sqrt(max(0, 1-z*z))
        direction = Vector((radius*math.cos(phi), radius*math.sin(phi), z))
        p = direction * rng.uniform(.22,.40)
        p.z *= .75
        p += Vector((.025*math.sin(phi*2), .04*math.cos(phi*3), .018*math.sin(phi*5)))
        axis = Vector((math.cos(phi+rng.uniform(-.8,.8)),
                       math.sin(phi+rng.uniform(-.8,.8)), rng.uniform(-.25,.55))).normalized()
        side = axis.cross(Vector((0,0,1))).normalized()
        normal = axis.cross(side).normalized()
        length = rng.uniform((.075,.105,.13)[lod], (.125,.155,.19)[lod])
        width = length * rng.uniform(.34,.48)
        start = len(verts)
        tone = rng.uniform(.62,1.05)
        leaf_color = (tone*rng.uniform(.91,1.08), tone, tone*rng.uniform(.76,1.05), 1)
        # Tapered lanceolate silhouette; close foliage has a curved midrib.
        points = [p-axis*length*.5, p-side*width*.5,
                  p+axis*length*.5, p+side*width*.5]
        local_uv = [(0.5,0),(0,.45),(.5,1),(1,.45)]
        if lod == 0:
            points.append(p+normal*length*.10)
            local_uv.append((.5,.45))
            faces.extend(tuple(start+i for i in f) for f in [(0,1,4),(1,2,4),(2,3,4),(3,0,4)])
        else:
            faces.extend(tuple(start+i for i in f) for f in [(0,1,2),(0,2,3)])
        verts.extend(points)
        colors.extend([leaf_color]*len(points))
        uvs.extend(local_uv)
    export_mesh('foliage','SM_CorridorLeafClusterV1',lod,verts,faces,colors,uvs,
                [[-.5,-.5,-.375],[.5,.5,.375]],'M_CorridorLeaf')


def grass(lod):
    rng = random.Random(165779)
    count, segments = [(20,4),(11,3),(6,2)][lod]
    verts, faces, colors, uvs = [], [], [], []
    for k in range(count):
        angle = k*2.3999632297 + rng.uniform(-.3,.3)
        root_radius = rng.uniform(.025,.28)
        root = Vector((math.cos(angle)*root_radius,math.sin(angle)*root_radius,0))
        axis = Vector((math.cos(angle+.3),math.sin(angle+.3),0))
        side = Vector((-axis.y,axis.x,0))
        height, bend = rng.uniform(.36,.85), rng.uniform(.12,.37)
        width = rng.uniform(.014,.028)*(1+.25*lod)
        start = len(verts)
        tone = rng.uniform(.68,1.06)
        for step in range(segments):
            t = step/segments
            center = root + axis*bend*t*t + Vector((0,0,height*(t-.12*t*t)))
            half_width = width * (1-.72*t) * .5
            for s in (-1,1):
                verts.append(center+side*half_width*s)
                colors.append((tone*(.68+.36*t),tone*(.78+.15*t),tone*(.66+.18*t),1))
                uvs.append(((s+1)/2,t))
        verts.append(root+axis*bend+Vector((0,0,height*.88)))
        colors.append((tone*1.06,tone*.90,tone*.82,1))
        uvs.append((.5,1))
        for step in range(segments-1):
            a=start+step*2
            faces.extend([(a,a+1,a+3),(a,a+3,a+2)])
        faces.append((start+(segments-1)*2,start+(segments-1)*2+1,len(verts)-1))
    export_mesh('grass','SM_CorridorGrassPatchV1',lod,verts,faces,colors,uvs,
                [[-.5,-.5,0],[.5,.5,1]],'M_CorridorDryGrass')


def branch(lod):
    sides = (8,6,4)[lod]
    verts, faces, colors, uvs = [], [], [], []
    for ring in range(3):
        z = ring*.5-.5
        for k in range(sides):
            angle = k*math.tau/sides
            radius = .5*(1-.05*ring)*(.96+.04*math.sin(k*2.7))
            verts.append((math.cos(angle)*radius,math.sin(angle)*radius,z))
            shade=.64+.32*(.5+.5*math.sin(angle*2+.3))
            colors.append((shade,shade*.94,shade*.86,1))
            uvs.append((k/sides,ring*.5))
    for ring in range(2):
        for k in range(sides):
            a,b=ring*sides+k,ring*sides+(k+1)%sides
            faces.extend([(a,b,b+sides),(a,b+sides,a+sides)])
    for ring in (0,2):
        center=len(verts)
        verts.append((0,0,ring*.5-.5))
        colors.append((.7,.7,.7,1))
        uvs.append((.5,.5))
        for k in range(sides):
            edge=(ring*sides+k,ring*sides+(k+1)%sides)
            faces.append((center,*edge) if ring==2 else (center,*reversed(edge)))
    export_mesh('branch','SM_CorridorBranchV1',lod,verts,faces,colors,uvs,
                [[-.5,-.5,-.5],[.5,.5,.5]],'M_CorridorBark')


for level in range(3):
    leaves(level)
    grass(level)
    branch(level)
assert [r['triangles'] for r in records if r['family']=='foliage'] == [672,220,92]
assert [r['triangles'] for r in records if r['family']=='grass'] == [140,55,18]
assert [r['triangles'] for r in records if r['family']=='branch'] == [48,36,24]
(OUT/'mesh-receipt.json').write_text(json.dumps(dict(revision='corridor-nature-v1',
    originalProjectAuthoredGeometry=True, thirdPartyAssets=False, generatedImages=False,
    botanicalReplica=False, naniteRequired=False, transparentOverdraw=False,
    axes='X route forward, Y lateral, Z up; metres',
    contracts={'foliage':'one metre XY, 0.75 metre Z, centered origin',
               'grass':'one metre XYZ, root plane Z=0',
               'branch':'one metre XYZ, centered origin, long axis Z'},
    records=records),indent=2))
(OUT/'ATTRIBUTION.md').write_text('Original project-authored procedural geometry and vertex colours. '
    'No external pixels, AI images, commercial assets or scanned species are used. '
    'These are representative broadleaf clusters, verge grass and bark branches, '
    'not certified botanical replicas. All three LODs are authored explicitly; '
    'branch geometry remains present in every LOD. Existing S8 assets remain unchanged.\n')
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CorridorNatureV1.blend'))
print('CORRIDOR_NATURE_SOURCES_READY '+str(OUT))
