import copy
import unittest
from device_cold_recovery import validate


class EgmoreRecoveryValidation(unittest.TestCase):
    def setUp(self):
        self.state = dict(content='IR-S8-EGMORE-1', logicalFingerprint='abc',
                          service=dict(serviceId='S8-Egmore-01'), generation=9,
                          paused=True, blocked=False, logical=dict(valid=True, faulted=False),
                          visualVehicles=0, loadedLevels=2,
                          egmoreScene=dict(ready=True, sceneRebuilt=True, trackExtension=True,
                                           modules=39, placements=2594,
                                           signals=[dict(id=1, aspect='stop'), dict(id=2, aspect='proceed')]),
                          locomotive=dict(ready=True), trafficLocomotive=dict(ready=True))
        self.reference = copy.deepcopy(self.state)

    def test_exact_paused_recovery(self):
        self.assertTrue(all(validate(self.state, self.reference, 'egmore').values()))

    def test_generation_mismatch_rejected(self):
        self.state['generation'] += 1
        self.assertFalse(validate(self.state, self.reference, 'egmore')['generation'])

    def test_resumed_simulation_rejected(self):
        self.state['paused'] = False
        self.assertFalse(validate(self.state, self.reference, 'egmore')['paused'])

    def test_incomplete_scene_rejected(self):
        self.state['egmoreScene']['placements'] = 1316
        self.assertFalse(validate(self.state, self.reference, 'egmore')['egmore_ready'])

    def test_duplicate_signal_ids_rejected(self):
        self.state['egmoreScene']['signals'][1]['id'] = 1
        self.assertFalse(validate(self.state, self.reference, 'egmore')['signals_ready'])

    def test_traffic_presentation_required(self):
        self.state['trafficLocomotive']['ready'] = False
        self.assertFalse(validate(self.state, self.reference, 'egmore')['locomotives_ready'])

    def test_service_state_must_match(self):
        self.state['service']['settled'] = True
        self.assertFalse(validate(self.state, self.reference, 'egmore')['service'])
