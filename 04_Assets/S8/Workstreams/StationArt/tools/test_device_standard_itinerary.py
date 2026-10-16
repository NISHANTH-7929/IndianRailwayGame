import unittest
from device_egmore_standard_itinerary import controls


class StandardControls(unittest.TestCase):
    def test_stationary_acceleration(self):
        self.assertEqual(controls(100, 0, False), (.25, 0))

    def test_dwell_holds_brake(self):
        self.assertEqual(controls(10, 0, True), (0, 1))

    def test_close_target_stops(self):
        self.assertEqual(controls(1, .3, False), (0, 1))

    def test_excess_speed_brakes(self):
        self.assertEqual(controls(5, 2, False), (0, .5))

    def test_cruise_coasts(self):
        self.assertEqual(controls(100, 2.1, False), (0, 0))
