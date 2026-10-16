from pathlib import Path
script=Path(__file__).with_name('import_station_kit.py')
code=script.read_text().replace('S8/StationKit','S8/EgmoreSignage').replace("'Kit01'","'EgmoreSignage01'").replace("'Kit01.json'","'EgmoreSignage01.json'").replace('Evidence/Station','Evidence/Egmore/Signage').replace('STATION_IMPORT_COMPLETE','EGMORE_SIGN_IMPORT_COMPLETE')
exec(compile(code,str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
import unreal
texture=unreal.load_asset('/Game/IndianRailway/S8/Stations/EgmoreSignage01/Textures/T_EgmoreName_BC')
# A single station-name atlas must remain readable at close range, including
# review captures which do not contribute ordinary player streaming feedback.
texture.set_editor_property('never_stream',True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
