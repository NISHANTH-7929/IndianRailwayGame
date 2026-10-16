"""Bake restrained static contact shading and consolidate fixed cab geometry.
Run after build_wap7_cab.py; moving controls retain their own materials and pivots.
"""
import bpy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'04_Assets/S8/Cabs/WAP7'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'WAP7_Cab.blend'))
d=json.loads((OUT/'assembly.json').read_text())
static=[r for r in d['parts'] if r['role']=='cab_static' and r['name']!='Glazing']
bpy.ops.object.select_all(action='DESELECT')
objects=[bpy.data.objects['SM_WAP7_Cab_'+r['name']] for r in static]
for o in bpy.context.scene.objects:o.hide_render=o not in objects
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();ob=bpy.context.object;ob.name='SM_WAP7_Cab_StaticCore'
# Existing UV0 remains the detail texture channel; channel 1 is unique contact shading.
while len(ob.data.uv_layers)>1:ob.data.uv_layers.remove(ob.data.uv_layers[-1])
uv=ob.data.uv_layers.new(name='CabContactUV');ob.data.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.004);bpy.ops.object.mode_set(mode='OBJECT')
image=bpy.data.images.new('CabContact',width=2048,height=2048,alpha=False);image.colorspace_settings.name='Non-Color'
original=list(ob.data.materials)
for i,m in enumerate(original):
 temp=bpy.data.materials.new('BakeContact_'+str(i));temp.use_nodes=True;n=temp.node_tree.nodes;n.clear();links=temp.node_tree.links
 ao=n.new('ShaderNodeAmbientOcclusion');ao.inputs['Distance'].default_value=.22;ao.samples=16
 em=n.new('ShaderNodeEmission');links.new(ao.outputs['Color'],em.inputs['Color']);out=n.new('ShaderNodeOutputMaterial');links.new(em.outputs[0],out.inputs['Surface'])
 tex=n.new('ShaderNodeTexImage');tex.image=image;n.active=tex;ob.data.materials[i]=temp
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=1;scene.render.bake.use_selected_to_active=False
bpy.ops.object.bake(type='EMIT',margin=6,use_clear=True)
image.filepath_raw=str(OUT/'Textures/CabContact.png');image.file_format='PNG';image.save()
defs={x['name']:x for x in d['materials']};newdefs=[]
for i,m in enumerate(original):
 mat=m.copy();mat.name=m.name+'_StaticAO';p=mat.node_tree.nodes.get('Principled BSDF');nodes=mat.node_tree.nodes;links=mat.node_tree.links
 tex=nodes.new('ShaderNodeTexImage');tex.image=image;coord=nodes.new('ShaderNodeUVMap');coord.uv_map='CabContactUV';links.new(coord.outputs['UV'],tex.inputs['Vector'])
 factor=nodes.new('ShaderNodeMath');factor.operation='MULTIPLY_ADD';factor.inputs[1].default_value=.45;factor.inputs[2].default_value=.55;links.new(tex.outputs['Color'],factor.inputs[0])
 mix=nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1
 old=p.inputs['Base Color'].links[0].from_socket if p.inputs['Base Color'].is_linked else None
 if old:links.new(old,mix.inputs[1])
 else:mix.inputs[1].default_value=p.inputs['Base Color'].default_value
 links.new(factor.outputs[0],mix.inputs[2]);links.new(mix.outputs[0],p.inputs['Base Color']);ob.data.materials[i]=mat
 row=dict(defs[m.name]);row.update(name=mat.name,ao_texture='Textures/CabContact.png',ao_uv=1);newdefs.append(row)
ob.data.uv_layers.active_index=0
for o in bpy.context.scene.objects:o.hide_render=False
bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
bpy.ops.export_scene.fbx(filepath=str(OUT/'Parts/SM_WAP7_Cab_StaticCore.fbx'),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,mesh_smooth_type='FACE',path_mode='STRIP')
ob.data.calc_loop_triangles();row=dict(static[0]);row.update(name='StaticCore',fbx='Parts/SM_WAP7_Cab_StaticCore.fbx',triangles=len(ob.data.loop_triangles))
d['parts']=[r for r in d['parts'] if r not in static]+[row];d['materials']+=newdefs;d['triangles']=sum(r['triangles'] for r in d['parts'])
(OUT/'assembly.json').write_text(json.dumps(d,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_Cab.blend'))
print('CONTACT_BAKE_COMPLETE',d['triangles'],len(d['parts']))
