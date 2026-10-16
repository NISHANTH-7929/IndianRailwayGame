"""Prepare the authorized Pixabay sample bank; never modify downloaded originals."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import wave
import numpy as np

ROOT = Path(__file__).resolve().parent
BANK = ROOT / 'Pixabay_20261009'
RATE = 48000
SOURCES = [
    ('Rail', 'arunangshubanerjee-indian-train-sound-from-vestibule-realistic-interior-noise-314562.mp3', 'arunangshubanerjee', 'city-indian-train-sound-from-vestibule-realistic-interior-noise-314562', True),
    ('Station', 'arunangshubanerjee-indian-railway-station-ambience-crowd-chatter-and-train-arrival-331012.mp3', 'arunangshubanerjee', 'city-indian-railway-station-ambience-crowd-chatter-and-train-arrival-331012', True),
    ('Horn', 'tanweraman-train-horn-337875.mp3', 'TanwerAman', 'city-train-horn-337875', False),
    ('Brake', 'freesound_community-depressurization-of-the-brakes-of-a-train-106983.mp3', 'Oscar de Ávila (Freesound), via freesound_community', 'city-depressurization-of-the-brakes-of-a-train-106983', False),
]

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def write_wave(path, samples):
    with wave.open(str(path), 'wb') as output:
        output.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        output.writeframes(np.round(np.clip(samples, -1, 1)*32767).astype('<i2').tobytes())

def main():
    (BANK/'Originals').mkdir(parents=True, exist_ok=True)
    (BANK/'Prepared').mkdir(exist_ok=True)
    records = []
    for name, filename, author, slug, looping in SOURCES:
        src = Path('C:/Users/nisha/Downloads')/filename
        original = BANK/'Originals'/filename
        if original.exists():
            assert digest(original) == digest(src), 'Original collision'
        else:
            shutil.copy2(src, original)
        raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', str(original), '-ac', '1', '-ar', str(RATE), '-af', 'highpass=f=45', '-f', 'f32le', '-'], capture_output=True, check=True).stdout
        x = np.frombuffer(raw, dtype='<f4').astype(np.float64)
        duration = len(x)/RATE
        # Select the strongest short horn event; retain its actual pitch.
        start, end = 0., duration
        if name == 'Horn':
            frame = RATE//10
            energy = np.array([np.mean(x[i:i+frame]**2) for i in range(0,len(x)-frame,frame)])
            peak = int(np.argmax(energy))
            left, right = peak, peak
            threshold = energy[peak]*.18
            while left>0 and energy[left-1]>threshold: left-=1
            while right+1<len(energy) and energy[right+1]>threshold: right+=1
            start=max(0,left*.1-.15);end=min(duration,(right+1)*.1+.3,start+4.)
            x=x[int(start*RATE):int(end*RATE)]
        x-=np.mean(x)
        crossfade=0.
        if looping:
            crossfade=.25 if name=='Rail' else 1.
            n=int(RATE*crossfade)
            ramp=np.linspace(0,1,n)
            blend=x[-n:]*(1-ramp)+x[:n]*ramp
            x=np.concatenate([x[n:-n],blend])
        else:
            n=min(int(RATE*.025),len(x)//4)
            x[:n]*=np.linspace(0,1,n);x[-n:]*=np.linspace(1,0,n)
        target=.5 if name in ('Horn','Brake') else .35
        x*=target/max(float(np.max(np.abs(x))),1e-9)
        output=BANK/'Prepared'/('SW_'+name+'.wav')
        write_wave(output,x)
        rms=float(np.sqrt(np.mean(x*x)))
        assert np.isfinite(x).all() and rms>.001 and np.max(np.abs(x))<.99
        records.append(dict(name=name,creator=author,url='https://pixabay.com/sound-effects/'+slug+'/',downloadDate='2026-10-09',license='Pixabay Content License',licenseUrl='https://pixabay.com/service/terms/',original=str(original.relative_to(ROOT)),originalSha256=digest(original),prepared=str(output.relative_to(ROOT)),preparedSha256=digest(output),sourceSeconds=duration,trimSeconds=[start,end],durationSeconds=len(x)/RATE,sampleRate=RATE,channels=1,bits=16,looping=looping,crossfadeSeconds=crossfade,peak=float(np.max(np.abs(x))),rms=rms,seamDelta=float(abs(x[-1]-x[0])),asset='/Game/IndianRailway/S8/Audio/SW_'+name,auditoryReview='pending human listening',authenticity='Generic sample bank; locomotive class unverified'))
    (BANK/'manifest.json').write_text(json.dumps(dict(schema=1,clips=records),indent=2),encoding='utf-8')
    print(json.dumps(records,indent=2))

if __name__=='__main__': main()
