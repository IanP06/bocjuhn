/* SPDX-License-Identifier: GPL-3.0-only
 * Original FRACT DATA engine. No artist samples or third-party Pd code.
 * Full-rate integer synthesis; finite packets on a sample-counted clock.
 * Optional build: RC4 remains the default. Session design layout is retained;
 * old FRACT presets necessarily have a new sound and new macro meanings.
 */
#include "fract_data_tables.h"
#define FRACT_N 6u
/* Keep the old symbolic identifiers for existing host diagnostics. Production
 * preset selection uses FRACT_N; only the first six kinds are exposed. */
enum { RK_HOLD, RK_DUST, RK_GATES, RK_CRACKLE, RK_NOR, RK_XOR, RK_RING, RK_CASCADE,
       RK_CLANG, RK_CHUNKS, RK_JUMP, RK_WANDER, RK_BEND, RK_QUAD, RK_BLIPS,
       RK_WALK, RK_CHORD, RK_N };
static const char *const RK_NAME[RK_N] = {"SINE","CLICK","BITS","SCAN","SUB","GRID"};
typedef struct {
    uint32_t seed;
    uint8_t kind,path,gen,bits,hold,chord,rsv[2];
    int32_t p1,p2,chaos,att,rel,out;
} rus_design_t;
typedef struct {
    uint32_t ph,inc,wp,wi,age,len;
    uint8_t click;
} fd_packet_t;
typedef struct {
    uint8_t live,nheld,trig,pad;
    uint8_t note[8];
    int8_t mac[NMAC];
    int32_t env,base,steady;
    uint32_t inc,phase,until,step,events,dropped;
    fd_packet_t packet[4];
} rus_t;

static uint32_t fd_hash(uint32_t x)
{ x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16); }
/* round(2^40 / 44100), local to FRACT: do not change other engines' tuning. */
static uint32_t fd_phase_inc(uint32_t hz8)
{ return (uint32_t)(((uint64_t)hz8*24932236u)>>16); }
static void rus_init(rus_t *c,uint32_t seed)
{ (void)seed;memset(c,0,sizeof *c); }
static void rus_note_on(rus_t *c,const rus_design_t *d,uint32_t note)
{
    (void)d;int fresh=!c->nheld;note=(uint32_t)clampi((int32_t)note,0,127);
    for(uint32_t k=0;k<c->nheld;k++)if(c->note[k]==note) {
        for(;k+1<c->nheld;k++)c->note[k]=c->note[k+1];c->nheld--;break;
    }
    if(c->nheld==8) {for(uint32_t k=0;k<7;k++)c->note[k]=c->note[k+1];c->nheld--;}
    c->note[c->nheld++]=(uint8_t)note;c->base=(int32_t)note*16;c->live=1;
    if(fresh) {c->trig=1;c->until=0;c->step=0;}
}
static void rus_note_off(rus_t *c,uint32_t note)
{
    for(uint32_t k=0;k<c->nheld;k++)if(c->note[k]==note) {
        for(;k+1<c->nheld;k++)c->note[k]=c->note[k+1];c->nheld--;
        if(c->nheld)c->base=(int32_t)c->note[c->nheld-1]*16;return;
    }
}
static void rus_release(rus_t *c) {c->nheld=0;}
static void rus_design(rus_design_t *d,int32_t kind,uint32_t seed,uint32_t path,uint32_t gen)
{
    memset(d,0,sizeof *d);d->seed=seed;d->kind=(uint8_t)(kind<0||kind>=FRACT_N?fd_hash(seed)%FRACT_N:(uint32_t)kind);
    d->path=(uint8_t)path;d->gen=(uint8_t)gen;d->bits=16;d->hold=1;
    d->att=2;d->rel=30;d->out=4096;
}
static void rus_measure(rus_design_t *d) {d->out=4096;}
static const char *rus_mac_name(const rus_design_t *d,uint32_t j)
{ (void)d;static const char *const names[4]={"FREQ","RATE","LENGTH","DATA"};return names[j&3]; }

/* At MIDI 60: FREQ = 20..18000 Hz (SUB 20..180 Hz), logarithmic.
 * Keyboard transposes, with final audible-band clamps. 64-bit products only;
 * all divisions are 32-bit, and no allocation occurs in the audio path. */
static uint32_t fd_hz8(const rus_t *c,unsigned fi,int sub)
{
    uint32_t ratio=(uint32_t)exp2_q16((c->base-60*16)*65536/(12*16));
    uint32_t hz=(uint32_t)(((uint64_t)(sub?FD_SUB8[fi]:FD_FREQ8[fi])*ratio)>>16);
    return (uint32_t)clampi((int32_t)hz,20*256,(sub?180:18000)*256);
}
static void fd_fire(rus_t *c,uint32_t hz8,uint32_t length,int click)
{
    fd_packet_t *v=0;
    for(unsigned k=0;k<4;k++)if(!c->packet[k].len){v=&c->packet[k];break;}
    /* Saturation skips a new packet instead of cutting a sounding low sine. */
    if(!v){c->dropped++;return;}
    memset(v,0,sizeof *v);v->len=length;v->inc=fd_phase_inc(hz8);
    v->wi=0xffffffffu/(length-1u);v->click=(uint8_t)click;c->events++;
}
static void fd_event(rus_t *c,const rus_design_t *d,unsigned fi,unsigned li,unsigned data)
{
    uint32_t step=c->step++ & 15u;
    uint32_t word=fd_hash(d->seed^(step*0x9e3779b9u)^((data/16u)*0x85ebca6bu));
    unsigned density=1+data*15/128;
    if(step && (word&15u)>=density)return;
    unsigned kind=d->kind%FRACT_N;
    int sub=kind==4 || (kind==5 && (step&3u)==0);
    int click=kind==1 || (kind==5 && (step&3u)==1);
    unsigned index=fi;
    if(kind==3)index=(unsigned)clampi((int)fi+((int)((step+data/8)&15u)-8)*4,0,128);
    uint32_t hz8=fd_hz8(c,index,sub),length=FD_LENGTH[li];
    if(kind==2 && (word&256u))hz8=(uint32_t)clampi((int32_t)hz8*2,20*256,18000*256);
    if(kind==5 && !sub && !click)hz8=(uint32_t)clampi((int32_t)hz8*8,20*256,18000*256);
    if(click) {
        /* Odd-symmetric one-cycle pulse under a Hann window. FREQ narrows it;
         * LENGTH widens it. Even support retains bipolar symmetry. These
         * intentionally broadband clicks are not advertised as bandlimited. */
        length=(uint32_t)clampi((int32_t)(length*600u/(hz8/256u)),4,2048)&~1u;
    } else if(sub) {
        uint32_t minimum=44100u*3u/(hz8/256u);
        length=minimum+length; /* at least three periods, even at low LENGTH */
    }
    fd_fire(c,hz8,length,click);
}
static int rus_render(rus_t *c,const rus_design_t *d,int32_t *out)
{
    if(!c->live){memset(out,0,CTL*sizeof *out);return 0;}
    unsigned fi=(unsigned)clampi(c->mac[0]+64,0,128),ri=(unsigned)clampi(c->mac[1]+64,0,128);
    unsigned li=(unsigned)clampi(c->mac[2]+64,0,128),data=(unsigned)clampi(c->mac[3]+64,0,128);
    uint32_t period=FD_PERIOD[ri];
    int continuous=d->kind%FRACT_N==0 && li==128;
    c->inc=fd_phase_inc(fd_hz8(c,fi,0));
    int32_t target=c->nheld?Q15*256:0;
    int32_t factor=cal_ex_fp(target>c->env?d->att:d->rel);
    int32_t e1=target+mulq16(c->env-target,factor);
    if(absi(e1-target)<32*256)e1=target;
    int32_t de=(e1-c->env)/(int32_t)CTL;
    /* Old sessions may contain large calibrated gains from the old algorithm.
     * Honor attenuation, but never amplify beyond this engine's bounded gain. */
    int32_t gain=clampi(d->out,0,4096);
    for(unsigned i=0;i<CTL;i++) {
        if(c->nheld && !c->until) {
            if(!continuous)fd_event(c,d,fi,li,data);
            c->until=period;c->trig=0;
        }
        if(c->until)c->until--;
        int32_t sum=0;
        for(unsigned k=0;k<4;k++) {
            fd_packet_t *v=&c->packet[k];if(!v->len)continue;
            int32_t w=(32767-sin_ph(v->wp+0x40000000u))/2;
            int32_t x=sin_ph(v->click?v->wp:v->ph);
            sum+=mulq15(x,w);v->ph+=v->inc;v->wp+=v->wi;
            if(++v->age>=v->len)v->len=0;
        }
        /* 128-sample crossfade when entering/leaving sustained SINE. Four
         * packets plus the carrier remain below 30720 before the envelope. */
        c->steady=clampi(c->steady+(continuous?256:-256),0,32767);
        sum+=mulq15(sin_ph(c->phase),c->steady);
        c->phase+=c->inc;c->env+=de;
        out[i]=mulq12(mulq15(sum*3/16,c->env>>8),gain);
    }
    c->env=e1;
    if(!c->nheld&&!e1){c->live=0;memset(c->packet,0,sizeof c->packet);return 0;}
    return 1;
}
