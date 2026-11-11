"""Build the deterministic RYUJI hybrid bank: captured percussion + compact accents.
Primary low drums use integer VA on device. Sample provenance lives in assets/ryuji.
Build uses only the Python standard library; playback uses a companding LUT.
"""
import math, random, wave, struct, json
from pathlib import Path
SR=22050
NAMES=['KICK','SUB','SNARE','CLAP','HAT','OPEN HAT','RIM','TOM','CLICK','ZAP','METAL','SHARD','NOISE','BLOP','TICK','CRUNCH']
LENGTHS=[6600,5500,3960,3300,880,3300,1760,3300,660,2200,3960,2200,3300,2200,880,2200]
def sample(k,n):
    r=random.Random(0x50434d+k); ph=0.; lp=0.; last=0.; data=[]
    for i in range(n):
        t=i/SR; noise=r.uniform(-1,1); lp+=.14*(noise-lp); hp=noise-lp
        if k<2:
            f=(49 if k==0 else 38)+(135 if k==0 else 65)*math.exp(-t*65)
            ph+=2*math.pi*f/SR; y=math.sin(ph)*math.exp(-t*(16 if k==0 else 11))+.15*hp*math.exp(-t*450)
        elif k==2: y=(.7*hp+.3*math.sin(2*math.pi*187*t))*math.exp(-t*30)
        elif k==3:
            env=sum(math.exp(-(t-a)*65) for a in [0,.011,.023,.036] if t>=a)
            y=hp*env*.45
        elif k in (4,5): y=(.7*hp+.3*math.sin(2*math.pi*7331*t)*math.sin(2*math.pi*5167*t))*math.exp(-t*(125 if k==4 else 23))
        elif k==6: y=(math.sin(2*math.pi*930*t)+.5*math.sin(2*math.pi*1673*t))*math.exp(-t*80)
        elif k==7:
            ph+=2*math.pi*(108+95*math.exp(-t*40))/SR; y=math.sin(ph)*math.exp(-t*27)
        elif k in (8,14): y=hp*math.exp(-t*(400 if k==8 else 230))+.35*math.sin(2*math.pi*(3200 if k==8 else 9400)*t)*math.exp(-t*180)
        elif k==9:
            ph+=2*math.pi*(150+3800*math.exp(-t*65))/SR; y=math.sin(ph)*math.exp(-t*38)
        elif k==10: y=sum(math.sin(2*math.pi*f*t) for f in (431,1033,1859,3011))*.25*math.exp(-t*23)
        elif k==11: y=(math.sin(2*math.pi*1511*t)*math.sin(2*math.pi*2699*t)+hp*.3)*math.exp(-t*45)
        elif k==12: y=(noise-lp)*math.exp(-t*30)*(1 if int(t*240)%3 else .1)
        elif k==13:
            ph+=2*math.pi*(75+400*math.exp(-t*55))/SR; y=math.sin(ph+.8*math.sin(2*ph))*math.exp(-t*35)
        else:
            if i%7==0:last=round(noise*7)/7
            y=last*math.exp(-t*38)
        y*=min(1,i/12,(n-1-i)/64)
        data.append(y)
    mean=sum(data)/len(data); data=[(x-mean)*min(1,i/12,(n-1-i)/64) for i,x in enumerate(data)]
    pk=max(abs(x) for x in data) or 1
    return [max(-120,min(120,round(x/pk*112))) for x in data]
# Classic-inspired original synthesis. Sparse families do not invent a kick for the 727.
FAMILIES=['ORIG','808','909','LM','727']
VOICES={
1:{0:'KICK',1:'SUB',2:'SNARE',3:'CLAP',4:'HAT',5:'OPEN HAT',6:'RIM',7:'TOM',8:'CLAVES',10:'COWBELL',11:'CYMBAL',12:'MARACAS',13:'CONGA',14:'TICK'},
2:{0:'KICK',1:'LOW KICK',2:'SNARE',3:'CLAP',4:'HAT',5:'OPEN HAT',6:'RIM',7:'TOM',10:'RIDE',11:'CRASH'},
3:{0:'KICK',2:'SNARE',3:'CLAP',6:'SIDESTICK'},
4:{7:'BONGO',10:'AGOGO',13:'CONGA',14:'CABASA'}}

def classic(f,k,sr,n):
    f=6 if f==4 else f
    r=random.Random(0x727800+f*101+k);ph=0.;lp=0.;out=[];hold=0.
    digital=f in (3,5,6)
    for i in range(n):
        t=i/sr;noise=r.uniform(-1,1);lp+=.18*(noise-lp);hp=noise-lp
        if k<2:
            base={1:48,2:56,3:63,4:58,5:66,7:72}[f]*(.78 if k==1 else 1)
            ph+=2*math.pi*(base+(110 if f==2 else 70)*math.exp(-t*(75 if f==2 else 110)))/sr
            y=math.sin(ph)*math.exp(-t*({1:10,2:17,3:23,4:27,5:26,7:32}[f]))
            y+=(.34 if digital or f==2 else .10)*hp*math.exp(-t*360)
            if digital:y+=.25*math.sin(ph*2.63)*math.exp(-t*70)
            if f==2:y=math.tanh(y*1.7)
        elif k==2:
            freq={1:180,2:195,3:169,4:210,5:220,7:240}[f]
            body=(math.sin(2*math.pi*freq*t)+.45*math.sin(2*math.pi*freq*1.59*t))*math.exp(-t*(40 if digital else 30))
            y=.48*body+.65*hp*math.exp(-t*({1:22,2:29,3:19,4:44,5:31,7:55}[f]))
        elif k==3:
            env=sum(math.exp(-(t-a)*90) for a in [0,.009+f*.0007,.021+f*.0003] if t>=a)
            y=hp*(env*.55+.5*math.exp(-t*(18+f)))
        elif k in (4,5,11):
            metal=sum((1 if math.sin(2*math.pi*q*t)>0 else -1) for q in (205.3,369.6,522.7,800.1,1043.3,1469.9))/6
            if digital or f==2:metal=sum(math.sin(2*math.pi*q*(1+f*.018)*t) for q in (2173,3299,4817,6343))/4
            y=(metal*.55+hp*.5)*math.exp(-t*(130 if k==4 else 16 if k==5 else 10+f))
            if f==7:y=metal*math.exp(-t*45)
        elif k==6 or k==8:
            freq=850+f*85 if k==6 else 2100+f*110
            y=(math.sin(2*math.pi*freq*t)+.5*math.sin(2*math.pi*freq*1.73*t))*math.exp(-t*(100 if k==6 else 190))
        elif k==7 or k==13:
            freq=(105 if k==7 else 210)+(f*9);ph+=2*math.pi*(freq+freq*.7*math.exp(-t*55))/sr
            y=(math.sin(ph)+(.35 if digital else .08)*math.sin(ph*1.6))*math.exp(-t*(24 if k==7 else 33))+.12*hp*math.exp(-t*260)
        elif k==9:y=math.sin(2*math.pi*1850*t+1.7*math.sin(2*math.pi*28*t))*math.exp(-t*9)
        elif k==10:
            freq=550+f*35;y=(math.sin(2*math.pi*freq*t)+.65*math.sin(2*math.pi*freq*1.48*t))*math.exp(-t*(25+f*2))
            if f==2:y=(y*.4+hp*.35)*math.exp(-t*3)
        elif k in (12,14):y=hp*math.exp(-t*(65 if k==14 else 30))*(.6+.4*math.sin(2*math.pi*(190+f*13)*t)**2)
        else:y=hp*math.exp(-t*15)*max(0,math.sin(2*math.pi*(65+f*5)*t))**5
        if digital:
            if i%2==0:hold=round(y*48)/48
            y=hold
        out.append(y*min(1,i/6,(n-1-i)/48))
    mean=sum(out)/n;out=[(v-mean)*min(1,i/6,(n-1-i)/48) for i,v in enumerate(out)];peak=max(map(abs,out)) or 1
    return [max(-120,min(120,round(v/peak*112))) for v in out]

# Source IDs and pad roles stay stable for saved kits. VA owns the low-frequency
# resonators; captured metallic/noisy transients use companded PCM with a LUT.
CAPTURES={(1,2):'808-snare',(1,3):'808-clap',(1,4):'808-hat',(1,5):'808-open',
          (1,6):'808-rim',(1,8):'808-claves',(1,10):'808-cowbell',(1,11):'808-cymbal',
          (1,12):'808-maracas',(1,14):'808-maracas',(2,4):'909-hat',(2,5):'909-open',
          (2,10):'909-ride',(2,11):'909-crash'}
VA={(1,0):1,(1,1):1,(1,7):2,(1,13):2,
    (2,0):1,(2,1):1,(2,2):3,(2,3):4,(2,6):5,(2,7):2}

def decode_mu(u):
    u=(~u)&255;v=(((u&15)<<3)+132) << ((u>>4)&7)
    return 132-v if u&128 else v-132
MU=[decode_mu(i) for i in range(256)]
# Nearest decoded value avoids an extra bias/rounding loss during offline encode.
SORTED=sorted((v,i) for i,v in enumerate(MU))
def encode_mu(x):
    import bisect
    j=bisect.bisect_left(SORTED,(x,-1));j=min(max(j,1),255)
    return min(SORTED[j-1:j+1],key=lambda a:abs(a[0]-x))[1]

def main():
    data=[];offsets=[0];rates=[];roles=[];families=[];labels=[];models=[];maps=[list(range(16)) for _ in FAMILIES]
    def add(pcm,sr,k,f,label,model=0):
        idx=len(rates);data.extend(encode_mu(v) for v in pcm);offsets.append(len(data));rates.append(sr);roles.append(k);families.append(f);labels.append(label);models.append(model);return idx
    # The secondary original bank is deliberately compact. Primary 808/909 get
    # the bandwidth and tail budget; no new source IDs or storage layout changes.
    for k,n in enumerate(LENGTHS):
        pcm=sample(k,n);pcm=pcm[::2][:1654]
        for j in range(min(64,len(pcm))):pcm[-1-j]=round(pcm[-1-j]*j/64)
        add([v*128 for v in pcm],11025,k,0,NAMES[k])
    for f,voices in VOICES.items():
        for k,label in voices.items():
            model=VA.get((f,k),0)
            if model: pcm=[0,0];sr=22050
            elif (f,k) in CAPTURES:
                path=Path('assets/ryuji')/(CAPTURES[f,k]+'.wav')
                with wave.open(str(path),'rb') as w:
                    assert w.getnchannels()==1 and w.getsampwidth()==2
                    sr=w.getframerate();pcm=list(struct.unpack('<'+'h'*w.getnframes(),w.readframes(w.getnframes())))
            else:
                sr=11025;seconds=.16 if k in (0,2,3,7,10,13) else .07
                pcm=[v*128 for v in classic(f,k,sr,int(sr*seconds))]
            assert len(pcm)<65535
            maps[f][k]=add(pcm,sr,k,f,label,model)
    out=['/* Generated RYUJI hybrid bank. Captures: assets/ryuji/manifest.json. */','#define PULSE_N '+str(len(rates)), '#define PULSE_FAMILIES 5']
    def arr(typ,name,vals):out.append('static const '+typ+' '+name+'['+str(len(vals))+'] = {'+','.join(map(str,vals))+'};')
    arr('uint32_t','PULSE_OFF',offsets);arr('uint16_t','PULSE_RATE',rates);arr('uint8_t','PULSE_ROLE',roles);arr('uint8_t','PULSE_FAMILY',families);arr('uint8_t','PULSE_MODEL',models)
    arr('int16_t','PULSE_MU',MU)
    arr('char *const','PULSE_NAME',['"'+v+'"' for v in NAMES]);arr('char *const','PULSE_LABEL',['"'+v+'"' for v in labels]);arr('char *const','PULSE_FAMILY_NAME',['"'+v+'"' for v in FAMILIES])
    out.append('static const uint8_t PULSE_MAP[5][16] = {'+','.join('{'+','.join(map(str,row))+'}' for row in maps)+'};')
    out.append('static const uint8_t PULSE_PCM['+str(len(data))+'] = {');out += [','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32)];out+=['};']
    Path('build/gen/pulse_pcm.h').write_text('\n'.join(out)+'\n')
    print('RYUJI hybrid:',len(data),'bytes;',sum(bool(x) for x in models),'VA sources;',len(rates),'total sources')
if __name__=='__main__':main()
