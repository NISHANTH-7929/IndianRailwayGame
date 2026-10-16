"""Run with Unreal's PythonScript commandlet after preparing the WAV files."""
import json
from pathlib import Path
import unreal

root=Path(__file__).resolve().parent
manifest=json.loads((root/'Pixabay_20261009/manifest.json').read_text())
results=[]
for clip in manifest['clips']:
    path=clip['asset']
    task=unreal.AssetImportTask()
    task.filename=str(root/clip['prepared'])
    task.destination_path=path.rsplit('/',1)[0]
    task.destination_name=path.rsplit('/',1)[1]
    task.automated=True
    task.replace_existing=False
    task.save=True
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=unreal.load_asset(path)
    assert isinstance(sound,unreal.SoundWave),path
    sound.set_editor_property('looping',clip['looping'])
    sound.set_editor_property('volume',1.)
    sound.set_editor_property('compression_quality',70)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
    assert sound.get_editor_property('num_channels')==1
    assert abs(sound.get_editor_property('duration')-clip['durationSeconds'])<.05
    results.append(dict(asset=path,duration=sound.get_editor_property('duration'),looping=sound.get_editor_property('looping')))
(root/'Pixabay_20261009/import-result.json').write_text(json.dumps(results,indent=2))
unreal.log('PIXABAY_AUDIO_IMPORT_PASS')
