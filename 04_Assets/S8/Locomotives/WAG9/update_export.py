from pathlib import Path
p=Path(__file__).with_name('export_parts.py')
s=p.read_text()
s=s.replace("p.inputs['Alpha'].default_value=.23", "p.inputs['Alpha'].default_value=.14")
s=s.replace("elif n.startswith(('Panto lower','Panto upper','Contact bow','Carbon strip')):add('Pantograph_'+side,'pantograph',o,(bx,0,4.5),{'animation_note':'Presentation vertical retraction approximation; linkage not physically constrained'})", """elif n.startswith(('Panto lower','Panto upper','Contact bow','Carbon strip')):
  stage='lower' if n.startswith('Panto lower') else 'upper' if n.startswith('Panto upper') else 'bow'
  pp={'lower':(bx-.45,0,4.56),'upper':(bx+.7,0,5.18),'bow':(bx-.25,0,5.85)}[stage]
  add('Pantograph_'+side+'_'+stage,'pantograph_'+stage,o,pp,{'side':side,'linkage':{'base_cm':[(bx-.45)*100,0,456],'elbow_cm':[(bx+.7)*100,0,518],'contact_cm':[(bx-.25)*100,0,585],'rotation_axis':[0,1,0]}})""")
s=s.replace("'Bogie','Primary spring','Suspension damper','Axle box'", "'Bogie','Primary spring','Primary coil','Suspension damper','Axle box','Axle bearing'")
s=s.replace("bpy.ops.export_scene.fbx(filepath=os.path.join(folder,key+'.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False)", """bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 for v in ob.data.vertices:v.co*=100
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 filename='SM_WAG9_01_'+key+'_LOD0.fbx'
 bpy.ops.export_scene.fbx(filepath=os.path.join(folder,filename),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False)""")
s=s.replace("'fbx':'Parts/'+key+'.fbx'", "'fbx':'Parts/'+filename")
s=s.replace("'locomotive':'WAG9'", "'locomotive':'WAG9','family':'WAG9','axis':'+X forward +Y right +Z up'")
s=s.replace("'fbx_unit':'meters; import FBX unit conversion enabled'", "'fbx_unit':'centimeters','fbx_import_settings':{'convert_scene':True,'force_front_x_axis':True,'convert_scene_unit':True}")
s=s.replace("'Pantograph linkage presentation uses approximate retraction.',", "'Pantograph planar linkage uses exported pivots; runtime binding requires verification.',")
s=s.replace("'materials':{'Blue window glass':{'blend_mode':'Translucent','opacity':.23}}", "'materials':{m.name:{'rgba':list(m.diffuse_color),'base_color':list(m.diffuse_color),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value,'blend_mode':'Translucent' if m==glass else 'Opaque','opacity':.14 if m==glass else 1} for m in bpy.data.materials if m.use_nodes and m.node_tree.nodes.get('Principled BSDF')}")
s=s.replace("bpy.ops.wm.save_as_mainfile(filepath=SRC)", "# Source .blend retains metre modeling units, exporter only transforms evaluated copies.")
p.write_text(s)
