"""S10 re-cut of the authorized Pixabay recordings (originals in Pixabay_20261009/Originals).

Never modifies originals. Removes events that would contradict the simulation: the station
ambience loop keeps only crowd chatter (its embedded horn at 18 s and train arrival at 25-33 s
would otherwise sound on every loop), and horns are high-passed above the recording's idling
engine rumble. The player horn is a sustain loop so the horn sounds exactly as long as it is held.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import wave
import numpy as np

ROOT = Path(__file__).resolve().parent
AUDIO = ROOT.parents[1]
ORIGINALS = AUDIO / 'Pixabay_20261009' / 'Originals'
RATE = 48000
HORN = 'tanweraman-train-horn-337875.mp3'
STATION = 'arunangshubanerjee-indian-railway-station-ambience-crowd-chatter-and-train-arrival-331012.mp3'
CLIPS = [
    # name, source, start, end, highpass Hz, looping, loop crossfade s, peak
    ('Wap7HornLoop', HORN, 5.60, 10.40, 300, True, .40, .50, 'Sustain of a twin-tone Indian locomotive horn blast (390/482 Hz family); held horn in the player cab.'),
    ('TrafficHorn', HORN, 14.15, 20.25, 300, False, 0, .50, 'Separate long blast from the same session; approaching traffic trains (one-shot with natural decay).'),
    ('StationChatter', STATION, 0.20, 17.60, 60, True, 1.0, .35, 'Platform crowd chatter only; the recording\'s horn (18 s) and train arrival (25-33 s) are excluded.'),
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode(path, highpass):
    chain = f'highpass=f={highpass},highpass=f={highpass}' if highpass else 'anull'
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', str(path), '-ac', '1', '-ar', str(RATE), '-af', chain, '-f', 'f32le', '-'],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype='<f4').astype(np.float64)


def main():
    out = ROOT / 'Prepared'
    out.mkdir(exist_ok=True)
    records = []
    for name, source, start, end, highpass, looping, crossfade, peak, use in CLIPS:
        original = ORIGINALS / source
        x = decode(original, highpass)[int(start * RATE):int(end * RATE)]
        x = x - np.mean(x)
        if looping:
            # Equal-power crossfade of the tail into the head: the loop boundary is the faded join.
            n = int(crossfade * RATE)
            ramp = np.linspace(0, np.pi / 2, n)
            x = np.concatenate([x[n:-n], x[-n:] * np.cos(ramp) + x[:n] * np.sin(ramp)])
        else:
            a, r = int(.02 * RATE), int(.15 * RATE)
            x[:a] *= np.linspace(0, 1, a)
            x[-r:] *= np.linspace(1, 0, r) ** 2
        x *= peak / np.max(np.abs(x))
        path = out / f'SW_{name}.wav'
        with wave.open(str(path), 'wb') as w:
            w.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
            w.writeframes(np.round(np.clip(x, -1, 1) * 32767).astype('<i2').tobytes())
        rms = float(np.sqrt(np.mean(x * x)))
        seam = float(abs(x[-1] - x[0]))
        assert np.isfinite(x).all() and rms > .01 and (not looping or seam < .05), (name, rms, seam)
        records.append(dict(name=name, use=use, source=f'05_Audio/Pixabay_20261009/Originals/{source}', sourceSha256=digest(original),
                            trimSeconds=[start, end], highpassHz=highpass, looping=looping, crossfadeSeconds=crossfade,
                            prepared=f'Prepared/SW_{name}.wav', preparedSha256=digest(path), durationSeconds=len(x) / RATE,
                            sampleRate=RATE, channels=1, peak=float(np.max(np.abs(x))), rms=rms, seamDelta=seam,
                            asset=f'/Game/IndianRailway/S10/AudioV1/SW_{name}', license='Pixabay Content License (see 05_Audio/SOURCE_REGISTER.md)',
                            authenticity='Indian railway field recording; locomotive class unverified'))
    (ROOT / 'manifest.json').write_text(json.dumps(dict(schema=1, clips=records), indent=2) + '\n', encoding='utf-8')
    print(json.dumps([dict(name=r['name'], duration=round(r['durationSeconds'], 2), rms=round(r['rms'], 4), seam=round(r['seamDelta'], 4)) for r in records], indent=2))


if __name__ == '__main__':
    main()
