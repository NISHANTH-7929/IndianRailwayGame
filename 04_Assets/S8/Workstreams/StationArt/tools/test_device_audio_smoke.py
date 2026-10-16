import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import device_egmore_audio_smoke as audio


class AudioSmoke(unittest.TestCase):
    def check(self, log, passed):
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary)
            (out / 'engine.log').write_text(log)
            with patch.object(audio.smoke, 'observe', return_value=3):
                if passed:
                    self.assertEqual(audio.exercise(out, 'marker', 90), 3)
                else:
                    with self.assertRaises(RuntimeError):
                        audio.exercise(out, 'marker', 90)

    def log(self):
        return 'marker\n' + '\n'.join('RAIL_AUDIO loaded ' + name + '.' + name
            for name in ('SW_Rail', 'SW_Station', 'SW_Horn', 'SW_Brake', 'SW_StationCue')) + '\nRAIL_AUDIO_STATE running=1 rail=1 station=1 horn=0 brake=0'

    def test_loaded_playing(self):
        self.check(self.log(), True)

    def test_station_not_playing(self):
        self.check(self.log().replace('station=1', 'station=0'), False)

    def test_missing_asset(self):
        self.check(self.log() + '\nRAIL_AUDIO missing SW_Rail', False)

    def test_stale_playback_excluded(self):
        self.check(self.log() + '\nmarker', False)
