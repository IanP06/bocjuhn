/* SPDX-License-Identifier: GPL-3.0-only
 * RYUJI: integer VA and finite companded one-shots. ADSR changes amplitude only.
 * ba_* identifiers retain the original engine slot and serialization fields.
 */
#include "pulse_pcm.h"
#include "ryuji_va.h"
/* Fits unused legacy design fields: layout/size of ba_design_t remains unchanged. */
typedef struct {uint8_t source[16];int8_t tune[16];uint16_t locks,magic;} ba_kit_t;
#define BA_KIT_MAGIC 0x4b39u
static void ba_kit_get(const ba_design_t *d,ba_kit_t *k) {
    memset(k,0,sizeof *k);k->magic=BA_KIT_MAGIC;
    for(unsigned j=0;j<16;j++)k->source[j]=(uint8_t)j;
    if(d->rsv[2]!=0x39)return;
    memcpy(k->source,d->to,32);k->locks=(uint16_t)(d->rsv[0]|d->rsv[1]<<8);
    for(unsigned j=0;j<16;j++){if(k->source[j]>=PULSE_N || PULSE_ROLE[k->source[j]]!=j)k->source[j]=(uint8_t)j;k->tune[j]=(int8_t)clampi(k->tune[j],-24,24);}
}
static void ba_kit_put(ba_design_t *d,const ba_kit_t *k) {
    if(k->magic!=BA_KIT_MAGIC){d->rsv[2]=0;return;}
    memcpy(d->to,k->source,32);d->rsv[0]=(uint8_t)k->locks;d->rsv[1]=(uint8_t)(k->locks>>8);d->rsv[2]=0x39;
    ba_kit_t clean;ba_kit_get(d,&clean);memcpy(d->to,clean.source,32);
}
static unsigned ba_source(const ba_design_t *d,unsigned pad) {
    unsigned n=d->rsv[2]==0x39?((const uint8_t*)d->to)[pad]:pad;
    return n<PULSE_N && PULSE_ROLE[n]==pad?n:pad;
}
static int ba_tune(const ba_design_t *d,unsigned pad) {return d->rsv[2]==0x39?clampi(((const int8_t*)d->to)[16+pad],-24,24):0;}
static unsigned ba_next_source(unsigned pad,unsigned old,int step) {
    for(unsigned j=0;j<PULSE_N;j++){old=(old+PULSE_N+(step>0?1:-1))%PULSE_N;if(PULSE_ROLE[old]==pad)return old;}
    return pad;
}
static void ba_kit_random(ba_kit_t *k,uint32_t seed,int only) {
    rnd_t r;rnd_seed(&r,seed);
    for(unsigned j=0;j<16;j++)if(!(k->locks&(1u<<j)) && (only<0 || (unsigned)only==j)) {
        unsigned count=0;for(unsigned a=0;a<PULSE_N;a++)if(PULSE_ROLE[a]==j)count+=PULSE_FAMILY[a]==1 || PULSE_FAMILY[a]==2?4:1;
        unsigned pick=rnd_below(&r,count);
        for(unsigned a=0;a<PULSE_N;a++)if(PULSE_ROLE[a]==j){unsigned weight=PULSE_FAMILY[a]==1 || PULSE_FAMILY[a]==2?4:1;if(pick<weight){k->source[j]=(uint8_t)a;break;}pick-=weight;}
    }
}
typedef struct {
    uint8_t live, gate, hold, slot;
    uint8_t source; int8_t tune; uint16_t reserved;
    int8_t mac[4];
    uint32_t pos, inc, age, choke, body;
    int32_t env, lp, damp, dc, last, blend, spectral;
    ry_va_t va;
} ba_voice_t;
static void ba_init(ba_voice_t *v,uint32_t seed){(void)seed;memset(v,0,sizeof *v);}
static void ba_note_on(ba_voice_t *v,const ba_design_t *d,int32_t note,int32_t velocity,int fresh)
{
    (void)velocity;(void)fresh;
    int8_t mac[4];memcpy(mac,v->mac,4);memset(v,0,sizeof *v);memcpy(v->mac,mac,4);
    v->slot=(uint8_t)((note-48)&15);v->source=(uint8_t)ba_source(d,v->slot);v->tune=(int8_t)ba_tune(d,v->slot); v->live=v->gate=1;
    v->env=v->spectral=Q15; v->inc=(uint32_t)d->fm1r;
    v->inc=(uint32_t)(((uint64_t)v->inc*PULSE_RATE[v->source])/22050);
    ry_va_init(&v->va,v->source);
    if(v->tune)v->inc=(uint32_t)(((uint64_t)v->inc*exp2_q16((int32_t)v->tune*65536/12))>>16);
}
static void ba_note_off(ba_voice_t *v,const ba_design_t *d){(void)d;v->gate=0;}
/* Linear interpolation, including a silent guard at the end. */
static int32_t ba_pcm(uint32_t start,uint32_t len,uint32_t pos)
{
    uint32_t p=pos>>16;
    if(p>=len-1)return 0;
    int32_t a=PULSE_MU[PULSE_PCM[start+p]],b=PULSE_MU[PULSE_PCM[start+p+1]];
    return a+(int32_t)(((int64_t)(b-a)*(pos&65535))>>16);
}
static int ba_render(ba_voice_t *v,const ba_design_t *d,int32_t *out,uint32_t n)
{
    if(!v->live)return 0;
    uint32_t start=PULSE_OFF[v->source],len=PULSE_OFF[v->source+1]-start;
    unsigned model=PULSE_MODEL[v->source];
    /* Zero macros are a real bypass. Negative brightness is a lowpass;
       positive brightness adds a gentle high-frequency shelf. */
    int32_t structure=clampi(v->mac[0],0,64);
    int32_t bright=clampi(32700+v->mac[2]*450,1200,32700);
    int32_t damping=clampi(v->mac[1],-64,64);
    int32_t position=v->mac[3], amount=position<0?-position:position;
    uint32_t tap=(uint32_t)(8+amount*2)<<16;
    for(uint32_t i=0;i<n;i++) {
        if(!v->live){out[i]=0;continue;}
        int32_t x=model?ry_va_tick(&v->va,PULSE_FAMILY[v->source],v->slot,model,v->age,v->inc,damping,position):ba_pcm(start,len,v->pos);
        if(!model && amount) {
            uint32_t at=v->pos>UINT32_MAX-tap?UINT32_MAX:v->pos+tap;
            int32_t y=ba_pcm(start,len,at);
            x=(x*128+(position<0?-y:y)*amount)/(128+amount);
        }
        if(structure) {
            int32_t driven=mulq12(x,4096+structure*96);
            int32_t folded=(driven+16384)&65535;
            folded=folded<32768?folded-16384:49152-folded;
            x=mulq15(x,Q15-structure*256)+mulq15(folded,structure*256);
        }
        if(v->mac[2]) {
            v->lp+=mulq15(x-v->lp,v->mac[2]<0?bright:10000);
            x=v->mac[2]<0?v->lp:x+mulq15(x-v->lp,v->mac[2]*128);
        }
        /* PCM already owns its envelope. Only an explicit positive damping
           setting adds attenuation. Q30 accumulation avoids short quantized tails. */
        if(!model && damping>0) {
            v->va.amp-=(int32_t)(((int64_t)v->va.amp*damping*12)>>20);
            x=mulq15(x,v->va.amp>>15);
        }
        /* End VA with a quiet finite fade; ADSR hold never changes topology. */
        if(model && v->age>110250)x=(int32_t)((int64_t)x*(132300-v->age)/22050);
        if(v->choke) {x=x*(int32_t)v->choke/128;if(!--v->choke)v->live=0;}
        out[i]=v->live?mulq12(clampi(x,-24000,24000),d->out):0;
        if(!model) {
            uint32_t end=len<<16;
            v->pos=v->inc>=end-v->pos?end:v->pos+v->inc;
            if(v->pos>=end)v->live=0;
        }
        v->age++;
        if(model && v->age>=132300)v->live=0;
        if(model && v->age>22050) {
            if(model==1 && PULSE_FAMILY[v->source]==1) {
                if(absi(v->va.low)+absi(v->va.band)+absi(v->va.tone)<1024)v->live=0;
            } else if(v->va.amp<32768 && v->va.noise_amp<32768)v->live=0;
        }
    }
    return 1;
}
static __typeof__(&ba_render) volatile ba_render_fp=ba_render;
static void ba_design(ba_design_t *d,int32_t kind,uint32_t seed,uint32_t path,uint32_t gen)
{
    rnd_t r; rnd_seed(&r,seed^0x50554c53u);uint32_t drawn=6u+rnd_below(&r,BK_N-6);
    memset(d,0,sizeof *d);d->kind=kind<0||kind>=BK_N?drawn:(uint8_t)kind;
    d->seed=seed;d->path=path;d->gen=gen;
    static const int32_t tone[6]={26000,32000,14000,10000,28000,22000};
    static const int32_t rate[6]={32768,38000,29000,25000,44000,31000};
    d->shape=clampi(tone[d->kind%6]+rnd_in(&r,-1200,1200),1800,32700);
    d->fm1r=rate[d->kind%6]+rnd_in(&r,-800,800);
    d->folds=(d->kind==2?35:d->kind==4?60:0)+rnd_in(&r,0,8);
    d->drive=d->kind==4?5000:4096;
    d->decay=550+rnd_in(&r,-80,80);d->attack=1;d->out=3072;
    d->shape=32700;d->fm1r=32768; /* clean source pitch, including original kits */
    if(d->kind>=6) {
        ba_kit_t kit;ba_kit_get(d,&kit);d->shape=30000;d->fm1r=32768;
        static const uint8_t hybrid[5][16]={
            {1,1,3,2,2,2,3,1,1,0,1,2,1,1,4,0},
            {2,2,2,2,1,2,2,2,1,0,2,2,1,1,1,0},
            {1,1,3,3,2,2,3,1,1,0,1,2,1,1,1,0},
            {1,1,2,1,2,2,2,4,1,0,4,2,1,4,4,0},
            {1,2,1,2,1,2,1,2,1,0,1,2,1,4,1,0}};
        for(unsigned j=0;j<16;j++)kit.source[j]=PULSE_MAP[d->kind<8?d->kind-5:hybrid[d->kind-8][j]][j];
        if(d->kind==12)ba_kit_random(&kit,seed,-1);
        ba_kit_put(d,&kit);
    }
}
static const char *ba_mac_name(const ba_design_t *d,uint32_t j)
{(void)d;static const char *const names[4]={"STRUCTURE","DAMPING","BRIGHTNESS","POSITION"};return names[j&3];}
static void ba_env_adsr(const ba_design_t *d,int32_t *a){a[0]=1;a[1]=d->decay;a[2]=0;a[3]=100;}
static void ba_measure_on(ba_design_t *d,ba_voice_t *v){(void)v;d->out=3072;} /* PCM peaks bounded offline */
