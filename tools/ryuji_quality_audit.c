#define main host_main
#include "../tests/host/host.c"
#undef main
/* Diagnostic exports from the unchanged RC9 renderer, no effects or mixer. */
int main(void) {
    wav_ch=1;
    for(unsigned family=1;family<=2;family++)for(unsigned j=0;j<7;j++) {
        unsigned pad=(unsigned[]){0,2,3,4,5,10,11}[j];unsigned source=PULSE_MAP[family][pad];
        ba_design_t d;ba_design(&d,5+family,123,PATH_OPEN,GEN_NOW);
        char path[160];unsigned start=PULSE_OFF[source],len=PULSE_OFF[source+1]-start;
        for(unsigned n=0;n<44100*2;n++){uint64_t pos=(uint64_t)n*65536*PULSE_RATE[source]/44100;wav_put(pos>UINT32_MAX?0:ba_pcm(start,len,(uint32_t)pos));}
        snprintf(path,sizeof path,"build/quality/%u-%u-raw.wav",family,pad);wav_write(path);
        for(unsigned mode=0;mode<2;mode++) {
            ba_voice_t v;ba_init(&v,1);if(mode){v.mac[0]=-64;v.mac[1]=-64;v.mac[2]=64;}
            ba_note_on(&v,&d,48+pad,127,1);int32_t out[CTL];
            for(unsigned block=0;block<44100*2/CTL;block++){memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);for(unsigned n=0;n<CTL;n++)wav_put(out[n]);}
            snprintf(path,sizeof path,"build/quality/%u-%u-%s.wav",family,pad,mode?"least-colored":"default");wav_write(path);
        }
    }
    return 0;
}
