"""Offline safety contracts; every subprocess/device entry point is mocked."""
import ast
import copy
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from PIL import Image
import device_cab_stabilization as cab
from test_device_egmore_service import telemetry


def picture(size=(2800, 1260)):
    data = io.BytesIO()
    Image.new('RGB', size).save(data, format='PNG')
    return data.getvalue()


class Clock:
    def __init__(self):
        self.now = 0

    def monotonic(self):
        return self.now

    def sleep(self, duration):
        self.now += duration


class Safety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.png = picture()

    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.out = Path(temp.name)
        self.reads = self.logs = 0
        self.touches = []
        self.clock = Clock()
        self.missing = self.blocked = self.wrong_size = self.bad_focus = False
        # A second independent trap ensures future runner changes cannot bypass
        # the subprocess mock through the shared device helper.
        guard = patch.object(cab.smoke.device, 'adb', side_effect=AssertionError('Real device access prohibited'))
        guard.start()
        self.addCleanup(guard.stop)

    def subprocess(self, command, **kwargs):
        self.clock.now += .05
        self.assertLessEqual(kwargs['timeout'], 20)
        args = command[3:]
        output = b''
        if args == ['exec-out', 'cat', cab.smoke.TELEMETRY]:
            self.reads += 1
            self.assertFalse(kwargs['check'])
            if self.missing and self.reads == 1:
                return SimpleNamespace(stdout=b'', returncode=1)
            state = telemetry()
            state['logical']['tick'] = self.reads
            state['service']['phase'] = 'Running'
            state['locomotive']['cabCamera'] = True
            state['blocked'] = self.blocked and self.reads > 1
            output = json.dumps(state).encode()
        elif args == ['exec-out', 'cat', cab.smoke.device.SAVED + '/Logs/MyProject.log']:
            self.logs += 1
            self.assertFalse(kwargs['check'])
            if not (self.missing and self.logs == 1):
                output = f'TOKEN LogWorld: Bringing World {cab.smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        elif args == ['shell', 'dumpsys', 'window']:
            output = b'mCurrentFocus overlay.package' if self.bad_focus else b'mCurrentFocus ' + cab.smoke.device.PACKAGE.encode()
        elif args == ['exec-out', 'screencap', '-p']:
            output = picture((1280, 720)) if self.wrong_size else self.png
        elif args[:3] == ['shell', 'input', 'tap']:
            self.touches.append(args[3:])
            raise RuntimeError('Mock stop after first tap')
        else:
            self.fail('Unexpected subprocess: ' + repr(args))
        return SimpleNamespace(stdout=output, returncode=0)

    def run_until_guard(self):
        with patch.object(cab.subprocess, 'run', side_effect=self.subprocess), patch.object(cab, 'time', self.clock):
            with self.assertRaises(RuntimeError):
                cab.exercise(self.out, 'TOKEN', 540, self.png)
        receipt = json.loads((self.out / 'cab-stabilization-checks.json').read_text())
        self.assertFalse(receipt['automatedPassed'])
        self.assertFalse(receipt['checks']['sequence-completed'])
        self.assertIsNone(receipt['visualReadoutAgreementVerified'])
        self.assertTrue(receipt['error'])
        return receipt

    def test_layout_proof_correct_size(self):
        cab.verify_picture(self.png)

    def test_layout_proof_wrong_size_before_runner(self):
        proof = self.out / 'wrong.png'
        proof.write_bytes(picture((1280, 720)))
        with patch('sys.argv', ['runner', '--apk', 'unused.apk', '--label', 'unused', '--layout-proof', str(proof)]), patch.object(cab.smoke, 'run') as run:
            with self.assertRaises(RuntimeError):
                cab.main()
            run.assert_not_called()

    def test_startup_missing_telemetry_and_log_retry(self):
        self.missing = True
        self.run_until_guard()
        self.assertGreaterEqual(self.reads, 4)
        self.assertEqual(self.touches, [['112', '1131']])

    def test_first_action_power_zero_and_error_receipt(self):
        self.run_until_guard()
        self.assertEqual(self.touches, [['112', '1131']])
        self.assertTrue(list(self.out.glob('*initial-cab.png')))
        self.assertTrue(list(self.out.glob('*before-initial-stop-power-zero.json')))

    def test_invalid_pre_action_state_blocks_touch(self):
        self.blocked = True
        self.run_until_guard()
        self.assertEqual(self.touches, [])

    def test_runtime_wrong_size_blocks_touch(self):
        self.wrong_size = True
        self.run_until_guard()
        self.assertEqual(self.touches, [])

    def test_overlay_focus_blocks_touch(self):
        self.bad_focus = True
        self.run_until_guard()
        self.assertEqual(self.touches, [])


class PhysicalSafety(unittest.TestCase):
    def invoke(self, mutate=None):
        # Execute the real nested helper in isolation, supplying only mocked
        # guards/I/O; this does not duplicate its coordinate-validation logic.
        tree = ast.parse(Path(cab.__file__).read_text())
        function = next(node for node in ast.walk(tree) if isinstance(node, ast.FunctionDef) and node.name == 'physical')
        state = dict(locomotive=dict(cabCamera=True, instrumentView=False,
                     viewport=[2000, 900], interactions=3,
                     controlPoses=[dict(role='throttle', screen=[1000, 500])]),
                     handling=dict(speedMps=0, throttle=0))
        if mutate:
            mutate(state)
        after = copy.deepcopy(state)
        after['locomotive']['interactions'] = 4
        states = iter([state, after])
        calls, checks = [], {}
        namespace = dict(math=cab.math, player=lambda s: s['handling'],
                         fresh=lambda name: next(states), remaining=lambda: None,
                         SIZE=cab.SIZE, adb=lambda *args: calls.append(args),
                         actions=[], previous_tick=100, time=Clock(),
                         check=lambda k, v: checks.update({k: v}), pose_checks=lambda *args: None)
        exec(compile(ast.Module(body=[function], type_ignores=[]), '<physical-helper>', 'exec'), namespace)
        error = None
        try:
            namespace['physical']('throttle', 'physical-test')
        except RuntimeError as exc:
            error = str(exc)
        return calls, checks, error

    def test_scaled_stationary_swipe(self):
        calls, checks, error = self.invoke()
        self.assertIsNone(error)
        self.assertEqual(calls, [('shell', 'input', 'swipe', '1400', '700', '1400', '700', '160')])
        self.assertTrue(all(checks.values()))

    def test_projection_boundaries_and_nan_rejected(self):
        for point in ([0, 500], [2000, 500], [1000, 161], [1000, 774], [float('nan'), 500], [1000, float('inf')]):
            with self.subTest(point=point):
                calls, _, error = self.invoke(lambda s: s['locomotive']['controlPoses'][0].update(screen=point))
                self.assertEqual(calls, [])
                self.assertIsNotNone(error)

    def test_invalid_viewport_rejected(self):
        for viewport in ([0, 900], [-1, 900], [2000, float('nan')], []):
            with self.subTest(viewport=viewport):
                calls, _, error = self.invoke(lambda s: s['locomotive'].update(viewport=viewport))
                self.assertEqual(calls, [])
                self.assertIsNotNone(error)

    def test_motion_zoom_and_power_rejected(self):
        for mutate in (lambda s: s['handling'].update(speedMps=.03), lambda s: s['handling'].update(throttle=.25), lambda s: s['locomotive'].update(instrumentView=True)):
            calls, _, error = self.invoke(mutate)
            self.assertEqual(calls, [])
            self.assertIsNotNone(error)


if __name__ == '__main__':
    unittest.main()
