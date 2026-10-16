"""Validate a real simulated opposing encounter, not a synthetic source sweep."""
import argparse
import array
import json
import math
import re
from pathlib import Path
import sys
import wave


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    parser.add_argument("--require-audio", action="store_true")
    args = parser.parse_args()
    folder = args.run / "native" if (args.run / "native").is_dir() else args.run
    summary = json.loads((folder / "passing-service.json").read_text(encoding="utf-8-sig"))
    samples = [json.loads(line) for line in (folder / "passing-service-samples.jsonl").read_text(encoding="utf-8-sig").splitlines() if line.strip()]
    checks = {
        "runtime_pass": summary["passed"] is True,
        "actual_train_three": summary["trainId"] == 3 and summary["synthetic"] is False,
        "samples_present": len(samples) >= 20,
        "approach_then_departure": bool(samples) and samples[0]["signedMetres"] > 500 and samples[-1]["signedMetres"] < -500,
        "opposite_direction": bool(samples) and all(row["forwardDot"] < -.8 for row in samples),
        "moving_ai": bool(samples) and all(abs(row["opposingSpeedMps"]) > .5 for row in samples),
        "close_encounter": bool(samples) and min(row["distanceMetres"] for row in samples) < 50,
        "three_rendered_views": all(any(folder.glob(f"*-{label}.png")) for label in ("opposing-approach", "opposing-pass", "opposing-departed")),
        "monotonic_ticks": all(a["tick"] <= b["tick"] for a, b in zip(samples, samples[1:])),
    }
    audio = None
    if args.require_audio:
        log = (args.run / 'engine.log').read_text(errors='replace')
        checks['opposing_horn_triggered'] = bool(re.search(r'RAIL_AUDIO traffic approach horn distance=[\d.]+m opposing=1', log))
        checks["audio_export_ready"] = summary["audioExportReady"] is True
        with wave.open(str(folder / "passing-service-audio.wav"), "rb") as wav:
            channels, width, rate, frames = wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()
            if width != 2:
                raise ValueError(f"Expected PCM16, got sample width {width}")
            pcm = array.array("h", wav.readframes(frames))
            if sys.byteorder != "little":
                pcm.byteswap()
        peak = max((abs(value) for value in pcm), default=0) / 32768
        rms = math.sqrt(sum((value / 32768) ** 2 for value in pcm) / max(1, len(pcm)))
        duration = frames / rate
        window = json.loads((folder/'passing-recording-window.json').read_text(encoding='utf-8-sig'))
        observed = window['wallSeconds']
        audio = dict(seconds=duration, peak=peak, rms=rms, channels=channels, observedEncounterSeconds=observed)
        checks["audible_not_clipped"] = .001 < rms and peak < .999
        checks["stereo_recording"] = channels == 2
        checks["real_time_encounter"] = observed > 30 and abs(duration - observed) < 5
        checks['normal_simulation_pace'] = abs(window['simulationSeconds']-observed) < 5
        checks['complete_recording_window'] = window['afterStreamingReload'] and window['startSignedMetres'] > 550 and window['endSignedMetres'] < -1200
    report = {"passed": all(checks.values()), "checks": checks, "audio": audio,
              "limitations": "Real simulated encounter with player/AI mix. Does not establish subjective authenticity or measured speaker loudness."}
    (args.run / "opposing-service-audit.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
