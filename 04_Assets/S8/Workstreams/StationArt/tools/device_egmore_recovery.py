"""Paused cold recovery of existing isolated Egmore checkpoints; no save creation."""
import argparse
import json
import device_review as device
import device_cold_recovery as recovery


def run(args):
    # Add test slots to every preflight, backup and final preservation check.
    family = 'S8/Egmore/AssetReview'
    if family not in device.SAVE_FAMILIES:
        device.SAVE_FAMILIES += (family,)
    before = device.preflight()
    for slot in range(2):
        if before['saves'].get(f'{device.SAVED}/{family}/checkpoint-{slot}.irs') is None:
            raise RuntimeError('Both existing isolated checkpoints are required; no mutation')
    remote = f'{device.SAVED}/{family}/last-checkpoint.json'
    raw = device.adb('exec-out', 'cat', remote).stdout
    reference = json.loads(raw.decode('utf-8-sig'))
    if (reference.get('content') != 'IR-S8-EGMORE-1'
            or reference.get('logicalFingerprint') in (None, '', 'INVALID')
            or reference.get('generation', 0) <= 0):
        raise RuntimeError('No valid isolated Egmore checkpoint reference; no mutation')
    case = ('egmore', family, '/Game/IndianRailway/S8/Egmore/L_RailService_Egmore',
            '-S8Egmore -S8Station -S8LHB -S8AssetReview -S8AssetReviewResume',
            'pre-existing isolated last-checkpoint.json')
    # run() verifies installed APK before mutations, backs up all ten paths,
    # restores the override/wake/telemetry and verifies every checkpoint hash.
    out = device.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device' / args.label
    if out.exists():
        raise RuntimeError('Evidence label already exists; no mutation')
    try:
        recovery.run(args, cases=(case,), references=[reference])
    finally:
        if out.is_dir():
            (out / 'reference-last-checkpoint.json').write_bytes(raw)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', required=True)
    parser.add_argument('--label', required=True)
    run(parser.parse_args())
