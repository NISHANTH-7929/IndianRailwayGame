"""Dense, all-angle canopy sprays; V1 sources remain unchanged.

V1's nearly horizontal leaves presented their edges to ground-level cameras.
This derivative varies leaf-plane inclinations and fills the inner crown using
small pointed leaves. All LODs retain the original unit envelope contract.
"""
import bpy
import hashlib
import json
import math
import random
from pathlib import Path
from mathutils import Matrix, Quaternion, Vector

ROOT=Path(__file__).resolve().parents[3]
BASE=ROOT/'04_Assets/S9/CorridorNatureV1'
OUT=BASE/'MeshesV2'
assert not OUT.exists(), 'Preserve previous dense canopy revision'
OUT.mkdir()
PROTECTED=[BASE/'mesh-receipt.json',BASE/'CorridorNatureV1.blend']
PROTECTED+=list((BASE/'Meshes').glob('*.fbx'))
hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in PROTECTED}
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1.0
material=bpy.data.materials.new('M_CorridorLeaf')
material.diffuse_color=(.085,.14,.035,1)
records=[]

for lod in range(3):
    rng=random.Random(829137)
    count=(320,120,48)[lod]
    verts,faces,colors,uvs=[],[],[],[]
    inclinations=[]
    for k in range(count):
        phi=k*2.3999632297
        z=1-2*(k+.5)/count
        radial=math.sqrt(max(0,1-z*z))
        # Two thirds of leaves form the irregular outer volume; the remaining
        # third fills the hollow core without introducing opaque canopy shells.
        radius=rng.uniform(.13,.29) if k%3==0 else rng.uniform(.28,.42)
        p=Vector((radial*math.cos(phi),radial*math.sin(phi),z*.78))*radius
        p+=Vector((.023*math.sin(phi*2),.031*math.cos(phi*3),.018*math.sin(phi*5)))
        a=phi+rng.uniform(-1.2,1.2)
        axis=Vector((math.cos(a),math.sin(a),rng.uniform(-.35,.35))).normalized()
        side=axis.cross(Vector((0,0,1))).normalized()
        # Deliberately distribute visible leaf faces through all side angles.
        # Keep each individual leaf small; density comes from overlap, not cards.
        tilt=math.radians((18,42,66,82)[k%4]+rng.uniform(-8,8))*(-1 if k%2 else 1)
        side=Quaternion(axis,tilt)@side
        normal=axis.cross(side).normalized()
        inclinations.append(math.degrees(math.acos(min(1,abs(normal.z)))))
        length=rng.uniform((.105,.145,.19)[lod],(.155,.215,.275)[lod])
        width=length*rng.uniform(.48,.63)
        start=len(verts)
        points=[p-axis*length*.5,p-side*width*.5,p+axis*length*.5,p+side*width*.5]
        local_uv=[(.5,0),(0,.46),(.5,1),(1,.46)]
        if lod==0 and k%27==0:
            points.append(p+normal*length*.08)
            local_uv.append((.5,.46))
            faces.extend(tuple(start+i for i in f) for f in [(0,1,4),(1,2,4),(2,3,4),(3,0,4)])
        else:
            faces.extend(tuple(start+i for i in f) for f in [(0,1,2),(0,2,3)])
        tone=rng.uniform(.67,1.08)
        color=(tone*rng.uniform(.94,1.07),tone,tone*rng.uniform(.82,1.04),1)
        verts.extend(points)
        colors.extend([color]*len(points))
        uvs.extend(local_uv)
    envelope=[[-.5,-.5,-.375],[.5,.5,.375]]
    low=[min(v[j] for v in verts) for j in range(3)]
    high=[max(v[j] for v in verts) for j in range(3)]
    verts=[tuple(envelope[0][j]+(v[j]-low[j])/(high[j]-low[j])*
                 (envelope[1][j]-envelope[0][j]) for j in range(3)) for v in verts]
    name='SM_CorridorLeafClusterV2_LOD'+str(lod)
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    mesh.materials.append(material)
    uv=mesh.uv_layers.new(name='LeafAndBladeUV')
    attr=mesh.color_attributes.new(name='NatureVariation',type='FLOAT_COLOR',domain='CORNER')
    for polygon in mesh.polygons:
        for loop in polygon.loop_indices:
            vertex=mesh.loops[loop].vertex_index
            uv.data[loop].uv=uvs[vertex]
            attr.data[loop].color=colors[vertex]
    mesh.calc_loop_triangles()
    count_triangles=len(mesh.loop_triangles)
    assert count_triangles==(664,240,96)[lod]
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    mesh.transform(Matrix.Diagonal((1,-1,1,1)))
    mesh.flip_normals()
    mesh.update()
    path=OUT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},
        axis_forward='X',axis_up='Z',use_triangles=True,apply_unit_scale=True,
        path_mode='STRIP',mesh_smooth_type='FACE',colors_type='LINEAR',add_leaf_bones=False)
    measured=[[min(v.co[j] for v in mesh.vertices) for j in range(3)],
              [max(v.co[j] for v in mesh.vertices) for j in range(3)]]
    assert max(abs(measured[i][j]-envelope[i][j]) for i in range(2) for j in range(3))<1e-6
    records.append(dict(lod=lod,leaves=count,triangles=count_triangles,sections=1,
        boundsMetres=measured,fbx=str(path.relative_to(ROOT)),
        sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        minPlaneInclinationDegrees=min(inclinations),maxPlaneInclinationDegrees=max(inclinations),
        leavesWithPlaneTiltOver40Degrees=sum(a>40 for a in inclinations),
        opaqueShell=False,originalAuthoredLod=True))
    obj.select_set(False)
    obj.hide_set(True)
for path,expected in hashes.items():
    assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==expected
(OUT/'mesh-receipt.json').write_text(json.dumps(dict(revision='corridor-nature-v2',
    originalProjectAuthoredGeometry=True,generatedImages=False,botanicalReplica=False,
    explanation='Small leaf planes have varied inclinations and a populated inner crown; no giant leaf sheets or solid shells.',
    records=records,protectedV1Sha256=hashes),indent=2))
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CorridorLeafClusterV2.blend'))
print('CORRIDOR_CANOPY_V2_READY '+str(OUT))
