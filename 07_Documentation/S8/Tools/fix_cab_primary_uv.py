import bpy,json,array
from pathlib import Path
root=Path(__file__).resolve().parents[3];out=root/'04_Assets/S8/Cabs/WAP7'
bpy.ops.wm.open_mainfile(filepath=str(out/'WAP7_Cab.blend'));o=bpy.data.objects['SM_WAP7_Cab_StaticCore'];u0,u1=o.data.uv_layers[:2]
if u0.name == 'CabContactUV' and u1.name == 'DetailUV':
 print('CAB_PRIMARY_UV_ALREADY_FIXED');raise SystemExit(0)
if u1.name != 'CabContactUV':
 raise RuntimeError('Expected contact atlas in secondary UV; refusing an unknown UV layout')
a=array.array('f',[0.0])*(len(u0.data)*2);b=array.array('f',[0.0])*(len(u1.data)*2);u0.data.foreach_get('uv',a);u1.data.foreach_get('uv',b);u0.data.foreach_set('uv',b);u1.data.foreach_set('uv',a);u0.name='TemporaryUV';u1.name='DetailUV';u0.name='CabContactUV';o.data.uv_layers.active_index=0;u0.active_render=True
for m in o.data.materials:
 for node in list(m.node_tree.nodes):
  if node.type=='TEX_IMAGE' and node.image and node.image.name!='CabContact':
   uv=m.node_tree.nodes.new('ShaderNodeUVMap');uv.uv_map='DetailUV';m.node_tree.links.new(uv.outputs['UV'],node.inputs['Vector'])
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.export_scene.fbx(filepath=str(out/'Parts/SM_WAP7_Cab_StaticCore.fbx'),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,use_triangles=True,mesh_smooth_type='FACE',path_mode='STRIP')
d=json.loads((out/'assembly.json').read_text())
for m in d['materials']:
 if m.get('ao_texture'):m['ao_uv']=0;m['detail_uv']=1
(out/'assembly.json').write_text(json.dumps(d,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(out/'WAP7_Cab.blend'));print('CAB_PRIMARY_UV_FIXED')
