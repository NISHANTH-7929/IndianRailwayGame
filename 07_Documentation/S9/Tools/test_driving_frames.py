import csv
import tempfile
import unittest
from pathlib import Path
from summarize_driving_frames import summarize


class DrivingFramesTests(unittest.TestCase):
    def capture(self, rows):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        p = Path(folder.name) / 'capture.csv'
        with p.open('w', newline='') as f:
            csv.writer(f).writerows(rows)
        return p

    def test_loading_hitch_included_menu_excluded(self):
        h = ['EVENTS', 'FrameTime', 'RailS7/DrivingFrame', 'RailS7/PresentationBlocked', 'RailScenery/GameThread/BuildCellMs']
        p = self.capture([h, ['', 9000, 0, 0, 0], ['', 33, 1, 0, 0], ['', 220, 1, 1, 180], h, ['[HasHeaderRowAtEnd]']])
        s = summarize(p)
        self.assertEqual(s['drivingFrames'], 2)
        self.assertEqual(s['excludedFrames'], 1)
        self.assertEqual(s['metrics']['FrameTime']['maxMs'], 220)
        self.assertEqual(s['metrics']['FrameTime']['over100ms'], 1)
        self.assertEqual(s['hitchRows'][0]['RailS7/PresentationBlocked'], 1)
        self.assertEqual(s['metrics']['RailScenery/GameThread/BuildCellMs']['maxMs'], 180)

    def test_unfinished_and_legacy_rejected(self):
        for rows in [[['EVENTS', 'FrameTime', 'RailS7/DrivingFrame'], ['', 33, 1]],
                     [['EVENTS', 'FrameTime'], ['', 33], ['[HasHeaderRowAtEnd]']]]:
            with self.assertRaises(ValueError): summarize(self.capture(rows))

    def test_late_timing_columns_are_zero_before_first_event(self):
        h = ['EVENTS', 'FrameTime', 'RailS7/DrivingFrame']
        final = h + ['RailS7/GameThread/CheckpointMs']
        s = summarize(self.capture([h, ['', 33, 1], ['', 60, 1, 40], final, ['[HasHeaderRowAtEnd]']]))
        self.assertEqual(s['metrics']['RailS7/GameThread/CheckpointMs']['meanMs'], 20)

    def test_invalid_or_empty_driving_rejected(self):
        for value, marker in [('nan', 1), (33, 0), (33, 'bad'), (-1, 1)]:
            with self.assertRaises(ValueError):
                summarize(self.capture([['EVENTS','FrameTime','RailS7/DrivingFrame'], ['',value,marker], ['[HasHeaderRowAtEnd]']]))


if __name__ == '__main__': unittest.main()
