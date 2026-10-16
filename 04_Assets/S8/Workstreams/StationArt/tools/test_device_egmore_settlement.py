import copy
import unittest
from device_egmore_settlement import settlement_checks


class SettlementChecks(unittest.TestCase):
    def setUp(self):
        self.before = dict(generation=1, service=dict(success=True, settled=False, phase='Result',
            settlementId='S8-Egmore-01/1', run=1,
            profile=dict(awards=0, reputation=0, lastSettledRun=0, nextRun=2)))
        self.after = copy.deepcopy(self.before)
        self.after['generation'] = 2
        self.after['service']['settled'] = True
        self.after['service']['profile'].update(awards=1, reputation=10, lastSettledRun=1)

    def test_exactly_one_reward(self):
        self.assertTrue(all(settlement_checks(self.before, self.after).values()))

    def test_duplicate_reward_rejected(self):
        self.after['service']['profile']['awards'] = 2
        self.assertFalse(settlement_checks(self.before, self.after)['awards'])

    def test_unsaved_state_rejected(self):
        self.after['generation'] = 1
        self.assertFalse(settlement_checks(self.before, self.after)['generation'])

    def test_wrong_service_rejected(self):
        self.after['service']['settlementId'] = 'other/1'
        self.assertFalse(settlement_checks(self.before, self.after)['identity'])
