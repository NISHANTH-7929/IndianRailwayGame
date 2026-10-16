"""Refresh bounded nameboard textures and leaf material, then deferred exit."""
from pathlib import Path
for filename in ('import_chennai_nameboards.py','import_chennai_leaf_cluster.py'):
    script=Path(__file__).with_name(filename)
    exec(compile(script.read_text(),str(script),'exec'),{'__file__':str(script)})
