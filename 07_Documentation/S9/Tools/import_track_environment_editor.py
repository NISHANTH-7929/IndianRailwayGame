"""Serial coordinated asset import followed by a delayed editor exit."""
from pathlib import Path
import runpy
import time
import unreal
try:
    for script in ('import_track_environment_materials.py','import_track_environment_meshes.py'):
        runpy.run_path(str(Path(__file__).with_name(script)),run_name='__main__')
finally:
    finished=time.monotonic()
    def finish(_dt):
        if time.monotonic()-finished<10:return
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(finish)
