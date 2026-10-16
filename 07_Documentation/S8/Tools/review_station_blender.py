"""Assembled-kit visual review, independent of runtime correctness."""
import bpy,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3];source=ROOT/'04_Assets/S8/StationKit'
bpy.ops.wm.open_mainfile(filepath=str(source/'StationKit.blend'),load_ui=False)
scene=bpy.context.scene
def add(name,x,y,z=0,yaw=0):
    original=bpy.data.objects['SM_Station_'+name];o=original.copy();o.data=original.data
    scene.collection.objects.link(o);o.hide_render=False;o.hide_set(False);o.location=(x,y,z);o.rotation_euler=(0,0,math.radians(yaw));return o
for i in range(10):
    add('Platform5m',95+i*5,-6.675)
    for y in [-11.675,-16.675]:add('Apron5m',95+i*5,y)
for i in range(6):add('Canopy6m',101+i*6,-6.675,.84)
for x in [102,112,125,137]:add('Bench',x,-6.2,.85)
for x in [98,142]:add('StationBoard',x,-6.4,.85);add('LampPost',x,-7,.85)
add('VendorKiosk',132,-11,.85);add('StationFacade6m',111,-13,.84,180)
add('WaitingShelter',100,-11,.85)
for i in range(16):add('Boundary3m',95+i*3,-17)
for i in range(4):add('Railing3m',98+i*3,-7.6,.85)
for x in range(75,165,3):add('Rail3m',x,0,-.1825);add('Ballast3m',x,0,-.18);add('ContactWire3m',x,0,5.5)
for i in range(150):add('Sleeper',75+i*.6,0,-.18)
for x in [90,150]:add('OHEMast',x,-3.3);add('ElectricalCabinet',x-2,-4)
for x in [75,125,175]:add('Ground50m',x,0)
add('StairFlight',120,-11,.85);add('StairFlight',126.38,-11,4.37)
for i in range(3):add('Footbridge6m',132.76,-11+i*6,7.89,90)
add('StairFlight',139.14,7,4.37,180);add('StairFlight',145.52,7,.85,180)
for y in [-8,5]:add('BridgePier',132.76,y,.4)
scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.resolution_x=1800;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Station sky');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.48,.6,1)
bpy.ops.object.light_add(type='SUN');bpy.context.object.data.energy=2;bpy.context.object.rotation_euler=(.5,-.5,-.4)
bpy.ops.object.camera_add(location=(156,27,19));cam=bpy.context.object
cam.rotation_euler=(Vector((119,-7,2.5))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=38;scene.camera=cam
scene.render.filepath=str(ROOT/'07_Documentation/S8/Evidence/Station/assembled-blender.png');bpy.ops.render.render(write_still=True)
