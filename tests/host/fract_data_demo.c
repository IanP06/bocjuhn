/* A dry, original audition: same production DSP as the experimental firmware. */
#define main original_host_main
#include "host.c"
#undef main
int main(void)
{
    static const int8_t settings[6][4]={{28,-16,64,-64},{48,24,-20,8},{12,28,-20,16},{28,20,-16,16},{0,0,0,16},{20,32,-20,8}};
    int32_t out[CTL];wav_ch=1;
    for(unsigned kind=0;kind<6;kind++) {
        rus_design_t d;rus_t c;rus_design(&d,kind,81991,PATH_OPEN,GEN_NOW);rus_init(&c,0);
        memcpy(c.mac,settings[kind],4);rus_note_on(&c,&d,60);
        for(unsigned b=0;b<5513;b++) {
            if(b==2756&&kind!=0){c.mac[1]+=16;c.mac[3]+=16;}
            if(b==5000)rus_release(&c);
            rus_render(&c,&d,out);for(unsigned i=0;i<CTL;i++)wav_put(out[i]);
        }
        for(unsigned i=0;i<13230;i++)wav_put(0);
    }
    wav_write("build/fract-data/FRACT-DATA-demo.wav");
}
