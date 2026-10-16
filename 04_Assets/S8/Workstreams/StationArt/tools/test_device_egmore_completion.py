import json
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import device_egmore_completion as completion
from test_device_egmore_service import telemetry


class ItinerarySafety(unittest.TestCase):
    def test_background_ready_stage_never_touches(self):
        state = telemetry()
        state['service'].update(station='ReadyToDepart', phase='Running', ticks=4000)
        proof = f'token\nLogWorld: Bringing World {completion.smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        with tempfile.TemporaryDirectory() as temp, patch.object(completion.device, 'exists', return_value=True), patch.object(completion.device, 'adb', side_effect=[SimpleNamespace(stdout=json.dumps(state).encode()), SimpleNamespace(stdout=proof), SimpleNamespace(stdout=b'mCurrentFocus=other.app')]) as adb, patch.object(completion.device, 'screenshot') as screenshot, patch.object(completion.time, 'sleep'), patch.object(completion.time, 'monotonic', side_effect=[0, 1]):
            with self.assertRaisesRegex(RuntimeError, 'foreground'):
                completion.itinerary(Path(temp), 'token', 600)
            screenshot.assert_not_called()
            self.assertFalse(any('input' in call.args for call in adb.call_args_list))

    def test_completed_without_depart_proof_is_rejected(self):
        state = telemetry()
        state['paused'] = True
        state['service'].update(station='Completed', phase='Result', success=True, departureAuthorized=True, dwellTicks=300)
        proof = f'token\nLogWorld: Bringing World {completion.smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        with tempfile.TemporaryDirectory() as temp, patch.object(completion.device, 'exists', return_value=True), patch.object(completion.device, 'adb', side_effect=[SimpleNamespace(stdout=json.dumps(state).encode()), SimpleNamespace(stdout=proof), SimpleNamespace(stdout=b'mCurrentFocus=' + completion.device.PACKAGE.encode())]), patch.object(completion.device, 'screenshot', return_value=b'validated PNG'), patch.object(completion.time, 'sleep'), patch.object(completion.time, 'monotonic', side_effect=[0, 1]):
            with self.assertRaisesRegex(RuntimeError, 'required service'):
                completion.itinerary(Path(temp), 'token', 600)


if __name__ == '__main__':
    unittest.main()
