"""Read-only full editor LOD audit; exits after the editor completes startup."""
from pathlib import Path
import runpy
import unreal
runpy.run_path(str(Path(__file__).with_name('audit_chennai_materials.py')), run_name='__main__')
def finish(dt):
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
