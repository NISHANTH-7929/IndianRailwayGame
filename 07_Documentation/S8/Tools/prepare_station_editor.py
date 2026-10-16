from pathlib import Path
for name in ['import_station_kit.py','create_station_map.py']:
    source=Path(__file__).with_name(name)
    exec(compile(source.read_text(),str(source),'exec'),{'__file__':str(source),'__name__':'__main__'})
