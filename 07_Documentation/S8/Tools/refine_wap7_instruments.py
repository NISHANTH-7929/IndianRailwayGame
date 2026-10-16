"""Refine the preserved, UV-repaired cab without regenerating its contact atlas.

Run in Blender. Original source is read from the immutable before snapshot;
outputs replace only the WAP7 cab source and receive a new evidence receipt.
No new physics, simulated pressure, measured fleet-variant or OEM face claim.
"""
import bpy, json, math, hashlib
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[3]
EVIDENCE=ROOT/'07_Documentation/S8/Evidence/Cab/InstrumentRefinement-20261007'
BASE=EVIDENCE/'Before/WAP7'
OUT=ROOT/'04_Assets/S8/Cabs/WAP7'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'WAP7_Cab.blend'))
d=json.loads((BASE/'assembly.json').read_text())
core=bpy.data.objects['SM_WAP7_Cab_StaticCore']
assert [u.name for u in core.data.uv_layers][:2]==['CabContactUV','DetailUV']
uv_before=hashlib.sha256(bytes(str([(tuple(v.uv)) for v in core.data.uv_layers[0].data]),'utf8')).hexdigest()
# Resolve image files against the original live cab so the new blend is portable
# within its existing source folder, not coupled to the evidence snapshot.
for img in bpy.data.images:
    if img.source=='FILE':
        candidate=OUT/'Textures'/Path(bpy.path.abspath(img.filepath)).name
        if candidate.exists():img.filepath=str(candidate)

def material(name,color,rough,metal=0,alpha=1,emission=None,strength=.16):
    m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,alpha)
    p=m.node_tree.nodes.get('Principled BSDF')
    for key,value in [('Base Color',(*color,1)),('Roughness',rough),('Metallic',metal),('Alpha',alpha)]:p.inputs[key].default_value=value
    row=dict(name=name,base_color=[*color,alpha],roughness=rough,metallic=metal)
    if emission:
        p.inputs['Emission Color'].default_value=(*emission,1);p.inputs['Emission Strength'].default_value=strength
        row.update(instrument_emission=emission,instrument_emission_strength=strength)
    d['materials'].append(row)
    return m

mark=material('InstrumentMarkings',(.71,.77,.66),.48,emission=[.55,.72,.39],strength=.20)
paper=material('InstrumentPaper',(.49,.52,.43),.53,emission=[.49,.59,.32],strength=.12)
ink=material('InstrumentInk',(.008,.011,.008),.57)
pointer=material('InstrumentPointer',(.63,.07,.025),.4,emission=[.62,.10,.025],strength=.20)
rim=material('InstrumentBezel',(.19,.205,.19),.27,.8)
glass=material('InstrumentGlass',(.23,.29,.26),.14,alpha=.075)
gauges=[(1.23,3.09,.064,True),(1.035,3.09,.064,True),(1.23,2.90,.064,False),(1.035,2.90,.064,True),(.12,3.58,.077,False)]
for m in [mark,paper,ink]:core.data.materials.append(m)
slots={m.name:i for i,m in enumerate(core.data.materials)}
reassigned=0
for poly in core.data.polygons:
    c=core.matrix_world@poly.center
    if not 9.025<c.x<9.24:continue
    for y,z,r,dark in gauges:
        if (c.y-y)**2+(c.z-z)**2>(r*.97)**2:continue
        old=core.data.materials[poly.material_index].name
        selected=mark if old.startswith('Lettering') else paper if old.startswith('DialIvory') else ink if old.startswith('PanelCharcoal') else None
        if selected:
            poly.material_index=slots[selected.name];reassigned+=1
        break
needle=bpy.data.objects['SM_WAP7_Cab_gauge_speed']
for i,m in enumerate(needle.data.materials):
    if m.name=='Red':needle.data.materials[i]=pointer

def bank_warp(v):
    if 9.015<v.x<9.20 and 2.77<v.z<3.235:
        v.x+=(v.z-2.78)*.24
        if v.y>.91:v.x-=.10*(v.y-.91)
        elif v.y<-.04:v.x+=.10*(v.y+.04)
    return v

rings=[];panes=[]
for y,z,r,dark in gauges:
    # Rolled rim and a clear front pane enclose the existing needle and face.
    bpy.ops.mesh.primitive_torus_add(major_radius=r*1.015,minor_radius=.0025,major_segments=48,minor_segments=6,location=(9.105-.032,y,z),rotation=(0,math.pi/2,0))
    ob=bpy.context.object;ob.name='Instrument rolled bezel';ob.data.materials.append(rim)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    for v in ob.data.vertices:v.co=bank_warp(v.co)
    for p in ob.data.polygons:p.use_smooth=True
    rings.append(ob)
    verts=[bank_warp(Vector((9.105-.040,y+math.cos(i*math.tau/48)*r*.985,z+math.sin(i*math.tau/48)*r*.985))) for i in range(48)]
    mesh=bpy.data.meshes.new('Clear instrument lens');mesh.from_pydata(verts,[],[tuple(reversed(range(48)))]);mesh.update()
    ob=bpy.data.objects.new('Clear instrument lens',mesh);bpy.context.collection.objects.link(ob);ob.data.materials.append(glass);panes.append(ob)

def join_into(target,extra):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in [target]+extra:ob.select_set(True)
    bpy.context.view_layer.objects.active=target;bpy.ops.object.join()
join_into(core,rings)
join_into(bpy.data.objects['SM_WAP7_Cab_Glazing'],panes)
assert [u.name for u in core.data.uv_layers][:2]==['CabContactUV','DetailUV']
original_loops=len(bpy.data.objects['SM_WAP7_Cab_StaticCore'].data.uv_layers[0].data)-5*48*6*4
uv_after=hashlib.sha256(bytes(str([tuple(v.uv) for v in core.data.uv_layers[0].data[:original_loops]]),'utf8')).hexdigest()
assert uv_before==uv_after, 'Existing contact UV coordinates changed'
changed={'StaticCore','Glazing','gauge_speed'}
for row in d['parts']:
    if row['name'] not in changed:continue
    ob=bpy.data.objects['SM_WAP7_Cab_'+row['name']]
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    saved=ob.location.copy();ob.location=(0,0,0)
    bpy.ops.export_scene.fbx(filepath=str(OUT/row['fbx']),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,mesh_smooth_type='FACE',path_mode='STRIP')
    ob.location=saved;ob.data.calc_loop_triangles();row['triangles']=len(ob.data.loop_triangles)
d['triangles']=sum(r['triangles'] for r in d['parts'])
d['presentation_revision']='cab-instruments-20261007'
d['driver_camera']['fov']=95  # Matches the existing runtime clamp; viewpoint unchanged.
d['limitations'].append('Instrument markings use a restrained always-powered presentation backlight, independent of the overhead cab lamp; no electrical supply circuit or exact prototype illumination is simulated.')
(OUT/'assembly.json').write_text(json.dumps(d,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_Cab.blend'))
receipt=dict(triangles=d['triangles'],parts=len(d['parts']),reassigned_polygons=reassigned,contact_uv_preserved=uv_before==uv_after,contact_uv_sha256=uv_before,changed_parts=sorted(changed),simulation_changed=False,source_hashes={str(p.relative_to(OUT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [OUT/'WAP7_Cab.blend',OUT/'assembly.json']+[OUT/r['fbx'] for r in d['parts']]})
(EVIDENCE/'source-validation.json').write_text(json.dumps(receipt,indent=2))
print('CAB_INSTRUMENT_REFINEMENT',json.dumps({k:v for k,v in receipt.items() if k!='source_hashes'}))
