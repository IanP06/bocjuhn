/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../../firmware/src/acid.h"
static uint32_t inc(int note) { return (uint32_t)(440.0*pow(2.0,(note-69)/12.0)*4294967296.0/44100.0); }
int main(int argc,char **argv) {
    acid_voice v;
    acid_params p={7000,26000,21000,220,24000,70,10000,0};
    int notes[]={36,36,48,39,36,51,43,39,60,63,67,72,63,60,55,48};
    int n, peak=0; long long energy=0;
    FILE *f=argc>1 ? fopen(argv[1],"wb") : NULL;
    if(argc>1 && !f) return 2;
    acid_init(&v);
    acid_on(&v,inc(36),0,0);
    for(n=0;n<100;++n) acid_sample(&v,&p);
    { int32_t env=v.env; uint32_t old=v.inc;
      acid_on(&v,inc(48),1,1); assert(v.env==env && v.inc==old);
      acid_on(&v,inc(39),0,0); assert(v.env==32767 && v.inc==inc(39)); }
    acid_init(&v);
    for(n=0;n<44100*8;++n) {
        int step=n/5512, within=n%5512;
        int16_t out;
        if(!within) acid_on(&v,inc(notes[step%16]),step%4==0,step%4==2);
        if(within==4000 && (step+1)%4!=2) acid_off(&v);
        p.square=n>=44100*4;
        out=(int16_t)acid_sample(&v,&p);
        if(abs(out)>peak) peak=abs(out);
        energy+=(long long)out*out;
        if(f) fwrite(&out,2,1,f);
    }
    assert(peak>500 && energy>1000000);
    acid_off(&v);
    for(n=0;n<44100;++n) acid_sample(&v,&p);
    assert(v.amp==0);
    /* Sweep extremes for bounded state and output. */
    for(n=0;n<200000;++n) {
        p.cutoff=n%32768; p.resonance=32767; p.drive=32767;
        if(n%1000==0) acid_on(&v,inc(24+(n/1000)%85),1,n%2000==0);
        assert(abs(acid_sample(&v,&p))<=32767);
        for(int j=0;j<4;++j) assert(abs(v.pole[j])<=32768);
    }
    if(f) fclose(f);
    printf("ACID prototype PASS: retrigger, slide continuity, release, bounded sweep; peak=%d\n",peak);
    return 0;
}
