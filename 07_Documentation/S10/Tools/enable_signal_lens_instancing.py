"""Allow the S8 signal lens material on instanced meshes (S10 lineside signal heads use HISMs).

Run as an Unreal Python commandlet. Only the usage flag changes; the lens graph is untouched.
"""
import unreal
path = '/Game/IndianRailway/S8/Egmore/M_SignalLens'
m = unreal.load_asset(path); assert isinstance(m, unreal.Material), path
unreal.MaterialEditingLibrary.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
unreal.MaterialEditingLibrary.recompile_material(m)
assert unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log_warning('SIGNAL_LENS_INSTANCING_READY')
