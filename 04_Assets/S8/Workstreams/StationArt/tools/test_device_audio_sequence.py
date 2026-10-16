import io
import json
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from PIL import Image
import device_egmore_audio_sequence as sequence


class AudioSequenceTests(unittest.TestCase):
    def run_sequence(self, size=(2800, 1260), paused=True, horn=True):
        png = io.BytesIO()
        Image.new('RGB', size).save(png, format='PNG')
        states = iter([False, False, paused, False, False])
        touches = []
        log = ('token\n' + ('RAIL_AUDIO horn\nhorn=1\n' if horn else '')
               + 'RAIL_AUDIO_STATE running=0 rail=0 station=0 horn=0 brake=0\n'
               + 'RAIL_AUDIO_STATE running=1 rail=1 station=1 horn=0 brake=1\n'
               + 'RAIL_AUDIO brake engaged=1\n')

        def adb(*args):
            if args[:3] == ('shell', 'dumpsys', 'window'):
                data = ('mCurrentFocus ' + sequence.smoke.device.PACKAGE).encode()
            elif args[:3] == ('shell', 'input', 'tap'):
                touches.append(args[3:])
                data = b''
            elif args[-1].endswith('MyProject.log'):
                data = log.encode()
            else:
                data = json.dumps(dict(content='IR-S8-EGMORE-1', blocked=False,
                    service=dict(phase='Running'), logical=dict(valid=True, faulted=False),
                    locomotive=dict(cabCamera=True), paused=next(states))).encode()
            return SimpleNamespace(stdout=data)

        with TemporaryDirectory() as folder, patch.object(sequence.smoke, 'observe', return_value=3), \
                patch.object(sequence.smoke.device, 'adb', side_effect=adb), \
                patch.object(sequence.smoke.device, 'screenshot', return_value=png.getvalue()), \
                patch.object(sequence.time, 'sleep'):
            result = sequence.exercise(Path(folder), 'token', 150)
            self.assertTrue(json.loads((Path(folder) / 'audio-sequence-checks.json').read_text())['passed'])
            return result, touches

    def test_happy_path(self):
        count, touches = self.run_sequence()
        self.assertEqual(count, 8)
        self.assertEqual(touches, [('190', '1180'), ('880', '1180'), ('1400', '537'), ('1570', '1180')])

    def test_wrong_layout_refused(self):
        with self.assertRaisesRegex(RuntimeError, 'layout size changed'):
            self.run_sequence(size=(100, 100))

    def test_pause_mismatch_refused(self):
        with self.assertRaisesRegex(RuntimeError, 'Unexpected pause state'):
            self.run_sequence(paused=False)

    def test_missing_horn_fails(self):
        with self.assertRaisesRegex(RuntimeError, 'Audio sequence check failed'):
            self.run_sequence(horn=False)


if __name__ == '__main__':
    unittest.main()
