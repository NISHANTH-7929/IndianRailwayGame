"""Unreal PythonScript commandlet import of the S10 RealRecordingsV1 sound waves."""
import hashlib
import json
from pathlib import Path
import unreal

root = Path(__file__).resolve().parent
manifest = json.loads((root/'manifest.json').read_text(encoding='utf-8'))
results = []
for clip in manifest['clips']:
    source = root/clip['prepared']
    assert hashlib.sha256(source.read_bytes()).hexdigest() == clip['preparedSha256'], source
    assert not unreal.EditorAssetLibrary.does_asset_exist(clip['asset']), 'Use a new revision: ' + clip['asset']
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path, task.destination_name = clip['asset'].rsplit('/', 1)
    task.automated = True
    task.replace_existing = False
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(clip['asset'])
    assert isinstance(sound, unreal.SoundWave), clip['asset']
    sound.set_editor_property('looping', clip['looping'])
    sound.set_editor_property('volume', 1.)
    sound.set_editor_property('compression_quality', 80)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
    assert sound.get_editor_property('num_channels') == 1
    assert abs(sound.get_editor_property('duration') - clip['durationSeconds']) < .03
    results.append(dict(asset=clip['asset'], duration=sound.get_editor_property('duration'), looping=sound.get_editor_property('looping')))
(root/'import-result.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
unreal.log_warning('S10_REAL_RECORDINGS_V1_IMPORT_PASS')
