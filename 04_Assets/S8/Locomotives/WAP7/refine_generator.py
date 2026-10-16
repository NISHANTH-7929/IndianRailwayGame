from pathlib import Path
p=Path(__file__).with_name('generate_wap7.py')
s=p.read_text()
s=s.replace("# chassis and machine room", "glass.diffuse_color=(.12,.24,.26,.18)\nglass.node_tree.nodes.get('Principled BSDF').inputs['Alpha'].default_value=.18\n# chassis and machine room")
s=s.replace("  box('WindowRubber',(x+.005*end,y,3.17),(.035,1.27,.82),dark,.04)","  for yy in [y-.615,y+.615]:box('WindowRubber',(x+.005*end,yy,3.17),(.035,.04,.82),dark,.012)\n  for zz in [2.78,3.56]:box('WindowRubber',(x+.005*end,y,zz),(.035,1.27,.04),dark,.012)")
s=s.replace("(end*8.51,y,2.41)","(end*8.51,y,2.43)")
a=s.index(" group('Pantograph_'+str(x)")
b=s.index(" group('Exterior','exterior')",a)
s=s[:a]+''' group('PantoLower_'+str(x),'pantograph_lower',(x-.72,0,4.3))
 for y in [-.52,.52]:rod('PantoLower',(x-.72,y,4.3),(x+.65,y,5.03),.035,red)
 rod('PantoTie',(x+.65,-.52,5.03),(x+.65,.52,5.03),.032,steel)
 group('PantoUpper_'+str(x),'pantograph_upper',(x+.65,0,5.03))
 for y in [-.52,.52]:rod('PantoUpper',(x+.65,y,5.03),(x-.55,y,5.64),.025,steel)
 group('PantoShoe_'+str(x),'pantograph_shoe',(x-.55,0,5.66))
 box('PantoShoe',(x-.55,0,5.66),(.12,1.95,.055),dark,.025)
 for y in [-1,1]:rod('PantoHorn',(x-.55,y*.95,5.66),(x-.55,y*1.17,5.47),.022,steel)
''' +s[b:]
s=s.replace(" if roles[name]=='pantograph':\n  ob.scale.z=.12;ob.keyframe_insert(data_path='scale',frame=1);ob.scale.z=1;ob.keyframe_insert(data_path='scale',frame=30)\n",'')
s=s.replace("sc.frame_set(30)",'''# Articulated pantograph keyframes solve two rigid links; no scaled geometry.
for x in [-5.7,5.7]:
 lower=bpy.data.objects['SM_WAP7_01_PantoLower_'+str(x)+'_LOD0']
 upper=bpy.data.objects['SM_WAP7_01_PantoUpper_'+str(x)+'_LOD0']
 shoe=bpy.data.objects['SM_WAP7_01_PantoShoe_'+str(x)+'_LOD0']
 base=Vector((x-.72,0,4.3))*100
 a0=math.atan2(.73,1.37);b0=math.atan2(.61,-1.2);l1=math.hypot(.73,1.37)*100;l2=math.hypot(.61,1.2)*100
 for frame,aa,bb in [(1,.06,math.pi-.06),(30,a0,b0)]:
  elbow=base+Vector((math.cos(aa)*l1,0,math.sin(aa)*l1))
  top=elbow+Vector((math.cos(bb)*l2,0,math.sin(bb)*l2))
  lower.rotation_euler.y=a0-aa;lower.keyframe_insert(data_path='rotation_euler',frame=frame)
  upper.location=elbow;upper.rotation_euler.y=b0-bb;upper.keyframe_insert(data_path='location',frame=frame);upper.keyframe_insert(data_path='rotation_euler',frame=frame)
  shoe.location=top+Vector((0,0,2));shoe.keyframe_insert(data_path='location',frame=frame)
sc.frame_set(30)''')
s=s.replace("'triangles_lod0':tri,", "'triangles_lod0':tri,'materials':[{'name':m.name,'base_color':list(m.diffuse_color),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value} for m in bpy.data.materials if m.use_nodes and m.node_tree.nodes.get('Principled BSDF')],")
p.write_text(s)
