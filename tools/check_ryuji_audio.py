"""Offline audio measurements; optional deps: numpy scipy soundfile.
Run after tools/ryuji_quality_audit.c exports build/quality/*.wav.
No reference recordings are bundled or downloaded by this script.
"""
from pathlib import Path
import json, importlib.util
import numpy as np
import soundfile as sf
from scipy import signal
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/rc10';OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('bank',ROOT/'tools/gen_pulse.py')
bank=importlib.util.module_from_spec(spec);spec.loader.exec_module(bank)
quant=[]
for p in sorted((ROOT/'assets/ryuji').glob('*.wav')):
    x,sr=sf.read(p,dtype='int16');y=np.array([bank.MU[bank.encode_mu(int(v))] for v in x],dtype=float)
    a=x.astype(float)
    snr=10*np.log10(np.sum(a*a)/(np.sum((a-y)**2)+1e-30))
    assert snr>35,(p.name,snr)
    quant.append(dict(file=p.name,rate=sr,samples=len(x),companding_snr_db=round(snr,2)))
metrics=[];clips=[];timeline=[];at=0
for family in (1,2):
    for pad,label in [(0,'kick'),(2,'snare'),(3,'clap'),(4,'closed hat'),(5,'open hat'),(10,'cowbell' if family==1 else 'ride'),(11,'cymbal' if family==1 else 'crash')]:
        p=ROOT/f'build/quality/{family}-{pad}-default.wav';x,sr=sf.read(p)
        energy=np.cumsum(x*x);f,pow=signal.welch(x,sr,nperseg=4096)
        metrics.append(dict(family=808 if family==1 else 909,instrument=label,peak=float(max(abs(x))),t95_ms=round(1000*np.searchsorted(energy,energy[-1]*.95)/sr,1),centroid_hz=round(float((f*pow).sum()/pow.sum()),1)))
        # Two seconds preserve almost all selected tails. Individual level matching
        # is deliberately not applied to this kit audition: it reveals kit balance.
        clips.extend([x,np.zeros(int(.25*sr))]);timeline.append(dict(seconds=at,family=family,instrument=label));at+=len(x)/sr+.25
sf.write(OUT/'ryuji-hybrid.wav',np.concatenate(clips),44100,subtype='PCM_16')
(OUT/'audio-metrics.json').write_text(json.dumps(dict(codec=quant,engine=metrics,timeline=timeline),indent=2)+'\n')
print('Capture companding SNR:',min(q['companding_snr_db'] for q in quant),'to',max(q['companding_snr_db'] for q in quant),'dB')
print(json.dumps(metrics,indent=2))
