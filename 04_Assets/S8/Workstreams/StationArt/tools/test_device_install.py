import tempfile
import json
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import device_install as installer


class InstallSafety(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.apk = self.root / 'candidate.apk'
        self.apk.write_bytes(b'fixture')
        self.digest = installer.device.sha(b'fixture')
        self.args = SimpleNamespace(apk=str(self.apk), sha256=self.digest, label='test')
        self.calls = []
        self.install_ok = True
        self.offline = False
        self.fail_stop = False
        self.before = dict(saves={}, wake='2', installedApkSha256='old')
        self.after = dict(saves={}, wake='2', installedApkSha256=self.digest)

    def adb(self, *args, check=True):
        self.calls.append(args)
        if args[0] == 'shell' and args[1].startswith('if [ -f '):
            if self.offline:
                return SimpleNamespace(returncode=1, stdout=b'', stderr=b'device offline')
            return SimpleNamespace(returncode=0, stdout=b'ABSENT\n', stderr=b'')
        if args[:3] == ('shell', 'am', 'force-stop') and self.fail_stop:
            raise RuntimeError('device offline during stop')
        if args[0] == 'install':
            return SimpleNamespace(returncode=0 if self.install_ok else 1,
                                   stdout=b'Success' if self.install_ok else b'', stderr=b'')
        return SimpleNamespace(returncode=0, stdout=b'', stderr=b'')

    def run_install(self):
        with patch.object(installer.device, 'ROOT', self.root), patch.object(installer.device, 'adb', self.adb), patch.object(installer.device, 'preflight', side_effect=[self.before, self.after]), patch.object(installer.device, 'save_state', return_value={}):
            installer.install(self.args)

    def test_wrong_artifact_refused_before_phone(self):
        self.args.sha256 = '0' * 64
        with self.assertRaisesRegex(RuntimeError, 'mismatch'):
            self.run_install()
        self.assertEqual(self.calls, [])

    def test_success_only_replace_installs(self):
        self.run_install()
        self.assertIn(('install', '-r', str(self.apk)), self.calls)
        self.assertFalse(any('uninstall' in c or 'clear' in c for c in self.calls))

    def test_failed_install_never_clears_or_retries(self):
        self.install_ok = False
        with self.assertRaisesRegex(RuntimeError, 'failed'):
            self.run_install()
        self.assertEqual(sum(c[0] == 'install' for c in self.calls), 1)
        self.assertFalse(any('uninstall' in c or 'clear' in c for c in self.calls))
        result = json.loads((self.root / '07_Documentation/S8/Workstreams/StationArt/Device/test/result.json').read_text())
        self.assertEqual(result['verificationStatus'], 'not-run')
        self.assertIsNone(result['installed'])
        self.assertIsNone(result['savesUnchanged'])

    def test_changed_save_blocks_acceptance(self):
        self.after['saves'] = {'changed': 'value'}
        with self.assertRaisesRegex(RuntimeError, 'verification failed'):
            self.run_install()

    def test_offline_existence_never_installs(self):
        self.offline = True
        with self.assertRaisesRegex(RuntimeError, 'existence'):
            self.run_install()
        self.assertFalse(any(call[0] == 'install' for call in self.calls))

    def test_failed_stop_never_installs(self):
        self.fail_stop = True
        with self.assertRaisesRegex(RuntimeError, 'offline during stop'):
            self.run_install()
        self.assertFalse(any(call[0] == 'install' for call in self.calls))


if __name__ == '__main__':
    unittest.main()
