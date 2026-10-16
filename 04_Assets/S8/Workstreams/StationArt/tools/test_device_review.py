"""No-phone tests of capture identity and restoration on failure."""
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import device_review as review
import io
from PIL import Image


class CaptureSafety(unittest.TestCase):
    def test_save_inventory_covers_egmore_and_absent_slots(self):
        present = f'{review.SAVED}/S8/Egmore/Service/checkpoint-0.irs'
        with patch.object(review, 'exists', side_effect=lambda path: path == present), patch.object(review, 'adb', return_value=SimpleNamespace(stdout=b'egmore-save')):
            state = review.save_state()
        self.assertEqual(len(state), 8)
        self.assertEqual(state[present], review.sha(b'egmore-save'))
        self.assertEqual(sum(value is None for value in state.values()), 7)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.out = Path(self.temp.name)
        self.apk = self.out / 'candidate.apk'
        self.apk.write_bytes(b'fixture')
        self.args = SimpleNamespace(apk=str(self.apk), map='/Game/IndianRailway/S8/Review/Test', flags='')
        self.before = dict(installedApkSha256=review.sha(b'fixture'), wake='2', saves={})
        self.calls = []
        self.original = b'original\r\ncommandline\x00'
        self.remote = self.original
        self.wake = b'2'
        self.fail_wake = False
        self.focus = b'mCurrentFocus=com.indianrailway.s1harness'
        self.offline = False
        self.log = b'LogWorld: Bringing World /Game/IndianRailway/S8/Review/Test.Test up for play'
        buffer = io.BytesIO()
        Image.new('RGB', (2, 2)).save(buffer, format='PNG')
        self.png = buffer.getvalue()

    def adb(self, *args, check=True):
        self.calls.append(args)
        output = b''
        code = 0
        if args[0] == 'shell' and args[1].startswith('if [ -f '):
            if self.offline:
                return SimpleNamespace(stdout=b'', returncode=1)
            present = self.remote is not None if review.REMOTE in args[1] else False
            output = b'EXISTS\n' if present else b'ABSENT\n'
        elif args[:2] == ('exec-out', 'cat'):
            output = self.remote if args[2] == review.REMOTE else self.log
        elif args[:2] == ('exec-out', 'screencap'):
            output = self.png
        elif args[0] == 'push':
            self.remote = Path(args[1]).read_bytes()
        elif args[:3] == ('shell', 'rm', '-f'):
            self.remote = None
        elif args[:3] == ('shell', 'settings', 'put'):
            if self.fail_wake and args[-1] == '3':
                raise RuntimeError('injected wake failure')
            self.wake = args[-1].encode()
        elif args[:3] == ('shell', 'settings', 'get'):
            output = self.wake
        elif args[:3] == ('shell', 'dumpsys', 'window'):
            output = self.focus
        return SimpleNamespace(stdout=output, returncode=code)

    def run_capture(self, sleep_error=None):
        with patch.object(review, 'adb', self.adb), patch.object(review, 'save_state', return_value={}), patch.object(review.time, 'sleep', side_effect=sleep_error):
            review.capture(self.args, self.out, self.before)

    def test_wrong_apk_never_mutates(self):
        self.before['installedApkSha256'] = 'wrong'
        with self.assertRaisesRegex(RuntimeError, 'differs'):
            self.run_capture()
        self.assertEqual(self.calls, [])

    def test_normal_map_never_mutates(self):
        self.args.map = '/Game/IndianRailway/S7/L_RailService'
        with self.assertRaisesRegex(RuntimeError, 'isolated'):
            self.run_capture()
        self.assertEqual(self.calls, [])

    def test_success_restores_original_bytes(self):
        self.run_capture()
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_capture_exception_restores(self):
        with self.assertRaisesRegex(RuntimeError, 'injected'):
            self.run_capture(RuntimeError('injected capture failure'))
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_early_wake_failure_restores(self):
        self.fail_wake = True
        with self.assertRaisesRegex(RuntimeError, 'injected'):
            self.run_capture()
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_missing_original_restored_to_missing(self):
        self.remote = None
        self.run_capture()
        self.assertIsNone(self.remote)

    def test_other_app_never_captured(self):
        self.focus = b'mCurrentFocus=some.other.app'
        with self.assertRaisesRegex(RuntimeError, 'foreground'):
            self.run_capture()
        self.assertNotIn(('exec-out', 'screencap', '-p'), self.calls)
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_changed_save_fails_verification(self):
        with patch.object(review, 'adb', self.adb), patch.object(review, 'save_state', side_effect=[{}, {'changed': 'hash'}]), patch.object(review.time, 'sleep'):
            with self.assertRaisesRegex(RuntimeError, 'Restoration verification'):
                review.capture(self.args, self.out, self.before)
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_offline_existence_refuses_mutation(self):
        self.offline = True
        with self.assertRaisesRegex(RuntimeError, 'existence'):
            self.run_capture()
        self.assertFalse(any(call[0] == 'push' for call in self.calls))
        self.assertEqual(self.remote, self.original)

    def test_empty_png_rejected_and_restored(self):
        self.png = b''
        with self.assertRaises(Exception):
            self.run_capture()
        self.assertEqual(self.remote, self.original)
        self.assertEqual(self.wake, b'2')

    def test_commandline_echo_is_not_world_proof(self):
        self.log = b'Command line: /Game/IndianRailway/S8/Review/Test'
        with self.assertRaisesRegex(RuntimeError, 'load proof'):
            self.run_capture()
        self.assertEqual(self.remote, self.original)

    def test_preserves_current_zero_wake(self):
        self.before['wake'] = '0'
        self.wake = b'0'
        self.run_capture()
        self.assertEqual(self.wake, b'0')


if __name__ == '__main__':
    unittest.main()
