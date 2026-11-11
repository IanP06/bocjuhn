/* SPDX-License-Identifier: GPL-3.0-only
 * Integer RYUJI percussion at 44100 Hz. No allocations, sample loops or floats.
 * 808 bass drum topology adapted from Plaits AnalogBassDrum, copyright 2016
 * Emilie Gillet, MIT (see assets/ryuji/Mutable-Instruments-LICENSE.txt).
 * Other voices are original two-mode/noise percussion models, not exact circuits.
 */
typedef struct {
    int32_t pulse,pulse_lp,fm_lp,retrigger,low,band,tone;
    int32_t amp,noise_amp,pitch,noise_lp,noise_hp;
    uint32_t phase,phase2,rng;
} ry_va_t;
static int32_t ry_mul(int32_t a,int32_t b){return (int32_t)(((int64_t)a*b)>>20);}
static int32_t ry_diode(int32_t x) {
    if(x>=0)return x;
    /* Bounded rational approximation; this is used only for the short exciter.
       The denominator is strictly positive, including extreme register values. */
    uint32_t a=(uint32_t)(-(int64_t)x)>>8;
    a=a>100000?100000:a;
    return -(int32_t)(a*5734/(4096+2*a))*256;
}
static void ry_va_init(ry_va_t *v,unsigned source) {
    memset(v,0,sizeof *v);v->amp=v->noise_amp=v->pitch=1<<30;
    v->rng=0x727909u+source*1973u;
}
/* Macro damping acts on amplitude, never on phase/sample playback position. */
static int32_t ry_va_tick(ry_va_t *v,unsigned family,unsigned slot,unsigned model,
                          uint32_t age,uint32_t ratio,int damping,int position) {
    int32_t noise=0;
    if(!(model==1 && family==1)) {
        v->rng^=v->rng<<13;v->rng^=v->rng>>17;v->rng^=v->rng<<5;
        noise=(int32_t)(v->rng>>16)-32768;
    }
    int32_t x=0;
    int32_t rate_scale=clampi(128+damping,16,256);
    if(model==1 && family==1) {
        /* Trigger -> diode exciter -> frequency-modulated two-integrator
           resonator -> tone filter. The resonator itself supplies the decay. */
        int32_t pulse;
        if(age<44){v->pulse=8283750;pulse=v->pulse;}
        else {v->pulse-=ry_mul(v->pulse,118890);pulse=v->pulse;}
        v->pulse_lp+=ry_mul(pulse-v->pulse_lp,237772);
        pulse=ry_diode(pulse-v->pulse_lp+ry_mul(pulse,46137));
        int32_t fm=age<265?1048576:0;
        if(age==265)v->retrigger=-838861;
        else if(age>265)v->retrigger-=ry_mul(v->retrigger,476);
        v->fm_lp+=ry_mul(fm-v->fm_lp,237772);
        /* At low pitches the dirty-frequency SVF approximation is extremely
           close to tan(pi*f); cap g for stability at extreme tuning. */
        int32_t base=slot==1?39:52;
        int32_t g=(int32_t)(((uint64_t)(base*75)*ratio)>>15);
        int32_t attack=ry_mul(v->fm_lp,534774);
        int32_t self=clampi(v->low/8,0,60000);
        g=clampi(ry_mul(g,1048576+attack+self),20,300000);
        int32_t r=clampi((int32_t)(38000LL*rate_scale/128),2000,100000);
        int32_t gh=ry_mul(g,g)+ry_mul(g,r);
        int32_t h=1048576-gh+ry_mul(gh,gh)-ry_mul(ry_mul(gh,gh),gh);
        int32_t input=ry_mul(pulse-ry_mul(v->retrigger,209715),889192);
        int32_t hp=ry_mul(input-ry_mul(r+g,v->band)-v->low,h);
        int32_t bp=ry_mul(g,hp)+v->band;
        int32_t lp=ry_mul(g,bp)+v->low;
        v->band=clampi(ry_mul(g,hp)+bp,-16000000,16000000);
        v->low=clampi(ry_mul(g,bp)+lp,-16000000,16000000);
        v->tone+=ry_mul(ry_mul(pulse,62915)+bp-v->tone,105000);
        x=v->tone/20;
    } else {
        unsigned hz=model==1?(slot==1?43:56):model==2?(slot==13?210:112):model==3?185:850;
        unsigned sweep=model==1?110:model==2?45:0;
        int32_t pitch=(int32_t)(((int64_t)sweep*v->pitch)>>30);
        uint32_t inc=(uint32_t)(((uint64_t)((hz+pitch)*97392u)*ratio)>>15);
        if(inc>0x28000000u)inc=0x28000000u;
        v->phase+=inc;v->phase2+=(uint32_t)((uint64_t)inc*(model==3?183:163)/100);
        int32_t body=sin_ph(v->phase);
        int32_t second=sin_ph(v->phase2);
        v->noise_lp+=mulq15(noise-v->noise_lp,5000);
        int32_t high=noise-v->noise_lp;
        v->noise_hp+=mulq15(high-v->noise_hp,model==3?24000:8000);
        if(model==1) {
            /* 909: independent oscillator amplitude, pitch and click envelopes. */
            x=mulq15(body,v->amp>>15)/2;
            x+=mulq15(high,v->noise_amp>>15)/5;
        } else if(model==2) {
            x=mulq15(body+second/12,v->amp>>15)*2/3;
            x+=mulq15(high,v->noise_amp>>15)/20;
        } else if(model==3) {
            x=mulq15(body/3+second/5,v->amp>>15);
            x+=mulq15(v->noise_hp,v->noise_amp>>15)*3/5;
        } else if(model==4) {
            /* Three excitation pulses followed by a separate diffuse tail. */
            if(age==0 || age==441 || age==882)v->amp=1<<30;
            v->tone+=mulq15(v->noise_hp-v->tone,8000);
            x=mulq15(v->tone,(v->amp>>15)+(v->noise_amp>>16));
        } else x=mulq15(body+second/2,v->amp>>15)/2;
        int decay=model==1?190:model==2?250:model==3?650:model==4?2400:2300;
        int nd=model==1 || model==2?7000:model==3?480:400;
        v->amp-= (int32_t)(((int64_t)v->amp*(decay*rate_scale/128))>>20);
        v->noise_amp-=(int32_t)(((int64_t)v->noise_amp*(nd*rate_scale/128))>>20);
        v->pitch-=(int32_t)(((int64_t)v->pitch*(model==1?1800:1200))>>20);
    }
    /* Position is a timbre blend on VA; it never delays or loops a source. */
    if(position)x+=mulq15(x,position*128);
    return clampi(x,-24000,24000);
}
