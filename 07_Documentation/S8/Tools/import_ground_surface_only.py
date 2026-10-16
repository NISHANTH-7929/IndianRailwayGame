"""Import the ground shader alone, preserving other assets, then close cleanly."""
from pathlib import Path
import unreal

path = Path(__file__).with_name('import_ground_surface_refinement.py')
exec(compile(path.read_text(), str(path), 'exec'), {'__file__': str(path)})
ticks = [0]

def finish(dt):
    ticks[0] += 1
    if ticks[0] > 90:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(finish)
