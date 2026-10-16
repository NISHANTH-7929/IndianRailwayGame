import unreal,runpy
for name in ['Material_012','Material_019']:
 material=unreal.load_asset('/Game/IndianRailway/S8/Coaches/LHB3A/Materials/'+name)
 if material is None:raise RuntimeError('Missing material '+name)
 material.set_editor_property('two_sided',True)
 unreal.MaterialEditingLibrary.recompile_material(material)
 unreal.EditorAssetLibrary.save_loaded_asset(material)
runpy.run_path('D:/Project/IndianRailwayGame/07_Documentation/S8/Tools/review_lhb.py',run_name='__main__')
