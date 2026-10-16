"""One coordinated editor import, then graceful close after shader/import work."""
import os,unreal
from pathlib import Path
os.environ['RAIL_KEEP_EDITOR_OPEN']='1'
for name in ('import_lhb_underframe_v2.py','import_ground_surface_refinement.py'):
 p=Path(__file__).with_name(name)
 exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
unreal.log('SURFACE_PASS_IMPORT_COMPLETE')
ticks=[0]
def finish(dt):
 ticks[0]+=1
 if ticks[0]>90:
  unreal.unregister_slate_post_tick_callback(handle)
  unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
