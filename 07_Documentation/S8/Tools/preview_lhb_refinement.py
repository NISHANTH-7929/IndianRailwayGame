"""Local art inspection only; no Unreal execution or simulation claim."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[3];out=root/'04_Assets/S8/LHBUnderframeV2'
bpy.ops.wm.open_mainfile(filepath=str(out/'LHB_UnderframeV2_Refined.blend'),load_ui=False,use_scripts=False)
body=bpy.data.objects['SM_LHB_BodyV2'];bogie=bpy.data.objects['SM_LHB_BogieV2'];bogie.location.x=-7.45
copy=bogie.copy();copy.data=bogie.data;bpy.context.collection.objects.link(copy);copy.location.x=7.45
for x in (-7.45,7.45):
 for offset in (-1.28,1.28):
  bpy.ops.object.select_all(action='DESELECT');bpy.ops.import_scene.fbx(filepath=str(root/'04_Assets/S8/RunningGearV2/SM_LHB_Wheelset.fbx'))
  for o in bpy.context.selected_objects:
   o.location.x+=x+offset;o.location.z+=.4575
   for m in o.data.materials:
    if not m:continue
    m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF')
    if bs:
     bs.inputs['Base Color'].default_value=(.15,.16,.16,1);bs.inputs['Metallic'].default_value=.8;bs.inputs['Roughness'].default_value=.32
# Source shell material paint reflectance corrected locally, consistent with
# prepared Unreal derivative materials. Original .blend is never saved over.
for m in body.data.materials:
 if not m:continue
 m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF')
 if not bs:continue
 if m.name=='Material.004':bs.inputs['Base Color'].default_value=(.48,.007,.02,1);bs.inputs['Metallic'].default_value=0;bs.inputs['Roughness'].default_value=.38
 if m.name=='Material.006':bs.inputs['Base Color'].default_value=(.43,.46,.46,1);bs.inputs['Metallic'].default_value=0;bs.inputs['Roughness'].default_value=.44
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.09));plane=bpy.context.object;ground=bpy.data.materials.new('Inspection ground');ground.diffuse_color=(.11,.12,.13,1);plane.data.materials.append(ground)
bpy.ops.object.light_add(type='AREA',location=(-8,-7,12));light=bpy.context.object;light.data.energy=2400;light.data.shape='DISK';light.data.size=9
light.rotation_euler=(Vector((0,0,1))-light.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene;scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera;camera.data.lens=48
for name,pos,target in [('bogie',(-10.1,-5.2,1.8),(-7.45,0,.95)),('coach',(-21,-25,8),(0,0,2))]:
 camera.location=pos;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/('refinement-'+name+'.png'));bpy.ops.render.render(write_still=True)
print('LHB_LOCAL_PREVIEW_COMPLETE')
