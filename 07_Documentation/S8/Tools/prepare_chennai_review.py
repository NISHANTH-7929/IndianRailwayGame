"""Prepare only the original Chennai tint material, then inventory imported slots."""
from pathlib import Path
import runpy
here = Path(__file__).resolve().parent
runpy.run_path(str(here / 'create_chennai_material.py'), run_name='__main__')
runpy.run_path(str(here / 'audit_chennai_materials.py'), run_name='__main__')
