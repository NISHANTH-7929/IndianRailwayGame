"""Bounded, no-save Egmore skylight isolation; parent owns editor execution.

Use a disposable editor process, never an editor containing unsaved user work.
Only transient capture actor and in-memory light visibility/affects_world change.
"""
import hashlib
import json
import time
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[3]
CONTENT = ROOT / '03_UnrealProject/MyProject/Content'
OUT = ROOT / '07_Documentation/S8/Evidence/StabilizationDesktop-20261011/Lighting'
OUT.mkdir(parents=True, exist_ok=True)
assert not (OUT / 'receipt.json').exists(), 'Preserve previous receipt: choose a new evidence directory'


def hashes():
    return {p.relative_to(CONTENT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in CONTENT.rglob('*.umap')}


def props(obj, names):
    values = {}
    for name in names:
        try:
            v = obj.get_editor_property(name)
            values[name] = v if v is None or isinstance(v, (str, int, float, bool)) else str(v)
        except Exception as e:
            values[name] = {'unavailable': str(e)}
    return values


before = hashes()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/IndianRailway/S8/Egmore/L_RailService_Egmore')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name() == 'L_RailService_Egmore'
lights = sorted([a for a in actors.get_all_level_actors() if isinstance(a, unreal.SkyLight)],
                key=lambda a: a.get_actor_label())
assert 1 <= len(lights) <= 6, 'Unexpected skylight population; stop bounded diagnostic'
names = ['intensity', 'mobility', 'visible', 'affects_world', 'source_type', 'cubemap',
         'real_time_capture', 'cast_shadows', 'lower_hemisphere_is_black',
         'lower_hemisphere_color', 'sky_distance_threshold', 'indirect_lighting_intensity']
original = [(a.light_component, a.light_component.get_editor_property('visible'),
             a.light_component.get_editor_property('affects_world')) for a in lights]
report = {'status': 'RUNNING', 'desktopOnly': True, 'assetsSaved': False,
          'camera': {'position': [3000, -560, 250], 'rotation': [5, 0, 0], 'fov': 78,
                     'resolution': [1600, 900], 'manualExposureBias': .5},
          'lights': [{'label': a.get_actor_label(), 'path': a.get_path_name(),
                      'properties': props(a.light_component, names)} for a in lights],
          'captures': [], 'errors': [],
          'interpretation': 'Visibility isolation is diagnostic only. Multiple skylights need not add linearly; renderer selection and capture freshness can matter. No correction is selected automatically.'}
(OUT / 'initial-state.json').write_text(json.dumps(report, indent=2))
capture = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(3000, -560, 250),
                                       unreal.Rotator(pitch=5, yaw=0, roll=0))
c = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
for k, v in [('capture_every_frame', False), ('capture_on_movement', False),
             ('always_persist_rendering_state', True), ('fov_angle', 78),
             ('post_process_blend_weight', 1.0)]:
    c.set_editor_property(k, v)
target = unreal.RenderingLibrary.create_render_target2d(world, 1600, 900,
                                                       unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
c.set_editor_property('texture_target', target)
c.set_editor_property('capture_source', unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
pp = c.get_editor_property('post_process_settings')
for k, v in [('override_auto_exposure_method', True), ('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL),
             ('override_auto_exposure_bias', True), ('auto_exposure_bias', .5),
             ('override_auto_exposure_apply_physical_camera_exposure', True),
             ('auto_exposure_apply_physical_camera_exposure', False)]:
    pp.set_editor_property(k, v)
c.set_editor_property('post_process_settings', pp)
# Baseline recapture separates capture-cache effects from light-selection effects.
stages = [('baseline_saved', None, False), ('baseline_recaptured', None, True),
          ('none', [], True)] + [('only_' + a.get_actor_label(), [i], True)
                                 for i, a in enumerate(lights)] + [('all_recaptured', list(range(len(lights))), True)]
index = 0
started = time.monotonic()
ready = started + 40
pending = None


def configure(stage):
    name, enabled, recapture = stage
    for i, (component, visible, affects) in enumerate(original):
        component.set_visibility(visible if enabled is None else i in enabled)
        component.set_editor_property('affects_world', affects if enabled is None else i in enabled)
    if recapture:
        for component, _, _ in original:
            if component.get_editor_property('visible') and component.get_editor_property('affects_world'):
                component.recapture_sky()


def finish(error=None):
    if error:
        report['errors'].append(str(error))
    for component, visible, affects in original:
        try:
            component.set_visibility(visible)
            component.set_editor_property('affects_world', affects)
        except Exception as e:
            report['errors'].append('restore: ' + str(e))
    actors.destroy_actor(capture)
    after = hashes()
    report.update(mapsUnchanged=before == after, mapHashesBefore=before, mapHashesAfter=after,
                  elapsedSeconds=time.monotonic() - started,
                  status='PASS' if not report['errors'] and before == after else 'FAIL')
    (OUT / 'receipt.json').write_text(json.dumps(report, indent=2))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()


def tick(dt):
    global index, ready, pending
    try:
        now = time.monotonic()
        if now - started > 300:
            raise RuntimeError('Lighting diagnostic exceeded 300-second bound')
        if now < ready:
            return
        if pending is None:
            c.capture_scene()
            pending = now
            return
        if now - pending < 3:
            return
        name, enabled, recapture = stages[index]
        unreal.RenderingLibrary.export_render_target(world, target, str(OUT), name + '.png')
        path = OUT / (name + '.png')
        assert path.is_file() and path.stat().st_size > 1024, 'Capture missing: ' + name
        report['captures'].append({'name': name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                                   'recaptured': recapture,
                                   'lights': [{'label': a.get_actor_label(), 'properties': props(a.light_component, names)}
                                              for a in lights]})
        index += 1
        pending = None
        if index == len(stages):
            finish()
            return
        configure(stages[index])
        ready = now + 14
    except Exception as e:
        finish(e)


handle = unreal.register_slate_post_tick_callback(tick)
