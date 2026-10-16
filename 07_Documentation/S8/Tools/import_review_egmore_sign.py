from pathlib import Path
for name in ['import_egmore_sign.py','repair_egmore_sign_materials.py']:
    p=Path(__file__).with_name(name)
    exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p),'__name__':'__main__'})
