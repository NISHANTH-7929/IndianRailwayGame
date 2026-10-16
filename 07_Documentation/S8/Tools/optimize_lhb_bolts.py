"""Measure bolt simplification error in world units on the isolated cleanup study.

Ratios are experiments, never final Android budgets. Source is retained untouched.
"""
import bpy, json
from pathlib import Path
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[3]
BASE=ROOT/'04_Assets/S8/Acquired/LHB_Sketchfab_c6c3d431'
OUT=BASE/'Review'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LHB_CleanupStudy.blend'),load_ui=False,use_scripts=False)
report={'status':'unaccepted geometric optimization experiment','error_limit_metres':.0015,
        'measurement':'bidirectional sampled vertex-to-surface distances; at most 2048 vertices per direction, not a certified Hausdorff bound',
        'rationale':'1.5 mm is below one pixel at 3 m for a 1080 px, 75 degree horizontal field of view; review closer views separately',
        'objects':[]}
def points(mesh,matrix):return [matrix@v.co for v in mesh.vertices]
def tree(mesh,matrix):
    return BVHTree.FromPolygons(points(mesh,matrix),[list(p.vertices) for p in mesh.polygons],all_triangles=False)
def error(vertices,target):
    step=max(1,len(vertices)//2048)
    return max((target.find_nearest(p)[3] for p in vertices[::step]),default=0)
for obj in list(bpy.context.scene.objects):
    if obj.type!='MESH' or not obj.name.startswith('Bolt'):continue
    source=obj.data
    source.calc_loop_triangles()
    original_tree=tree(source,obj.matrix_world)
    original_points=points(source,obj.matrix_world)
    trials=[]
    for ratio in [.01,.025,.05,.1,.2,.4,.7,1.0]:
        candidate=bpy.data.objects.new('BoltTrial',source.copy())
        bpy.context.collection.objects.link(candidate)
        candidate.matrix_world=obj.matrix_world.copy()
        bpy.context.view_layer.objects.active=candidate
        modifier=candidate.modifiers.new('Measured reduction','DECIMATE')
        modifier.ratio=ratio
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        candidate.data.calc_loop_triangles()
        deviation=max(error(original_points,tree(candidate.data,candidate.matrix_world)),error(points(candidate.data,candidate.matrix_world),original_tree))
        trials.append({'ratio':ratio,'triangles':len(candidate.data.loop_triangles),'sampled_error_metres':deviation})
        if deviation<=report['error_limit_metres']:
            obj.data=candidate.data
            bpy.data.objects.remove(candidate,do_unlink=True)
            break
        mesh=candidate.data
        bpy.data.objects.remove(candidate,do_unlink=True)
        bpy.data.meshes.remove(mesh)
    report['objects'].append({'name':obj.name,'original_triangles':len(source.loop_triangles),'trials':trials})
total=0
for obj in bpy.context.scene.objects:
    if obj.type=='MESH':obj.data.calc_loop_triangles();total+=len(obj.data.loop_triangles)
report['total_triangles']=total
(OUT/'bolt-optimization.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LHB_OptimizedStudy.blend'))
print('LHB_OPTIMIZED_STUDY',total)
