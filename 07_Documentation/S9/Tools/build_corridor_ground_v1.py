"""Author one bounded median ground strip, preserving every existing asset.

Blender --background --factory-startup --python <this file>. This is original
representative scenery geometry, not a measured terrain/elevation model.
"""
import bpy
import hashlib
import json
import math
from pathlib import Path
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S9/CorridorNatureV1/Ground'
assert not OUT.exists() or not any(OUT.iterdir()), 'Keep previous ground authoring receipt'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
material = bpy.data.materials.new('M_CorridorNatureGround')
material.diffuse_color = (.13, .105, .065, 1)
records = []


def smooth(a, b, value):
    t = min(1., max(0., (value-a)/(b-a)))
    return t*t*(3.-2.*t)


def ground(x, y):
    # Longitudinal ends and outer metre stay flat. All LOD boundaries agree,
    # allowing ordinary HISM streaming and mixed LODs without edge cracks.
    fade = smooth(0., 1., min(x, 12.-x)) * (1.-smooth(2.6, 3.5, abs(y)))
    path_y = -1.3 + .18*math.sin(x*math.tau/12.)
    drain_y = 1.95 + .14*math.sin(x*math.tau/12.+.7)
    path = 1.-smooth(.30, .78, abs(y-path_y))
    damp = 1.-smooth(.14, .68, abs(y-drain_y))
    mound = .095*math.sin(x*.79+y*1.6) + .045*math.cos(x*1.37-y*2.4)
    height = fade * max(-.24, (mound*(1.-path*.8)) - .045*path - .17*damp)
    vegetation = max(0., (1.-path)*(1.-damp*.65))
    return height, (path, damp, vegetation, 1.)


for lod, nx, ny in ((0,24,36), (1,12,18), (2,6,12)):
    vertices = []
    colors = []
    for ix in range(nx+1):
        for iy in range(ny+1):
            x, y = 12.*ix/nx, -4.5+9.*iy/ny
            z, color = ground(x, y)
            vertices.append((x,y,z))
            colors.append(color)
    faces = []
    for ix in range(nx):
        for iy in range(ny):
            a = ix*(ny+1)+iy
            faces.append((a,a+ny+1,a+ny+2,a+1))
    # A 30cm buried skirt prevents hairline sky gaps on rail-alignment curves.
    # Never protrudes above the common Z0 edge; no box-like raised top slab.
    boundary = ([i*(ny+1) for i in range(nx+1)] +
                [nx*(ny+1)+j for j in range(1,ny+1)] +
                [i*(ny+1)+ny for i in range(nx-1,-1,-1)] +
                [j for j in range(ny-1,0,-1)])
    lower = []
    for top in boundary:
        lower.append(len(vertices))
        x,y,z = vertices[top]
        vertices.append((x,y,-.3));colors.append(colors[top])
    top_face_count = len(faces)
    for i,a in enumerate(boundary):
        j=(i+1)%len(boundary)
        faces.append((a,lower[i],lower[j],boundary[j]))
    mesh = bpy.data.meshes.new('MedianGroundLOD%d' % lod)
    mesh.from_pydata(vertices, [], faces);mesh.update()
    uv=mesh.uv_layers.new(name='MetricXY')
    color=mesh.color_attributes.new(name='GroundMasks', type='FLOAT_COLOR', domain='CORNER')
    for polygon in mesh.polygons:
        polygon.use_smooth = polygon.index < top_face_count
        for loop in polygon.loop_indices:
            index=mesh.loops[loop].vertex_index
            uv.data[loop].uv = vertices[index][:2]
            color.data[loop].color = colors[index]
    mesh.materials.append(material)
    obj=bpy.data.objects.new('SM_CorridorVerge12m_LOD%d' % lod,mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    mesh.calc_loop_triangles()
    triangles=len(mesh.loop_triangles)
    bounds=[[min(v[i] for v in vertices) for i in range(3)],
            [max(v[i] for v in vertices) for i in range(3)]]
    assert triangles < 2200 and bounds[0][2] == -.3 and bounds[1][2] < .15
    assert all(abs(ground(x,y)[0]) < 1e-7 for x,y in ((0,1),(12,1),(3,-4.5),(8,4.5)))
    original=obj.data;obj.data=original.copy()
    obj.data.transform(Matrix.Diagonal((1,-1,1,1)));obj.data.flip_normals();obj.data.update()
    target=OUT/(obj.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,
        object_types={'MESH'},axis_forward='X',axis_up='Z',use_triangles=True,
        apply_unit_scale=True,path_mode='STRIP',add_leaf_bones=False,
        mesh_smooth_type='FACE')
    temporary=obj.data;obj.data=original;bpy.data.meshes.remove(temporary)
    records.append(dict(lod=lod,triangles=triangles,fbx=str(target.relative_to(ROOT)),
        sha256=hashlib.sha256(target.read_bytes()).hexdigest(),bounds_m=bounds,
        metric_uv=True,vertex_color_masks='R worn path, G damp drainage, B vegetation',
        seam_height_m=0.,collision=False))

bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CorridorGroundV1.blend'))
receipt=dict(revision='corridor-nature-ground-v1',records=records,units='metres',
    geometry_contract=dict(length_m=12,width_m=9,root_at='X0 Y0 Z0',
        ground_reference_m=-.65,skirt_depth_m=.3,collision=False),
    height_samples=[dict(x=x,y=y,height=ground(x,y)[0])
        for x in (0.,1.,3.,6.,9.,11.,12.) for y in (-4.5,-2.,-1.3,0.,1.95,3.,4.5)],
    provenance='Original locally authored procedural scenery; no third-party pixels or geometry; representative, not surveyed.')
(OUT/'mesh-receipt.json').write_text(json.dumps(receipt,indent=2))
print('CORRIDOR_GROUND_AUTHORED '+str(OUT/'mesh-receipt.json'))
