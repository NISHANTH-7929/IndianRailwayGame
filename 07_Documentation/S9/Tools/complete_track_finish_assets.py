"""Persist unchanged finish derivatives and verify the final rail bindings."""
from pathlib import Path
import json
import time
import unreal
ROOT=Path(__file__).resolve().parents[3]
BASE='/Game/IndianRailway/S9/TrackEnvironmentV1/'
OUT=ROOT/'07_Documentation/S9/Evidence/TrackEnvironment-20261025-01/finish-persistence.json'
assert not OUT.exists()
try:
    saved=[]
    for name in ('M_RailHead','M_RailRust','M_Fastener'):
        target=BASE+'MaterialsV2/'+name
        assert not unreal.EditorAssetLibrary.does_asset_exist(target)
        material=unreal.EditorAssetLibrary.duplicate_asset(BASE+'Materials/'+name,target)
        assert material and unreal.EditorAssetLibrary.save_loaded_asset(material)
        saved.append(target)
    mesh=unreal.load_asset(BASE+'Meshes/SM_Track6m_RealismV1')
    mapping={'RailHead':'M_RailHead','RailRust':'M_RailRust','Concrete':'M_SleeperConcrete'}
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.get_editor_property('imported_material_slot_name'))
        material=unreal.load_asset(BASE+'MaterialsV2/'+mapping[key]);assert material
        mesh.set_material(index,material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    OUT.write_text(json.dumps(dict(passed=True,persisted=saved,
        railMaterials=[mesh.get_material(i).get_path_name() for i in range(3)]),indent=2))
finally:
    started=time.monotonic()
    def finish(_dt):
        if time.monotonic()-started<10:return
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
