"""Create only provisional S7 assets, leaving earlier stage assets intact."""
import math
import pathlib
import struct
import wave
import unreal

ROOT = '/Game/IndianRailway/S7'
WORKSPACE = pathlib.Path(__file__).resolve().parents[3]
audio_dir = WORKSPACE / '07_Documentation/S7/Assets'
audio_dir.mkdir(parents=True, exist_ok=True)
rate = 22050
for name, duration in [('Horn', 1.1), ('Rail', 2.0), ('StationCue', .65)]:
    samples = []
    for i in range(round(rate * duration)):
        t = i / rate
        envelope = min(1, t / .04, (duration - t) / .08)
        if name == 'Horn':
            value = .24 * (math.sin(2*math.pi*330*t) + .6*math.sin(2*math.pi*440*t)) * envelope
        elif name == 'Rail':
            # Periodic synthetic rolling and joint pulses; deterministic loop.
            value = .08*math.sin(2*math.pi*90*t) + .13*math.exp(-35*(t % .5))*math.sin(2*math.pi*640*t)
        else:
            value = .25*math.sin(2*math.pi*(660 if t < .3 else 880)*t)*envelope
        samples.append(struct.pack('<h', round(value * 32767)))
    path = audio_dir / (name + '.wav')
    with wave.open(str(path), 'wb') as output:
        output.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
        output.writeframes(b''.join(samples))
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(path))
    task.set_editor_property('destination_path', ROOT)
    task.set_editor_property('destination_name', 'SW_' + name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(ROOT + '/SW_' + name)
    assert sound, name
    sound.set_editor_property('looping', name == 'Rail')
    unreal.EditorAssetLibrary.save_loaded_asset(sound)

unreal.EditorAssetLibrary.make_directory(ROOT)
factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.load_class(None, '/Script/MyProject.RailServiceDefinition'))
asset = unreal.load_asset(ROOT + '/DA_Shantipur')
if not asset:
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_Shantipur', ROOT, None, factory)
assert asset
asset.set_editor_property('tutorial_steps', [
    'Power moves the train; brake slows it. Speed is shown in km/h.',
    'The express has priority. Stop before the junction and wait for the route to clear.',
    'Berth at the platform marker, hold still for 10 seconds, then press Depart.',
    'Stop at Finish. Review the grade and save the reputation award. Assisted helps with braking.'
])
unreal.EditorAssetLibrary.save_loaded_asset(asset)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not unreal.EditorAssetLibrary.does_asset_exist(ROOT + '/L_RailService'):
    assert levels.new_level(ROOT + '/L_RailService', False)
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 500))
    assert levels.save_current_level()
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
unreal.log('S7_ASSETS_CREATED')
