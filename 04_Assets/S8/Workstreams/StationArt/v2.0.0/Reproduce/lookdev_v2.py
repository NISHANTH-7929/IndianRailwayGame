"""Reference lighting/wet-material previews only; never edits source or runtime."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1]/'v2.0.0';records=[]
for name,night,wet in [('DayWet',False,True),('NightDry',True,False),('NightWet',True,True)]:
    bpy.ops.wm.open_mainfile(filepath=str(R/'EgmoreScene.blend'));s=bpy.context.scene;s.camera=bpy.data.objects['PlatformEye']
    s.cycles.samples=32
    if wet:
        for m in bpy.data.materials:
            if any(t in m.name for t in ['PlatformTile','Asphalt','Concrete','RailHead']):
                p=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
                if not p:continue
                for link in list(p.inputs['Roughness'].links):m.node_tree.links.remove(link)
                p.inputs['Roughness'].default_value=.26;p.inputs['Coat Weight'].default_value=.22;p.inputs['Coat Roughness'].default_value=.18
    if night:
        bpy.data.objects['DaySun'].data.energy=.03
        s.world.node_tree.nodes['Background'].inputs[0].default_value=(.025,.04,.085,1)
        s.world.node_tree.nodes['Background'].inputs[1].default_value=.14
        for x in range(-90,91,18):
            for y in (-5.675,10.675,27.025):
                bpy.ops.object.light_add(type='AREA',location=(x,y,5.15));l=bpy.context.object;l.data.energy=200;l.data.shape='RECTANGLE';l.data.size=3;l.data.size_y=.6;l.data.color=(1,.86,.68)
        s.view_settings.exposure=1.0
    s.render.filepath=str(R/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
    records.append({'name':name,'night':night,'wetMaterialResponse':wet,'rainParticles':False,'runtimeValidated':False,'note':'Estimated source lighting/material appearance; no runtime weather/audio or device claim.'})
(R/'Evidence/lookdev.json').write_text(json.dumps(records,indent=2))
