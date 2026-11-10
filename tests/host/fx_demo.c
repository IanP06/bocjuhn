/* SPDX-License-Identifier: GPL-3.0-only: offline demo of the production DSP. */
#define main host_original_main
#include "host.c"
#undef main
int main(void) {
    grain_reset();wav_ch=2;wav_n=0;clk.bpm=120;
    int32_t x[CTL],l[CTL],r[CTL],send[CTL];
    uint32_t phase[3]={0};const int notes[4][3]={{48,55,60},{51,58,63},{53,60,65},{55,62,67}};
    dl_param(35,55,35,65);rv_param(70,78,35,35);
    for(unsigned block=0;block<22050;block++) {
        unsigned sample=block*CTL,section=sample/(44100*4),beat=sample/22050;
        fx.grain[0][3]=section?65:0;fx.grain[0][1]=section==1?65:100;
        fx.grain[0][0]=section==2?12:-7;fx.grain[0][2]=section==2;
        gr_edit[0][0]=10;gr_edit[0][1]=55;gr_edit[0][2]=40;
        fx.rv[0]=section==2?2:0;fx.dl[0]=2;fx.dl[1]=section==3?3:0;fx.dl[2]=3;fx_update();
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(send,0,sizeof send);
        for(unsigned j=0;j<CTL;j++) {
            int32_t env=Q15-(int32_t)((sample+j)%22050)*Q15/22050,voice=0;
            for(unsigned k=0;k<3;k++){phase[k]+=pitch_inc(notes[(beat/4)%4][k]*16);voice+=sin_ph(phase[k])/8;}
            x[j]=section==2?0:mulq15(voice,env);
        }
        gran_block(0,x,CTL,0,1);
        for(unsigned j=0;j<CTL;j++){l[j]=r[j]=x[j];send[j]=x[j];}
        if(section==3)dl_block(x,l,r,send);
        if(section>=2)rv_block(send,l,r,CTL);
        for(unsigned j=0;j<CTL;j++){wav_put(l[j]);wav_put(r[j]);}
    }
    wav_write("build/rc8/prism-demo.wav");return 0;
}
