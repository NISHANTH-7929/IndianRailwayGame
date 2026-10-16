"""Import the separate Egmore study through the proven station material/LOD path."""
from pathlib import Path
script=Path(__file__).with_name('import_station_kit.py')
code=script.read_text().replace("S8/StationKit","S8/EgmoreKit").replace("'Kit01'","'EgmoreHeritage01'").replace("'Kit01.json'","'EgmoreHeritage01.json'").replace("Evidence/Station","Evidence/Egmore").replace('STATION_IMPORT_COMPLETE','EGMORE_IMPORT_COMPLETE')
exec(compile(code,str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
