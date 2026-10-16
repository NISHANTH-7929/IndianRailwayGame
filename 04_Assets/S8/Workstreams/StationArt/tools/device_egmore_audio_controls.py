"""Observed-layout audio interactions in a fresh isolated AudioReview service.

Uses the existing screenshot/action-file protocol; does not resume or overwrite
normal checkpoints. Engine logs are evidence of playback state, not listening QA.
"""
import argparse
import device_egmore_controls as controls


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    smoke = controls.smoke
    smoke.FLAGS += ' -S8AudioReview'
    smoke.TELEMETRY = smoke.device.SAVED + '/S8/AudioReview/telemetry.json'
    args.map, args.duration, args.cab = smoke.WORLD, 600, True
    smoke.run(args, exercise=controls.exercise)
