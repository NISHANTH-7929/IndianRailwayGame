"""Reference-led corrections to the existing cab; preserve working parts and UVs."""
import bpy, bmesh, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
EVIDENCE=ROOT/'07_Documentation/S8/Evidence/Stabilization-20261010'
SOURCE=ROOT/'04_Assets/S8/Cabs/WAP7'
BASE=EVIDENCE/'Before/WAP7'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'WAP7_Cab.blend'))
d=json.loads((BASE/'assembly.json').read_text())
core=bpy.data.objects['SM_WAP7_Cab_StaticCore']
glass=bpy.data.objects['SM_WAP7_Cab_Glazing']
for img in bpy.data.images:
    if img.source=='FILE':
        candidate=SOURCE/'Textures'/Path(bpy.path.abspath(img.filepath)).name
        if candidate.exists():img.filepath=str(candidate)

def remove_components(obj, predicate):
    bm=bmesh.new();bm.from_mesh(obj.data);seen=set();remove=[];count=0
    for vertex in bm.verts:
        if vertex in seen:continue
        todo=[vertex];component=[];seen.add(vertex)
        while todo:
            v=todo.pop();component.append(v)
            for edge in v.link_edges:
                other=edge.other_vert(v)
                if other not in seen:seen.add(other);todo.append(other)
        coords=[obj.matrix_world@v.co for v in component]
        if predicate(coords):remove.extend(component);count+=1
    bmesh.ops.delete(bm,geom=remove,context='VERTS');bm.to_mesh(obj.data);bm.free()
    return count

# Remove only the old long side-window hardware, not front screens or fan.
removed=remove_components(core,lambda p: all(abs(v.y)>1.34 and 7.25<v.x<9.67 and 2.86<v.z<3.96 for v in p))
removed_glass=remove_components(glass,lambda p: all(abs(v.y)>1.35 for v in p))
assert removed_glass==2, 'Expected exactly two oversized side panes'
extra=[];panes=[]

def plain_material(name):
    if name in bpy.data.materials:return bpy.data.materials[name]
    existing=next(m for m in d['materials'] if m['name'].startswith(name))
    # New wall infill must not sample another surface's baked contact UV atlas.
    row={k:existing[k] for k in ('base_color','roughness','metallic')};row['name']=name
    m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=row['base_color'];p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Metallic'].default_value=row['metallic']
    if not any(r['name']==name for r in d['materials']):d['materials'].append(row)
    return m

def box(name,loc,size,mat='CabIvory',bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        m=o.modifiers.new('Folded edge','BEVEL');m.width=bevel;m.segments=1;bpy.ops.object.modifier_apply(modifier=m.name)
    o.data.materials.append(plain_material(mat));extra.append(o)
    return o

# Two discrete openings per side: rear access-door window plus driver side pane.
# Dimensions inferred from photographs, not a certified fleet drawing.
for sign in (-1,1):
    y=sign*1.405
    intervals=[(7.28,7.49),(8.03,8.36),(9.20,9.65)]
    for a,b in intervals:box('Solid side lining',((a+b)/2,y,3.42),(b-a,.065,1.06))
    for a,b,bottom,top in [(7.49,8.03,3.12,3.69),(8.36,9.20,3.06,3.73)]:
        box('Side window sill',((a+b)/2,y,(2.89+bottom)/2),(b-a,.065,bottom-2.89))
        box('Side window header',((a+b)/2,y,(top+3.95)/2),(b-a,.065,3.95-top))
        for x in (a,b):box('Side window gasket',(x,y-sign*.037,(bottom+top)/2),(.019,.016,top-bottom),'Rubber',.004)
        for z in (bottom,top):box('Side window gasket',((a+b)/2,y-sign*.037,z),(b-a,.016,.019),'Rubber',.004)
        mesh=bpy.data.meshes.new('Bounded side glass');mesh.from_pydata([(a,y,bottom),(b,y,bottom),(b,y,top),(a,y,top)],[],[(0,1,2,3)]);mesh.update()
        o=bpy.data.objects.new('Bounded side glass',mesh);bpy.context.collection.objects.link(o);o.data.materials.append(bpy.data.materials['WindowGlass']);panes.append(o)
    box('Sliding pane divider',(8.78,y-sign*.035,3.395),(.014,.015,.67),'Steel',.002)
    for x in (7.36,8.16):box('Door seam',(x,y-sign*.039,2.79),(.007,.007,1.94),'Rubber',.001)
    box('Door handle',(8.10,y-sign*.065,2.77),(.026,.032,.15),'Steel',.004)

def join(target,objects):
    bpy.ops.object.select_all(action='DESELECT')
    for o in [target]+objects:o.select_set(True)
    bpy.context.view_layer.objects.active=target;bpy.ops.object.join()
join(core,extra);join(glass,panes)

# Preserve the authored bank and controls: a bounds-based tilt splits connected
# glyphs and panels. Readability is improved through contrast and reversible zoom.
changed={'StaticCore','Glazing'}

for row in d['materials']:
    if row['name'].startswith('PanelCharcoal'):
        row['base_color'][:3]=[.024,.028,.027]
        m=bpy.data.materials.get(row['name'])
        if m:
            p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*row['base_color'][:3],1)
            for n in m.node_tree.nodes:
                if n.type=='MIX_RGB' and n.blend_type=='MULTIPLY':n.inputs[2].default_value=(*row['base_color'][:3],1)

for row in d['parts']:
    if row['name'] not in changed:continue
    o=bpy.data.objects['SM_WAP7_Cab_'+row['name']];bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    saved=o.location.copy();o.location=(0,0,0)
    bpy.ops.export_scene.fbx(filepath=str(SOURCE/row['fbx']),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,mesh_smooth_type='FACE',path_mode='STRIP')
    o.location=saved;o.data.calc_loop_triangles();row['triangles']=len(o.data.loop_triangles)
d['triangles']=sum(r['triangles'] for r in d['parts'])
d['presentation_revision']='cab-stabilization-20261010'
d['limitations'].append('Interior side windows adjusted to traditional WAP7 photo proportions; dimensions remain inferred, not fleet-certified. Existing instrument bank geometry retained.')
(SOURCE/'assembly.json').write_text(json.dumps(d,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'WAP7_Cab.blend'))
(EVIDENCE/'cab-source-change.json').write_text(json.dumps(dict(changed_parts=sorted(changed),old_side_components_removed=removed,old_side_panes_removed=removed_glass,new_side_panes=4,triangles=d['triangles'],reference='IRFCA IMG_0092 and Wikimedia side-view WAP7 photographs',simulation_changed=False),indent=2))
