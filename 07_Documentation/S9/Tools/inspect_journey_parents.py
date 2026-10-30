import unreal,json
m=unreal.load_asset('/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/SM_LHB_BodyV2')
for i in range(len(m.get_editor_property('static_materials'))):
 p=m.get_material(i)
 if p and p.get_name().startswith('LHB_'): unreal.log('JOURNEY_PARENT '+p.get_path_name()+' '+str(unreal.MaterialEditingLibrary.get_scalar_parameter_names(p)))
