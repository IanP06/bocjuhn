/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental dry Particle Noise adaptation. Particle/SVF equations derived
 * from Emilie Gillet's MIT-licensed Plaits/stmlib (see LICENSES/Plaits-MIT.txt).
 * No diffuser: MORPH spans original 0.5..1.0. FM1 note ownership/ADSR retained.
 * Default builds still compile the untouched RC3 algorithm in cal.c.
 */
#include "particle_tables.h"
#define DP_ONE (1 << 24)
#define DP_LIMIT (1 << 28)
AINL int32_t dp_mul(int32_t a, int32_t b) { return (int32_t)(((int64_t)a*b + (1<<23)) >> 24); }
AINL int32_t dp_sat(int32_t x) { return clampi(x, -DP_LIMIT, DP_LIMIT); }
/* Reciprocal of Q24 >= 1; 32-bit initial division plus Newton refinement. */
AINL int32_t dp_recip(int32_t x) {
    int32_t y=(int32_t)(0x40000000u/(uint32_t)(x>>12)) << 6;
    y += dp_mul(y, DP_ONE-dp_mul(x,y));
    y += dp_mul(y, DP_ONE-dp_mul(x,y));
    return y;
}
AINL void dp_coef(dp_filter_t *p, uint32_t f, int32_t r) {
    /* Same FREQUENCY_DIRTY cubic as stmlib. f is cycles/sample, Q24. */
    int32_t ff=(int32_t)f;
    p->g=dp_mul(ff,52707179 + dp_mul(194346347,dp_mul(ff,ff)));
    p->r=r;
    p->h=dp_recip(DP_ONE+dp_mul(p->g,p->g+r));
}
AINL int32_t dp_step(dp_filter_t *p, int32_t in, int lp_out) {
    int32_t hp=dp_mul(in-dp_mul(p->r+p->g,p->s1)-p->s2,p->h);
    int32_t gh=dp_mul(p->g,hp), bp=gh+p->s1;
    int64_t a1=(int64_t)p->g*hp*2+p->e1;
    int32_t delta1=(int32_t)(a1>>24);
    p->e1=(int32_t)(a1-(int64_t)delta1*DP_ONE);
    p->s1=dp_sat(p->s1+delta1);
    int32_t gb=dp_mul(p->g,bp),lp=gb+p->s2;
    int64_t a2=(int64_t)p->g*bp*2+p->e2;
    int32_t delta2=(int32_t)(a2>>24);
    p->e2=(int32_t)(a2-(int64_t)delta2*DP_ONE);
    p->s2=dp_sat(p->s2+delta2);
    return lp_out?lp:bp;
}
/* Bipolar preset macro: endpoints always reach 0 and 1, zero recalls preset. */
AINL int32_t dp_control(int32_t base,int32_t macro) {
    return clampi(base + macro*(macro<0?base:Q15-base)/MAC_MAX,0,Q15);
}
AINL uint32_t dp_table(const uint32_t *t,int32_t x) {
    uint32_t i=(uint32_t)x>>8,f=(uint32_t)x&255u;
    return i>=128?t[128]:t[i]+(uint32_t)(((int64_t)((int32_t)t[i+1]-(int32_t)t[i])*f)>>8);
}
static void cal_init(cal_t *c,uint32_t seed) { memset(c,0,sizeof *c);rnd_seed(&c->r,seed); }
AINL int cal_fingers_down(const cal_t *c) { return c->nheld; }
static const char *cal_mac_name(const cal_design_t *d,uint32_t j) {
    static const char *const names[4]={"FREQUENCY","HARMONICS","TIMBRE","MORPH"};
    (void)d;return names[j&3u];
}
static void cal_land(cal_t *c,const cal_design_t *d,uint32_t k,int held) {
    (void)d;(void)held;c->sync=1;c->live=1;c->inc[k&7u]=pitch_inc(c->base_t);
}
static void cal_note_on(cal_t *c,const cal_design_t *d,uint32_t note,uint32_t who) {
    uint32_t k;
    for(k=0;k<c->nheld;k++) if(c->note[k]==note && c->nsrc[k]==who) {
        for(;k+1<c->nheld;k++){c->note[k]=c->note[k+1];c->nsrc[k]=c->nsrc[k+1];}c->nheld--;break;
    }
    if(c->nheld==8){for(k=0;k<7;k++){c->note[k]=c->note[k+1];c->nsrc[k]=c->nsrc[k+1];}c->nheld--;}
    c->note[c->nheld]=(uint8_t)note;c->nsrc[c->nheld++]=(uint8_t)who;
    c->base=c->base_t=(int32_t)note*16;c->hush=0;c->tail_blocks=0;
    cal_land(c,d,c->dw++&7u,1);
}
static void cal_lift(cal_t *c) { c->awake=0; }
static void cal_note_off(cal_t *c,uint32_t note,uint32_t who) {
    for(uint32_t k=0;k<c->nheld;k++)if(c->note[k]==note&&c->nsrc[k]==who){
        for(;k+1<c->nheld;k++){c->note[k]=c->note[k+1];c->nsrc[k]=c->nsrc[k+1];}
        if(--c->nheld)c->base_t=c->note[c->nheld-1]*16;else cal_lift(c);return;
    }
}
static void cal_release(cal_t *c,uint32_t who) {
    uint32_t k=0;while(k<c->nheld){if(c->nsrc[k]==who)cal_note_off(c,c->note[k],who);else k++;}
}
static FAST int cal_render(cal_t *c,const cal_design_t *d,int32_t *out) {
    memset(out,0,CTL*sizeof *out);if(!c->live)return 0;
    int32_t h=dp_control(d->drift,c->mac[1]);
    int32_t t=dp_control(d->touch,c->mac[2]);
    int32_t m=dp_control(d->sharp[0],c->mac[3]);
    /* Dry MORPH: Q = 0.5 + 2^(10*m), i.e. original upper half. */
    uint32_t q=32768u+exp2_q16(m*20);
    int32_t r=(int32_t)(0xFFFFFFFFu/(q>>8));
    uint32_t qi=16777216u/isqrt32(q); /* 1/sqrt(Q), Q16 */
    uint32_t density=dp_table(DP_DENSITY,t), pre=dp_table(DP_PRE,t);
    int32_t spread=(int32_t)(((int64_t)h*h*768)>>30); /* +/-48 semitones, note/16 */
    int32_t note=clampi(c->base_t+c->mac[0]*12,0,127*16);
    uint32_t f0=pitch_inc(note)>>8;
    dp_coef(&c->post,f0>8220836u?8220836u:f0,2*DP_ONE);
    for(uint32_t k=0;k<6;k++) {
        dp_filter_t *p=&c->particle[k];int changed=0;
        for(uint32_t i=0;i<CTL;i++) {
            uint32_t u=rnd_next(&c->r);int force=c->sync&&i==0;
            int32_t input=0;
            if(force||(c->nheld && !c->hush && u<density)) {
                if(!changed) {
                    int32_t offset=(int32_t)(((int64_t)rnd_bi(&c->r)*spread)>>15);
                    uint32_t f=pitch_inc(clampi(note+offset,0,127*16))>>8;
                    if(f>4194304u)f=4194304u;
                    dp_coef(p,f,r);
                    /* Gain normalization uses reference 48 kHz normalized f. */
                    uint32_t f48=(uint32_t)(((uint64_t)f*60211u)>>16);
                    uint32_t invf=exp2_q16((24*Q16-log2_q16(f48?f48:1))/2);
                    p->pre=(int32_t)(((uint64_t)pre*qi)>>16);
                    p->pre=(int32_t)(((uint64_t)(uint32_t)p->pre*invf)>>16);
                    changed=1;
                }
                /* u / density, Q16. Division only on events, no 64-bit divide. */
                uint32_t amp=force?65536u:(uint32_t)(((uint64_t)u*(0xFFFFFFFFu/(density>>8)))>>24);
                if(amp>65536u)amp=65536u;
                input=(int32_t)(((int64_t)p->pre*amp)>>14); /* Q18 */
                input=dp_sat(input);
            }
            out[i]+=dp_step(p,input,0);
        }
    }
    c->sync=0;
    int32_t peak=0;
    for(uint32_t i=0;i<CTL;i++) {
        int32_t x=dp_step(&c->post,out[i],1);
        /* Fixed headroom (1/32), then bounded soft saturation; no automatic gain. */
        out[i]=tanh_q15(x>>8);if(absi(out[i])>peak)peak=absi(out[i]);
    }
    if(!c->nheld||c->hush) {
        c->tail_blocks++;
        /* Bounded tail; natural decay first, 20 ms emergency fade when hushed.
         * After 12 s fade integrators to terminate integer limit cycles safely. */
        if(c->hush||c->tail_blocks>16538u) {
            int32_t fade=cal_ex(20);
            for(uint32_t k=0;k<6;k++){c->particle[k].s1=mulr16(c->particle[k].s1,fade);c->particle[k].s2=mulr16(c->particle[k].s2,fade);}
            c->post.s1=mulr16(c->post.s1,fade);c->post.s2=mulr16(c->post.s2,fade);
        }
        int32_t state=absi(c->post.s1)|absi(c->post.s2);
        for(uint32_t k=0;k<6;k++)state|=absi(c->particle[k].s1)|absi(c->particle[k].s2);
        if((peak<2&&state<256&&c->tail_blocks>512u)||c->tail_blocks>17916u){
            memset(c->particle,0,sizeof c->particle);memset(&c->post,0,sizeof c->post);c->live=0;
        }
    }else c->tail_blocks=0;
    c->base=c->base_t;return c->live;
}
static __typeof__(&cal_render) volatile cal_render_fp=cal_render;
static void cal_design(cal_design_t *d,int32_t kind,uint32_t seed,uint32_t path,uint32_t gen) {
    static const uint16_t timbre[CS_N]={4000,7000,10000,20000,12000,16000,6000,5000,24000,30000};
    static const uint16_t morph[CS_N]={0,20000,27000,8000,23000,29000,31000,25000,3000,12000};
    rnd_t r;rnd_seed(&r,seed);memset(d,0,sizeof *d);uint32_t drawn=rnd_below(&r,CS_N);
    d->kind=(uint8_t)(kind<0?drawn:(uint32_t)kind%CS_N);d->seed=seed;d->path=(uint8_t)path;d->gen=(uint8_t)gen;
    d->touch=clampi(timbre[d->kind]+rnd_in(&r,-1500,1500),0,Q15);
    d->sharp[0]=clampi(morph[d->kind]+rnd_in(&r,-1000,1000),0,Q15);
    d->drift=d->kind==CS_LITANY?0:rnd_in(&r,8000,24000);d->out=4096;d->fingers=8;d->first=1;
}
static void cal_measure(cal_design_t *d){d->out=4096;}
