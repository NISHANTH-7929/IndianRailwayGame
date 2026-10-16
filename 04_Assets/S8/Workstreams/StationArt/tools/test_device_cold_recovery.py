"""No-phone regression tests; every device entry point is mocked."""
import copy
import json
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import device_cold_recovery as recovery


def fixture():
    return dict(content='fixture', logicalFingerprint='fingerprint', service={'ticks': 123},
                paused=True, blocked=False, logical={'valid': True, 'faulted': False},
                visualVehicles=1, loadedLevels=2, stationInstances=4216,
                stationComponents=30, lhbVehicles=8, lhbComponents=14, proxyInstances=0)


class Validation(unittest.TestCase):
    def test_canonical_fields(self):
        state = fixture()
        self.assertTrue(all(recovery.validate(state, state, 'station').values()))
        for key, value in [('paused', False), ('blocked', True), ('content', 'wrong'),
                           ('logicalFingerprint', 'wrong'), ('service', {}),
                           ('stationInstances', 0), ('stationComponents', 0),
                           ('lhbVehicles', 0), ('lhbComponents', 0), ('proxyInstances', 1)]:
            changed = copy.deepcopy(state)
            changed[key] = value
            self.assertFalse(all(recovery.validate(changed, state, 'station').values()), key)


class Runner(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.apk = self.root / 'candidate.apk'
        self.apk.write_bytes(b'fixture-apk')
        self.digest = recovery.device.sha(self.apk.read_bytes())
        self.saves = {f'{recovery.device.SAVED}/{case[1]}/checkpoint-{slot}.irs': f'hash-{slot}'
                      for case in recovery.CASES for slot in range(2)}
        self.saves.update({f'{recovery.device.SAVED}/S8/Egmore/Service/checkpoint-{slot}.irs': None
                           for slot in range(2)})
        self.before = dict(wake='0', saves=self.saves, installedApkSha256=self.digest)
        self.files = {recovery.device.REMOTE: b'original command\r\n'}
        self.wake = b'0'
        self.calls = []
        self.fail_case = False
        for case in recovery.CASES:
            path = self.root / '07_Documentation/S8/Evidence' / case[4]
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(fixture()))
        self.args = SimpleNamespace(apk=str(self.apk), label='test-recovery')

    def adb(self, *args, **kwargs):
        self.calls.append(args)
        output = b''
        if args[:2] == ('exec-out', 'cat'):
            output = self.files[args[2]]
        elif args[0] == 'push':
            self.files[args[2]] = Path(args[1]).read_bytes()
        elif args[:3] == ('shell', 'rm', '-f'):
            self.files.pop(args[3], None)
        elif args[:3] == ('shell', 'settings', 'put'):
            self.wake = args[-1].encode()
        elif args[:3] == ('shell', 'settings', 'get'):
            output = self.wake
        return SimpleNamespace(stdout=output, returncode=0)

    def case(self, *args):
        self.files[recovery.device.REMOTE] = b'temporary'
        if self.fail_case:
            raise RuntimeError('injected capture failure')
        return {'passed': True}

    def run_mocked(self):
        with patch.object(recovery.device, 'ROOT', self.root), patch.object(recovery.device, 'preflight', return_value=copy.deepcopy(self.before)), patch.object(recovery.device, 'save_state', return_value=self.saves), patch.object(recovery.device, 'backup_saves'), patch.object(recovery.device, 'exists', side_effect=lambda remote: remote in self.files), patch.object(recovery.device, 'adb', self.adb), patch.object(recovery, 'run_case', side_effect=self.case):
            return recovery.run(self.args)

    def test_success_restores_bytes_zero_wake_and_stops(self):
        out = self.run_mocked()
        self.assertEqual(self.files[recovery.device.REMOTE], b'original command\r\n')
        self.assertEqual(self.wake, b'0')
        self.assertTrue(json.loads((out / 'result.json').read_text())['passed'])
        self.assertFalse(any('start' in call or 'input' in call for call in self.calls))
        self.assertEqual(sum(call[:3] == ('shell', 'am', 'force-stop') for call in self.calls), 2)

    def test_exception_still_restores(self):
        self.fail_case = True
        with self.assertRaisesRegex(RuntimeError, 'injected'):
            self.run_mocked()
        self.assertEqual(self.files[recovery.device.REMOTE], b'original command\r\n')
        self.assertEqual(self.wake, b'0')

    def test_existing_egmore_saves_are_allowed_and_preserved(self):
        self.saves[f'{recovery.device.SAVED}/S8/Egmore/Service/checkpoint-0.irs'] = 'egmore-hash'
        out = self.run_mocked()
        self.assertTrue(json.loads((out / 'result.json').read_text())['savesUnchanged'])

    def test_missing_legacy_save_refuses_launch(self):
        self.saves.pop(next(iter(self.saves)))
        with self.assertRaisesRegex(RuntimeError, 'six existing'):
            self.run_mocked()
        self.assertFalse(any(call[0] == 'push' or 'start' in call or 'settings' in call for call in self.calls))

    def test_wrong_apk_prevents_mutation(self):
        self.before['installedApkSha256'] = 'wrong'
        with self.assertRaisesRegex(RuntimeError, 'differs'):
            self.run_mocked()
        self.assertEqual(self.calls, [])

    def test_missing_commandline_restored_to_absent(self):
        self.files.clear()
        self.run_mocked()
        self.assertNotIn(recovery.device.REMOTE, self.files)

    def test_existing_evidence_refused(self):
        self.run_mocked()
        self.calls.clear()
        with self.assertRaises(FileExistsError):
            self.run_mocked()
        self.assertEqual(self.calls, [])


class CaseFreshness(unittest.TestCase):
    def test_transient_and_persistent_presentation_readiness(self):
        for eventually_ready in (False, True):
            with self.subTest(eventually_ready=eventually_ready), tempfile.TemporaryDirectory() as temp:
                case = recovery.CASES[0]
                remote = recovery.device.SAVED + '/' + case[1] + '/telemetry.json'
                files = {remote: b'original telemetry'}
                launched = False
                samples = 0
                def adb(*args, **kwargs):
                    nonlocal launched, samples
                    output = b''
                    if args[:2] == ('exec-out', 'cat'):
                        if args[2] == remote and launched:
                            samples += 1
                            state = fixture()
                            if not eventually_ready or samples == 1:
                                state.update(blocked=True, loadedLevels=0, visualVehicles=0)
                            output = json.dumps(state).encode()
                        else:
                            output = files[args[2]]
                    elif args[:3] == ('shell', 'rm', '-f'):
                        files.pop(args[3], None)
                    elif args[0] == 'push':
                        files[args[2]] = Path(args[1]).read_bytes()
                    elif args[:3] == ('shell', 'am', 'start'):
                        launched = True
                        files[remote] = b'present'
                        files[recovery.device.SAVED + '/Logs/MyProject.log'] = f'current-run\nLogWorld: Bringing World {case[2]}.{case[2].rsplit("/", 1)[-1]} up for play'.encode()
                    elif args[:3] == ('shell', 'am', 'force-stop'):
                        launched = False
                    elif args[:3] == ('shell', 'dumpsys', 'window'):
                        output = b'mCurrentFocus=' + recovery.device.PACKAGE.encode()
                    return SimpleNamespace(stdout=output, returncode=0)
                out = Path(temp) / 'case'
                with patch.object(recovery.device, 'adb', side_effect=adb), patch.object(recovery.device, 'exists', side_effect=lambda path: path in files), patch.object(recovery.device, 'screenshot', return_value=b'PNG') as capture, patch.object(recovery.uuid, 'uuid4', return_value=SimpleNamespace(hex='current-run')), patch.object(recovery.time, 'monotonic', side_effect=[0, 1, 2, 100]), patch.object(recovery.time, 'sleep'):
                    if eventually_ready:
                        self.assertTrue(recovery.run_case(case, fixture(), out)['passed'])
                        capture.assert_called_once()
                    else:
                        with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                            recovery.run_case(case, fixture(), out)
                        capture.assert_not_called()
                self.assertEqual(samples, 2)
                history = [json.loads(line) for line in (out / 'readiness.jsonl').read_text().splitlines()]
                self.assertEqual(len(history), 1 if eventually_ready else 2)
                self.assertTrue(history[0]['checks']['fingerprint'])
                self.assertFalse(history[0]['checks']['unblocked'])
                self.assertEqual(files[remote], b'original telemetry')

    def test_presentation_readiness_wait_is_bounded(self):
        for becomes_ready in (False, True):
            with self.subTest(becomes_ready=becomes_ready), tempfile.TemporaryDirectory() as temp:
                case = recovery.CASES[0]
                remote = recovery.device.SAVED + '/' + case[1] + '/telemetry.json'
                files = {remote: b'original telemetry'}
                reads = 0
                def adb(*args, **kwargs):
                    nonlocal reads
                    output = b''
                    if args[:2] == ('exec-out', 'cat'):
                        if args[2] == remote and files[remote] != b'original telemetry':
                            reads += 1
                            current = fixture()
                            if reads == 1 or not becomes_ready:
                                current.update(blocked=True, loadedLevels=0, visualVehicles=0)
                            output = json.dumps(current).encode()
                        else:
                            output = files[args[2]]
                    elif args[:3] == ('shell', 'rm', '-f'):
                        files.pop(args[3], None)
                    elif args[0] == 'push':
                        files[args[2]] = Path(args[1]).read_bytes()
                    elif args[:3] == ('shell', 'am', 'start'):
                        files[remote] = b'new telemetry'
                        world = case[2]
                        files[recovery.device.SAVED + '/Logs/MyProject.log'] = ('current-run\n' + f'LogWorld: Bringing World {world}.{world.rsplit("/", 1)[-1]} up for play').encode()
                    elif args[:3] == ('shell', 'dumpsys', 'window'):
                        output = b'mCurrentFocus=' + recovery.device.PACKAGE.encode()
                    return SimpleNamespace(stdout=output, returncode=0)
                with patch.object(recovery.device, 'adb', side_effect=adb), patch.object(recovery.device, 'exists', side_effect=lambda path: path in files), patch.object(recovery.device, 'screenshot', return_value=b'validated PNG'), patch.object(recovery.uuid, 'uuid4', return_value=SimpleNamespace(hex='current-run')), patch.object(recovery.time, 'monotonic', side_effect=[0, 1, 2, 100]), patch.object(recovery.time, 'sleep'):
                    if becomes_ready:
                        self.assertTrue(recovery.run_case(case, fixture(), Path(temp) / 'case')['passed'])
                    else:
                        with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                            recovery.run_case(case, fixture(), Path(temp) / 'case')
                self.assertEqual(files[remote], b'original telemetry')
                self.assertTrue((Path(temp) / 'case/readiness.jsonl').is_file())

    def test_only_current_run_log_accepts_fresh_telemetry(self):
        for fresh in (False, True):
            with self.subTest(fresh=fresh), tempfile.TemporaryDirectory() as temp:
                case = recovery.CASES[0]
                remote = recovery.device.SAVED + '/' + case[1] + '/telemetry.json'
                files = {remote: b'original telemetry'}
                world = case[2]
                def adb(*args, **kwargs):
                    output = b''
                    if args[:2] == ('exec-out', 'cat'):
                        output = files[args[2]]
                    elif args[:3] == ('shell', 'rm', '-f'):
                        files.pop(args[3], None)
                    elif args[0] == 'push':
                        files[args[2]] = Path(args[1]).read_bytes()
                    elif args[:3] == ('shell', 'am', 'start'):
                        files[remote] = json.dumps(fixture()).encode()
                        marker = 'current-run' if fresh else 'previous-run'
                        files[recovery.device.SAVED + '/Logs/MyProject.log'] = (marker + '\n' + f'LogWorld: Bringing World {world}.{world.rsplit("/", 1)[-1]} up for play').encode()
                    elif args[:3] == ('shell', 'dumpsys', 'window'):
                        output = b'mCurrentFocus=' + recovery.device.PACKAGE.encode()
                    return SimpleNamespace(stdout=output, returncode=0)
                with patch.object(recovery.device, 'adb', side_effect=adb), patch.object(recovery.device, 'exists', side_effect=lambda path: path in files), patch.object(recovery.device, 'screenshot', return_value=b'validated PNG'), patch.object(recovery.uuid, 'uuid4', return_value=SimpleNamespace(hex='current-run')), patch.object(recovery.time, 'monotonic', side_effect=[0, 1, 100]), patch.object(recovery.time, 'sleep'):
                    if fresh:
                        result = recovery.run_case(case, fixture(), Path(temp) / 'case')
                        self.assertTrue(result['passed'])
                    else:
                        with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                            recovery.run_case(case, fixture(), Path(temp) / 'case')
                self.assertEqual(files[remote], b'original telemetry')

    def test_stale_log_and_removed_telemetry_cannot_pass(self):
        with tempfile.TemporaryDirectory() as temp:
            case = recovery.CASES[0]
            remote = recovery.device.SAVED + '/' + case[1] + '/telemetry.json'
            files = {remote: b'old telemetry'}
            calls = []
            def adb(*args, **kwargs):
                calls.append(args)
                output = b''
                if args[:2] == ('exec-out', 'cat'):
                    output = files[args[2]]
                elif args[:3] == ('shell', 'rm', '-f'):
                    files.pop(args[3], None)
                elif args[0] == 'push':
                    files[args[2]] = Path(args[1]).read_bytes()
                return SimpleNamespace(stdout=output, returncode=0)
            with patch.object(recovery.device, 'adb', side_effect=adb), patch.object(recovery.device, 'exists', side_effect=lambda path: path in files), patch.object(recovery.time, 'monotonic', side_effect=[0, 1]), patch.object(recovery.time, 'sleep'):
                with self.assertRaisesRegex(RuntimeError, 'No fresh'):
                    recovery.run_case(case, fixture(), Path(temp) / 'case', timeout=0)
            self.assertEqual(files[remote], b'old telemetry')
            self.assertFalse(any('input' in call for call in calls))


if __name__ == '__main__':
    unittest.main()
