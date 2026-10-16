"""Original deterministic local synthesis; no samples, provider, or network required."""
import hashlib
import json
from pathlib import Path
import wave
import numpy as np

ROOT = Path(__file__).resolve().parent
RATE = 48000
SEED = 20261013


def noise(seconds, low, high, seed):
    n = round(seconds * RATE)
    rng = np.random.default_rng(seed)
    frequencies = np.fft.rfftfreq(n, 1 / RATE)
    weights = (1 - np.exp(-(frequencies / low) ** 4)) * np.exp(-(frequencies / high) ** 4)
    weights /= np.sqrt(np.maximum(frequencies, 30))
    spectrum = (rng.standard_normal(len(frequencies)) + 1j * rng.standard_normal(len(frequencies))) * weights
    spectrum[0] = 0
    result = np.fft.irfft(spectrum, n)
    return result / max(np.std(result), 1e-9)


def write(name, samples, looping, description):
    samples = samples - np.mean(samples)
    samples *= .62 / max(np.max(np.abs(samples)), 1e-9)
    if not looping:
        fade = min(2400, len(samples) // 10)
        samples[:fade] *= np.linspace(0, 1, fade)
        samples[-fade:] *= np.linspace(1, 0, fade)
    pcm = np.round(samples * 32767).astype('<i2')
    path = ROOT / 'Prepared' / f'SW_{name}.wav'
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(pcm.tobytes())
    # Verify actual encoded bytes and reject clipping or a conspicuous loop seam.
    with wave.open(str(path), 'rb') as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 2, RATE)
        actual = np.frombuffer(wav.readframes(wav.getnframes()), dtype='<i2').astype(float) / 32768
    peak = float(np.max(np.abs(actual)))
    seam = float(abs(actual[-1] - actual[0]))
    assert .1 < peak < .9 and np.all(np.isfinite(actual))
    assert not looping or seam < .12, (name, seam)
    return dict(name=name, prepared=str(path.relative_to(ROOT)).replace('\\', '/'),
                asset=f'/Game/IndianRailway/S8/Audio/RealismV2/SW_{name}',
                looping=looping, durationSeconds=len(actual) / RATE, sampleRate=RATE,
                channels=1, peak=peak, rms=float(np.sqrt(np.mean(actual ** 2))),
                loopSeamDelta=seam, sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                description=description)


def main():
    t = np.arange(8 * RATE) / RATE
    motor = .7*np.sin(2*np.pi*60*t) + .4*np.sin(2*np.pi*120*t + .2*np.sin(2*np.pi*.5*t))
    motor += .2*np.sin(2*np.pi*420*t + .8*np.sin(2*np.pi*1.5*t))
    motor += .16*noise(8, 55, 1500, SEED)
    coast = noise(8, 90, 4800, SEED+1) * (.8 + .12*np.sin(2*np.pi*.5*t))
    squeal = .6*np.sin(2*np.pi*920*t + 1.3*np.sin(2*np.pi*2*t))
    squeal += .25*np.sin(2*np.pi*1360*t + .7*np.sin(2*np.pi*3*t))
    squeal += .16*noise(8, 600, 4500, SEED+2)
    squeal *= .7 + .15*np.sin(2*np.pi*.75*t)
    tr = np.arange(round(2.6*RATE))/RATE
    release = noise(2.6, 350, 7200, SEED+3) * (1-np.exp(-tr*18))*np.exp(-tr*1.7)
    te = np.arange(round(5.4*RATE))/RATE
    emergency = noise(5.4, 80, 9000, SEED+4)*(1-np.exp(-te*45))*np.exp(-te*.6)
    emergency += .3*noise(5.4, 40, 900, SEED+5)*(1-np.exp(-te*30))*np.exp(-te*.85)
    clips = [write('Motor', motor, True, 'Generic electric traction hum/whine; speed pitch and real traction load gain.'),
             write('Coast', coast, True, 'Generic aerodynamic and unpowered running texture.'),
             write('Squeal', squeal, True, 'Restrained metallic curve/brake-friction approximation; not measured wheel slip.'),
             write('BrakeRelease', release, False, 'Short pneumatic release hiss; no simulated pressure implied.'),
             write('Emergency', emergency, False, 'Longer broadband emergency vent; once per applied emergency latch.')]
    manifest = dict(version=2, origin='Original local procedural audio; no external source samples',
                    generator='generate_audio.py', seed=SEED, numpyVersion=np.__version__,
                    license='CC0-1.0', licenseNote='Original generated supplemental waveforms dedicated to CC0; existing Pixabay sources retain their own license.',
                    prompt='Generic railway supplemental motor, coasting, curve/brake friction, brake release and emergency air sounds.',
                    negativePrompt='No claim of authentic WAP-7 recording, no speech, music, licensed sample reuse or artificial wheel-slip telemetry.',
                    clips=clips)
    (ROOT/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(ok=True, clips=[dict(name=c['name'], peak=c['peak'], rms=c['rms'], seam=c['loopSeamDelta']) for c in clips]), indent=2))


if __name__ == '__main__':
    main()
