#define main host_main
#include "host.c"
#undef main
#include <assert.h>
#include <time.h>
/* Audio integrity and bounded-state tests independent of UI/storage behavior. */
int main(void) {
    int32_t out[CTL];ba_design_t d;ba_kit_t kit;
    unsigned pcm=0,va=0;
    for(unsigned seed=0;seed<128;seed++){ba_design(&d,-1,seed,PATH_OPEN,GEN_NOW);assert(d.kind>=6 && d.kind<BK_N);}
    for(unsigned s=0;s<PULSE_N;s++) {
        ba_design(&d,6,123,PATH_OPEN,GEN_NOW);ba_kit_get(&d,&kit);
        unsigned pad=PULSE_ROLE[s];kit.source[pad]=s;ba_kit_put(&d,&kit);
        ba_voice_t v;ba_init(&v,1);ba_note_on(&v,&d,48+pad,127,1);
        if(!PULSE_MODEL[s]) {
            pcm++;uint32_t pos=0;
            for(unsigned b=0;b<128;b++) {
                memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);
                for(unsigned i=0;i<CTL;i++) {
                    uint32_t len=PULSE_OFF[s+1]-PULSE_OFF[s];
                    int32_t raw=ba_pcm(PULSE_OFF[s],len,pos);
                    assert(out[i]==mulq12(clampi(raw,-24000,24000),d.out));
                    uint32_t end=len<<16;pos=v.inc>=end-pos?end:pos+v.inc;
                }
            }
        } else va++;
        /* Extreme macro combinations, all supported octaves and pad tuning.
           Watch all resonator states, finite termination and no position wrap. */
        for(unsigned extreme=0;extreme<4;extreme++) {
            ba_init(&v,1);for(unsigned m=0;m<4;m++)v.mac[m]=(extreme&(1u<<(m&1)))?64:-64;
            ba_note_on(&v,&d,48+pad,127,1);v.inc=extreme<2?v.inc>>8:v.inc<<8;
            for(unsigned b=0;b<4300 && v.live;b++) {
                uint32_t before=v.pos;memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);
                assert(v.pos>=before);
                for(unsigned i=0;i<CTL;i++)assert(abs(out[i])<=18000);
                assert(abs(v.va.low)<=16000000 && abs(v.va.band)<=16000000);
            }
            if(PULSE_MODEL[s])assert(!v.live);
        }
    }
    printf("HYBRID PASS: %u PCM sources bypass coloration; %u VA sources; extreme tuning/macros bounded; voice state %zu bytes\n",pcm,va,sizeof(ba_voice_t));
    /* Report host throughput only; target firmware telemetry is required for
       an actual device CPU percentage. Include the live part of each sound. */
    volatile uint32_t sink=0;
    for(unsigned family=1;family<=2;family++)for(unsigned pad=0;pad<6;pad+=2) {
        ba_design(&d,(int)family+5,123,PATH_OPEN,GEN_NOW);
        clock_t start=clock();
        for(unsigned rep=0;rep<400;rep++) {
            ba_voice_t v;ba_init(&v,1);ba_note_on(&v,&d,48+pad,127,1);
            for(unsigned b=0;b<128;b++){ba_render(&v,&d,out,CTL);sink+=out[b%CTL];}
        }
        printf("HOST %u pad %u: %.2f ns/sample\n",family,pad,1e9*(clock()-start)/CLOCKS_PER_SEC/(400.0*128*CTL));
    }
    return sink==0x12345678;
}
