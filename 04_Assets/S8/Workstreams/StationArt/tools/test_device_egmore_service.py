"""Mocked smoke safety checks; never connects to a phone."""
import copy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import device_egmore_service as smoke


def telemetry():
    return dict(content='IR-S8-EGMORE-1', service={'serviceId': 'S8-Egmore-01'},
                logical={'valid': True, 'faulted': False, 'tick': 120}, blocked=False, paused=False,
                logicalFingerprint='123456789abcdef0',
                loadedLevels=2, visualVehicles=1,
                locomotive={'ready': True}, trafficLocomotive={'ready': True},
                egmoreScene=dict(ready=True, sceneRebuilt=True, trackExtension=True,
                                 modules=39, placements=2594,
                                 signals=[{'id': 1, 'aspect': 'stop'}, {'id': 2, 'aspect': 'proceed'}]))


class Safety(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.apk = self.root / 'candidate.apk'
        self.apk.write_bytes(b'apk')
        self.args = SimpleNamespace(apk=str(self.apk), label='smoke', map=smoke.WORLD, duration=60, cab=False)
        self.saves = {f'{smoke.device.SAVED}/{family}/checkpoint-{slot}.irs': None
                      for family in ('S7', 'S8/LHB/Service', 'S8/Station/Service', 'S8/Egmore/Service') for slot in range(2)}
        self.before = dict(installedApkSha256=smoke.device.sha(b'apk'), wake='0', saves=self.saves)
        self.files = {smoke.device.REMOTE: b'original command', smoke.TELEMETRY: b'original telemetry'}
        self.wake = b'0'
        self.calls = []
        self.fail = None

    def adb(self, *args, **kwargs):
        self.calls.append(args)
        output = b''
        if args[:2] == ('exec-out', 'cat'):
            output = self.files[args[2]]
        elif args[:3] == ('shell', 'rm', '-f'):
            self.files.pop(args[3], None)
        elif args[0] == 'push':
            self.files[args[2]] = Path(args[1]).read_bytes()
        elif args[:3] == ('shell', 'settings', 'put'):
            self.wake = args[-1].encode()
        elif args[:3] == ('shell', 'settings', 'get'):
            output = self.wake
        return SimpleNamespace(stdout=output, returncode=0)

    def mocked(self):
        with patch.object(smoke.device, 'ROOT', self.root), patch.object(smoke.device, 'preflight', return_value=copy.deepcopy(self.before)), patch.object(smoke.device, 'save_state', return_value=self.saves), patch.object(smoke.device, 'backup_saves'), patch.object(smoke.device, 'exists', side_effect=lambda path: path in self.files), patch.object(smoke.device, 'adb', side_effect=self.adb), patch.object(smoke, 'observe', side_effect=self.fail, return_value=2):
            return smoke.run(self.args)

    def test_restoration_and_smoke_only_receipt(self):
        out = self.mocked()
        result = json.loads((out / 'result.json').read_text())
        self.assertTrue(result['smokeCompleted'])
        self.assertFalse(result['gameplayValidated'])
        self.assertFalse(result['serviceCompletionValidated'])
        self.assertEqual(self.wake, b'0')
        self.assertEqual(self.files[smoke.device.REMOTE], b'original command')
        self.assertEqual(self.files[smoke.TELEMETRY], b'original telemetry')
        actions = [call[2] for call in self.calls if call[:2] == ('shell', 'am')]
        self.assertEqual(actions, ['force-stop', 'start', 'force-stop'])

    def test_failure_restores_everything(self):
        self.fail = RuntimeError('wrong identity')
        with self.assertRaisesRegex(RuntimeError, 'wrong identity'):
            self.mocked()
        self.assertEqual(self.wake, b'0')
        self.assertEqual(self.files[smoke.device.REMOTE], b'original command')
        self.assertEqual(self.files[smoke.TELEMETRY], b'original telemetry')

    def test_wrong_apk_refused(self):
        self.before['installedApkSha256'] = 'wrong'
        with self.assertRaisesRegex(RuntimeError, 'differs'):
            self.mocked()
        self.assertEqual(self.calls, [])

    def test_wrong_map_refused(self):
        self.args.map = '/Game/Other'
        with self.assertRaisesRegex(ValueError, 'isolated'):
            self.mocked()
        self.assertEqual(self.calls, [])

    def test_duration_is_bounded(self):
        self.args.duration = 181
        with self.assertRaisesRegex(ValueError, '60..180'):
            self.mocked()
        self.assertEqual(self.calls, [])

    def test_wrong_identity_rejected(self):
        for key in ('content', 'service'):
            state = telemetry()
            state[key] = 'wrong' if key == 'content' else {'serviceId': 'wrong'}
            with self.assertRaisesRegex(RuntimeError, 'identity'):
                smoke.validate(state)

    def test_missing_station_or_traffic_cannot_pass_readiness(self):
        self.assertTrue(smoke.validate(telemetry()))
        for key in ('egmoreScene', 'trafficLocomotive', 'locomotive'):
            state = telemetry()
            state.pop(key)
            self.assertFalse(smoke.validate(state))

    def test_paused_or_invalid_snapshot_cannot_pass(self):
        for key, value in [('paused', True), ('logicalFingerprint', 'INVALID')]:
            state = telemetry()
            state[key] = value
            self.assertFalse(smoke.validate(state))

    def test_stale_log_cannot_validate_telemetry(self):
        raw = json.dumps(telemetry()).encode()
        proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        with patch.object(smoke.device, 'exists', return_value=True), patch.object(smoke.device, 'adb', side_effect=[SimpleNamespace(stdout=raw), SimpleNamespace(stdout=b'old-token\n' + proof)]), patch.object(smoke.device, 'screenshot') as screenshot, patch.object(smoke.time, 'monotonic', side_effect=[0, 1, 100]), patch.object(smoke.time, 'sleep'):
            with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                smoke.observe(self.root, 'new-token', 60)
            screenshot.assert_not_called()

    def test_missing_fresh_telemetry_times_out(self):
        with patch.object(smoke.device, 'exists', return_value=False), patch.object(smoke.device, 'adb') as adb, patch.object(smoke.time, 'monotonic', side_effect=[0, 1, 100]), patch.object(smoke.time, 'sleep'):
            with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                smoke.observe(self.root, 'token', 60)
            adb.assert_not_called()


if __name__ == '__main__':
    unittest.main()
