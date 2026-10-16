"""Read-only phone verification following interrupted stabilization updates."""
import json
import device_review as d

out = d.ROOT / '07_Documentation/S8/Evidence/Stabilization-20261010'
baseline = d.ROOT / '07_Documentation/S8/Workstreams/StationArt/Device/oct10-stabilization-install1'
before = json.loads((baseline / 'before-stopped.json').read_text())
after = d.preflight()
commandline = d.adb('exec-out', 'cat', d.REMOTE).stdout
checks = dict(oldApkUnchanged=after['installedApkSha256'] == before['installedApkSha256'],
              eightSaveSlotsUnchanged=after['saves'] == before['saves'],
              wakeUnchanged=after['wake'] == before['wake'],
              commandlineUnchanged=commandline == (baseline / 'original-commandline.txt').read_bytes())
result = dict(passed=all(checks.values()), checks=checks, after=after,
              newBuildInstalled=False, phoneAcceptance=False)
(out / 'failed-install-preservation.json').write_text(json.dumps(result, indent=2))
print(json.dumps(result, indent=2))
raise SystemExit(0 if result['passed'] else 1)
