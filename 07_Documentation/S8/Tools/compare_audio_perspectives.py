"""Measured cab/exterior mixer comparison; not a listening or authenticity test."""
import argparse,json,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('exterior',type=Path);p.add_argument('cab',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
def measure(path):
 with wave.open(str(path),'rb') as w:
  rate=w.getframerate();channels=w.getnchannels();assert w.getsampwidth()==2
  x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float).reshape(-1,channels).mean(axis=1)/32768
 # Mean spectral energy over 4096-frame windows avoids allocating a huge FFT.
 n=4096;blocks=x[:len(x)//n*n].reshape(-1,n)
 energy=np.mean(np.abs(np.fft.rfft(blocks*np.hanning(n),axis=1))**2,axis=0)
 freq=np.fft.rfftfreq(n,1/rate)
 return dict(durationSeconds=len(x)/rate,rms=float(np.sqrt(np.mean(x*x))),peak=float(np.max(np.abs(x))),highFrequencyFraction=float(energy[freq>5000].sum()/max(energy.sum(),1e-20)))
outside=measure(a.exterior);inside=measure(a.cab)
report=dict(exterior=outside,cab=inside,passed=bool(inside['rms']<outside['rms'] and inside['rms']>0 and inside['highFrequencyFraction']<outside['highFrequencyFraction']),scope='Native mixer output: cab quieter and less high-frequency energy; not equal-sample alignment or acoustic authenticity.')
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
raise SystemExit(0 if report['passed'] else 1)
