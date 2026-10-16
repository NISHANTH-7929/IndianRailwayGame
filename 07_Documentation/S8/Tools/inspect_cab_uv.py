import bpy,json
from pathlib import Path
root=Path(r'D:/Project/IndianRailwayGame');out={}
bpy.ops.wm.open_mainfile(filepath=str(root/'04_Assets/S8/Cabs/WAP7/WAP7_Cab.blend'))
o=bpy.data.objects['SM_WAP7_Cab_StaticCore'];out['blend']=[{'name':u.name,'render':u.active_render,'sample':[list(x.uv) for x in u.data[:8]]} for u in o.data.uv_layers]
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(root/'04_Assets/S8/Cabs/WAP7/Parts/SM_WAP7_Cab_StaticCore.fbx'))
o=next(x for x in bpy.context.scene.objects if x.type=='MESH');out['fbx']=[{'name':u.name,'render':u.active_render,'sample':[list(x.uv) for x in u.data[:8]]} for u in o.data.uv_layers]
(root/'07_Documentation/S8/Evidence/Cab/uv-inspection.json').write_text(json.dumps(out,indent=2))
