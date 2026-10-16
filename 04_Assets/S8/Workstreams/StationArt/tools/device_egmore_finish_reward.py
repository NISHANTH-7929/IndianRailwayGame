"""Recover an existing completed isolated service, then save its reward once."""
import argparse
import json
import time
import device_egmore_settlement as reward
from device_cold_recovery import validate

smoke, device = reward.smoke, reward.device


def run(args):
    device.preflight()  # Authorized hardware only; no mutation.
    raw = device.adb('exec-out', 'cat', device.SAVED + '/S8/Egmore/AssetReview/last-checkpoint.json').stdout
    reference = json.loads(raw.decode('utf-8-sig'))
    service = reference.get('service', {})
    if (reference.get('content') != 'IR-S8-EGMORE-1' or service.get('serviceId') != 'S8-Egmore-01'
            or service.get('phase') != 'Result' or service.get('success') is not True
            or service.get('settled') is not False):
        raise RuntimeError('Requires existing completed, unsettled isolated Egmore checkpoint')

    def exercise(out, token, duration):
        (out / 'reference-last-checkpoint.json').write_bytes(raw)
        deadline = time.monotonic() + duration - 45
        samples = 0
        proof = f'LogWorld: Bringing World {smoke.WORLD}.L_RailService_Egmore up for play'.encode()
        while time.monotonic() < deadline:
            time.sleep(2)
            if not device.exists(smoke.TELEMETRY):
                continue
            current = device.adb('exec-out', 'cat', smoke.TELEMETRY).stdout
            try:
                state = json.loads(current.decode('utf-8-sig'))
            except (ValueError, UnicodeError):
                continue
            log = device.adb('exec-out', 'cat', device.SAVED + '/Logs/MyProject.log').stdout
            start = log.rfind(token.encode())
            if start < 0 or proof not in log[start:]:
                continue
            if b'S8_ASSET_REVIEW_RESUME isolated=1 recovered=1 blocked=0' not in log[start:]:
                raise RuntimeError('No current-run isolated recovery proof')
            checks = validate(state, reference, 'egmore')
            (out / 'recovery-checks.json').write_text(json.dumps(checks, indent=2))
            (out / 'engine.log').write_bytes(log)
            (out / 'telemetry.json').write_bytes(current)
            if not all(checks[k] for k in ('identity', 'fingerprint', 'service', 'paused', 'valid', 'generation')):
                raise RuntimeError('Completed checkpoint did not restore exactly; no touch')
            if not all(checks.values()):
                samples = 0
                continue
            samples += 1
            if samples >= 2:
                return reward.settle(out, state, samples)
        raise RuntimeError('Completed checkpoint readiness timed out')

    smoke.FLAGS += ' -S8AssetReviewResume'
    args.map, args.duration, args.cab = smoke.WORLD, 150, False
    smoke.run(args, exercise=exercise)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    run(parser.parse_args())
