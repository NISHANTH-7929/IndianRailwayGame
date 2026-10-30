"""Full editor entry point; let asset-editor notifications settle before exit."""
from pathlib import Path
import runpy
import unreal

try:
    runpy.run_path(str(Path(__file__).with_name('import_cab_hardware_v1.py')), run_name='__main__')
finally:
    ticks = [0]

    def finish(_delta_time):
        ticks[0] += 1
        if ticks[0] >= 60:
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()

    handle = unreal.register_slate_post_tick_callback(finish)
