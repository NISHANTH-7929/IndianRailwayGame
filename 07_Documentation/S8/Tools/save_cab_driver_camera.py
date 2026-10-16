import bpy, math
from mathutils import Vector
p=r'D:/Project/IndianRailwayGame/04_Assets/S8/Cabs/WAP7/WAP7_Cab.blend'
bpy.ops.wm.open_mainfile(filepath=p)
bpy.ops.object.camera_add(location=(7.8,.65,3.35))
c=bpy.context.object;c.name='WAP7_Driver_Android_View';c.rotation_euler=(Vector((1,0,-math.tan(math.radians(12))))).to_track_quat('-Z','Y').to_euler();c.data.sensor_fit='HORIZONTAL';c.data.lens=36/(2*math.tan(math.radians(49)));c.data.clip_start=.02
bpy.context.scene.camera=c;bpy.context.scene.render.resolution_x=2800;bpy.context.scene.render.resolution_y=1260
bpy.ops.wm.save_as_mainfile(filepath=p)
