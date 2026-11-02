/* SPDX-License-Identifier: GPL-3.0-only
 * RYUJI: PCM attacks with a crossfaded body for the track ADSR.
 * ba_* identifiers retain the original engine slot and serialization fields.
 */
#include "pulse_pcm.h"
typedef struct {
    uint8_t live, gate, hold, slot;
    int8_t mac[4];
    uint32_t pos, inc, age, choke, body;
    int32_t env, lp, damp, dc, last, blend, spectral;
} ba_voice_t;
static void ba_init(ba_voice_t *v,uint32_t seed){(void)seed;memset(v,0,sizeof *v);}
static void ba_note_on(ba_voice_t *v,const ba_design_t *d,int32_t note,int32_t velocity,int fresh)
{
    (void)velocity;(void)fresh;
    int8_t mac[4];memcpy(mac,v->mac,4);memset(v,0,sizeof *v);memcpy(v->mac,mac,4);
    v->slot=(uint8_t)((note-48)&15); v->live=v->gate=1;
    v->env=v->spectral=Q15; v->inc=(uint32_t)d->fm1r;
}
static void ba_note_off(ba_voice_t *v,const ba_design_t *d){(void)d;v->gate=0;}
/* Linear interpolation, including a silent guard at the end. */
static int32_t ba_pcm(uint32_t start,uint32_t len,uint32_t pos)
{
    uint32_t p=pos>>16;
    if(p>=len-1)return 0;
    int32_t a=PULSE_PCM[start+p]*128,b=PULSE_PCM[start+p+1]*128;
    return a+(int32_t)(((int64_t)(b-a)*(pos&65535))>>16);
}
/* Two overlapping triangular grains: no hard wrap or repeated attack.
 * Power-of-two windows keep the audio loop free of division. */
static int32_t ba_body(uint32_t start,uint32_t len,uint32_t phase,uint32_t span)
{
    uint32_t mask=(span<<16)-1, half=span<<15;
    uint32_t a=phase&mask,b=(a+half)&mask;
    int32_t wa=(int32_t)(a<half?a:((span<<16)-a));
    wa>>=span==512?9:8; /* Q16 triangle, peak 32768 */
    uint32_t offset=(len/16)<<16;
    return mulq15(ba_pcm(start,len,offset+a),wa)+mulq15(ba_pcm(start,len,offset+b),Q15-wa);
}
static int ba_render(ba_voice_t *v,const ba_design_t *d,int32_t *out,uint32_t n)
{
    if(!v->live)return 0;
    uint32_t start=PULSE_OFF[v->slot],len=PULSE_OFF[v->slot+1]-start;
    uint32_t span=len>=1100?512:256;
    int32_t structure=clampi(64+v->mac[0],0,128);
    int32_t bright=clampi(d->shape+v->mac[2]*420,700,32700);
    int32_t damping=clampi(64+v->mac[1],0,128);
    int32_t dampcoef=clampi(32700-damping*250,700,32700);
    int32_t position=v->mac[3], amount=position<0?-position:position;
    uint32_t tap=(uint32_t)(8+amount*2)<<16;
    for(uint32_t i=0;i<n;i++) {
        int32_t x=ba_pcm(start,len,v->pos);
        if(v->hold) {
            int32_t body=ba_body(start,len,v->body,span);
            /* Become a continuous body within ~23 ms, independent of playback pitch. */
            if(v->blend<Q15)v->blend=clampi(v->blend+32,0,Q15);
            x+=mulq15(body-x,v->blend);
        }
        if(amount) {
            int32_t y=v->hold?ba_body(start,len,v->body+tap,span):ba_pcm(start,len,v->pos+tap);
            x=(x*128+(position<0?-y:y)*amount)/(128+amount);
        }
        /* Odd, symmetric folding: soft sine-like body -> rich folded overtones. */
        int32_t driven=mulq12(x,2048+structure*96);
        int32_t folded=(driven+16384)&65535;
        folded=folded<32768?folded-16384:49152-folded;
        x=mulq15(x,Q15-structure*128)+mulq15(folded,structure*128);
        v->lp+=mulq15(x-v->lp,bright);
        /* DAMPING closes after the strike; BRIGHTNESS is the static spectral limit. */
        int32_t dc=dampcoef+mulq15(32700-dampcoef,v->spectral);
        v->damp+=mulq15(v->lp-v->damp,dc);
        v->spectral=clampi(v->spectral-v->spectral/(64+(128-damping)*48)-1,0,Q15);
        x=v->damp;
        /* Remove DC from asymmetric PCM regions / comb processing. */
        v->dc=x-v->last+mulq15(v->dc,32740);v->last=x;x=v->dc;
        if(!v->hold)x=mulq15(x,v->env);
        if(v->choke) {x=x*(int32_t)v->choke/128;if(!--v->choke)v->live=0;}
        out[i]=v->live?mulq12(clampi(x,-24000,24000),d->out):0;
        if(v->pos<(len<<16))v->pos+=v->inc;
        v->body=(v->body+v->inc)&((span<<16)-1);v->age++;
        if(!v->hold) {
            v->env-=v->env/(clampi(d->decay,50,1400)*22)+1;
            if(v->env<=2 || (v->pos>>16)>=len)v->live=0;
        }
    }
    return 1;
}
static __typeof__(&ba_render) volatile ba_render_fp=ba_render;
static void ba_design(ba_design_t *d,int32_t kind,uint32_t seed,uint32_t path,uint32_t gen)
{
    rnd_t r; rnd_seed(&r,seed^0x50554c53u);uint32_t drawn=rnd_below(&r,6);
    memset(d,0,sizeof *d);d->kind=kind<0||kind>=6?drawn:(uint8_t)kind;
    d->seed=seed;d->path=path;d->gen=gen;
    static const int32_t tone[6]={26000,32000,14000,10000,28000,22000};
    static const int32_t rate[6]={32768,38000,29000,25000,44000,31000};
    d->shape=clampi(tone[d->kind]+rnd_in(&r,-1200,1200),1800,32700);
    d->fm1r=rate[d->kind]+rnd_in(&r,-800,800);
    d->folds=(d->kind==2?35:d->kind==4?60:0)+rnd_in(&r,0,8);
    d->drive=d->kind==4?5000:4096;
    d->decay=550+rnd_in(&r,-80,80);d->attack=1;d->out=3072;
}
static const char *ba_mac_name(const ba_design_t *d,uint32_t j)
{(void)d;static const char *const names[4]={"STRUCTURE","DAMPING","BRIGHTNESS","POSITION"};return names[j&3];}
static void ba_env_adsr(const ba_design_t *d,int32_t *a){a[0]=1;a[1]=d->decay;a[2]=0;a[3]=100;}
static void ba_measure_on(ba_design_t *d,ba_voice_t *v){(void)v;d->out=3072;} /* PCM peaks bounded offline */
