"""Preserve rail sections at distance; bevel close sleepers without moving gauge."""
import bpy
import hashlib
import json
from pathlib import Path
from mathutils import Matrix

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'04_Assets/S9/TrackEnvironmentV1/Rails'
assert not OUT.exists(), 'Preserve previous source revision'
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1.
materials=[]
for name in ('RailHead','RailRust','Concrete'):
    materials.append(bpy.data.materials.new(name))
records=[]
for lod in range(3):
    pieces=[]
    def box(center,size,slot,bevel=False,open_bottom=False):
        bpy.ops.mesh.primitive_cube_add(size=1,location=center)
        o=bpy.context.object;o.dimensions=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        o.data.materials.append(materials[slot])
        if bevel:
            mod=o.modifiers.new('Worn concrete edges','BEVEL');mod.width=.018;mod.segments=1
            bpy.ops.object.modifier_apply(modifier=mod.name)
        if open_bottom:
            import bmesh
            bm=bmesh.new();bm.from_mesh(o.data)
            # Far sleepers retain top and long side silhouettes; end/bottom
            # faces are hidden at this range. Rails remain closed at every LOD.
            faces=[f for f in bm.faces if f.normal.z<-.5 or abs(f.normal.y)>.5]
            bmesh.ops.delete(bm,geom=faces,context='FACES');bm.to_mesh(o.data);bm.free()
        pieces.append(o)
    for y in (-.873,.873):
        box((3,y,-.0225),(6,.07,.045),0)
        box((3,y,-.09),(6,.016,.09),1)
        box((3,y,-.151),(6,.15,.032),1)
    for i in range(10):
        x=i*.6+.3
        box((x,0,-.25),(.24,2.75,.17),2,bevel=lod==0,open_bottom=lod==2)
        if lod==0:
            for y in (-.98,-.76,.76,.98):box((x,y,-.158),(.13,.07,.035),1)
    bpy.ops.object.select_all(action='DESELECT')
    for o in pieces:o.select_set(True)
    bpy.context.view_layer.objects.active=pieces[0]
    bpy.ops.object.join();mesh=bpy.context.object
    mesh.name='SM_Track6m_RealismV1_LOD'+str(lod)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    uv=mesh.data.uv_layers.new(name='MetricUV')
    for p in mesh.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(p.normal[j]))]
        for index in p.loop_indices:
            v=mesh.data.vertices[mesh.data.loops[index].vertex_index].co
            uv.data[index].uv=(v[axes[0]],v[axes[1]])
    mesh.data.calc_loop_triangles()
    count=len(mesh.data.loop_triangles)
    mesh.data.transform(Matrix.Diagonal((1,-1,1,1)));mesh.data.flip_normals();mesh.data.update()
    path=OUT/(mesh.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},
        axis_forward='X',axis_up='Z',use_triangles=True,apply_unit_scale=True,
        path_mode='STRIP',add_leaf_bones=False,mesh_smooth_type='FACE')
    records.append(dict(lod=lod,triangles=count,fbx=str(path.relative_to(ROOT)),
        sha256=hashlib.sha256(path.read_bytes()).hexdigest(),railHeadWebFootPreserved=True))
    mesh.hide_set(True);mesh.select_set(False)
assert [r['triangles'] for r in records]==[992,192,132],records
(OUT/'rails-receipt.json').write_text(json.dumps(dict(records=records,gaugeInsideM=1.676,
    railCentresM=[-.873,.873],railTopM=0,sleeperSpacingM=.6,lengthM=6,
    estimatedPresentation=True,originalS8AssetsModified=False),indent=2))
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'TrackRailsV1.blend'))
