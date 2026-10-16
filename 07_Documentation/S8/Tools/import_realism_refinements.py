"""Coordinated local derivative asset import, one editor process."""
import os,unreal
from pathlib import Path
os.environ['RAIL_KEEP_EDITOR_OPEN']='1'
for name in ('import_lhb_underframe_v2.py','import_chennai_canopy_v3.py'):
    source=Path(__file__).with_name(name)
    exec(compile(source.read_text(),str(source),'exec'),{'__file__':str(source)})
unreal.log('REALISM_REFINEMENTS_IMPORTED')
