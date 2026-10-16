"""Inspect actual PCM mixer output; does not infer subjective sound quality."""
import argparse
import hashlib
import json
from pathlib import Path
import wave
import numpy as np

p = argparse.ArgumentParser()
p.add_argument('wav', type=Path)
a = p.parse_args()
with wave.open(str(a.wav), 'rb') as w:
    channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
    assert width == 2, f'Expected PCM16, got {width}'
    samples = np.frombuffer(w.readframes(frames), dtype='<i2').astype(np.float64).reshape(-1, channels) / 32768
assert frames > rate and channels in (1, 2) and np.isfinite(samples).all()
peak = float(np.max(np.abs(samples)))
clipped = int(np.count_nonzero(np.abs(samples) >= .999))
seconds = []
for begin in range(0, frames, rate):
    block = samples[begin:begin + rate]
    seconds.append(dict(second=begin / rate, rms=float(np.sqrt(np.mean(block**2))), peak=float(np.max(np.abs(block)))))
report = dict(file=str(a.wav.resolve()), sha256=hashlib.sha256(a.wav.read_bytes()).hexdigest(),
              durationSeconds=frames/rate, channels=channels, sampleRate=rate,
              peak=peak, clippedSamples=clipped, rms=float(np.sqrt(np.mean(samples**2))),
              dcOffset=float(np.mean(samples)), seconds=seconds,
              passed=bool(.001 < peak < .999 and clipped == 0),
              scope='Native mixer PCM integrity and non-silence only; not subjective or prototype acoustic acceptance.')
a.wav.with_suffix('.audit.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(json.dumps({k:v for k,v in report.items() if k != 'seconds'}, indent=2))
raise SystemExit(0 if report['passed'] else 1)
