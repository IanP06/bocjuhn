#define main original_host_main
#include "host.c"
#undef main
#include <assert.h>
static uint64_t fd_render_test(unsigned kind,unsigned seed,int macro,int value,int note,int write)
{
    rus_t c;rus_design_t d;int32_t out[CTL];rus_design(&d,kind,seed,PATH_OPEN,GEN_NOW);rus_init(&c,seed);
    if(macro>=0)c.mac[macro]=value;rus_note_on(&c,&d,note);
    uint64_t h=1469598103934665603ull;int peak=0;double energy=0;
    for(unsigned b=0;b<4410;b++) {
        if(b==4134)rus_note_off(&c,note);
        rus_render(&c,&d,out);
        for(unsigned i=0;i<CTL;i++) {
            assert(abs(out[i])<=30720);if(abs(out[i])>peak)peak=abs(out[i]);
            energy+=(double)out[i]*out[i];h=(h^(uint32_t)out[i])*1099511628211ull;
            if(write)wav_put(out[i]);
        }
    }
    assert(peak>0&&energy>0&&!c.live&&!c.nheld);
    if(write){char path[100];snprintf(path,sizeof path,"build/fract-data/%02u-%s.wav",kind,RK_NAME[kind]);wav_write(path);}
    return h;
}
static void fd_sine(unsigned index)
{
    rus_t c;rus_design_t d;int32_t out[CTL];rus_design(&d,0,1,0,GEN_NOW);rus_init(&c,1);
    c.mac[0]=(int)index-64;c.mac[2]=64;rus_note_on(&c,&d,60);
    double hz=FD_FREQ8[index]/256.0;unsigned n=0;double aa=0,bb=0,ab=0;
    for(unsigned b=0;b<1600;b++) {
        rus_render(&c,&d,out);
        for(unsigned i=0;i<CTL;i++,n++) {
            double ref=sin(2*M_PI*hz*n/44100.0);
            if(n>=4096){aa+=(double)out[i]*out[i];bb+=ref*ref;ab+=out[i]*ref;}
            wav_put(out[i]);
        }
    }
    double corr=ab/sqrt(aa*bb);assert(corr>0.99999);
    char path[100];snprintf(path,sizeof path,"build/fract-data/sine-%u.wav",index);wav_write(path);
    printf("sine %.6f Hz: correlation %.9f with analytical reference\n",hz,corr);
}
int main(void)
{
    wav_ch=1;
    assert(sizeof(rus_design_t)==36);
    for(unsigned k=0;k<FRACT_N;k++) {
        uint64_t h=fd_render_test(k,81991,-1,0,60,1);
        assert(h==fd_render_test(k,81991,-1,0,60,0));
        assert(h!=fd_render_test(k,81992,-1,0,60,0));
        for(unsigned m=0;m<4;m++)assert(fd_render_test(k,81991,m,-48,60,0)!=fd_render_test(k,81991,m,48,60,0));
    }
    for(unsigned k=0;k<FRACT_N;k++)for(unsigned mask=0;mask<16;mask++)for(unsigned note=0;note<128;note+=127) {
        rus_t c;rus_design_t d;int32_t out[CTL];rus_design(&d,k,mask,0,GEN_NOW);rus_init(&c,0);
        for(unsigned m=0;m<4;m++)c.mac[m]=(mask&(1u<<m))?64:-64;
        rus_note_on(&c,&d,note);d.out=0x7fffffff; /* old calibrated gain must stay bounded */
        for(unsigned b=0;b<2200;b++) {
            if(b==2000)rus_release(&c);rus_render(&c,&d,out);
            for(unsigned i=0;i<CTL;i++)assert(abs(out[i])<=30720);
        }
        assert(!c.live&&!c.nheld);
    }
    fd_sine(0);fd_sine(64);fd_sine(117);fd_sine(128);
    rus_t c;rus_design_t d;int32_t out[CTL];rus_design(&d,1,1,0,GEN_NOW);rus_init(&c,0);
    c.mac[1]=64;c.mac[2]=-64;c.mac[3]=64;rus_note_on(&c,&d,60);
    for(unsigned b=0;b<2000;b++)rus_render(&c,&d,out);
    assert(c.events==(2000*CTL-1)/FD_PERIOD[128]+1 && !c.dropped);
    for(unsigned n=0;n<16;n++)rus_note_on(&c,&d,48+n);
    assert(c.nheld==8&&c.base==63*16);rus_note_off(&c,63);assert(c.base==62*16);
    rus_release(&c);for(unsigned b=0;b<300;b++)rus_render(&c,&d,out);assert(!c.live);
    /* Bipolar click kernels: mean relative to peak below 0.1%, no stuck DC. */
    for(unsigned len=4;len<=2048;len*=2) {
        rus_init(&c,0);c.live=1;c.nheld=1;c.env=Q15*256;c.until=100000;
        fd_fire(&c,1000*256,len,1);long sum=0;int peak=0;
        for(unsigned n=0;n<len;n+=CTL){rus_render(&c,&d,out);for(unsigned i=0;i<CTL;i++){sum+=out[i];if(abs(out[i])>peak)peak=abs(out[i]);}}
        assert(peak>0&&fabs((double)sum/len)<peak*0.001+1);
    }
    printf("FRACT DATA: 6 families, repeatable seeds, 4 macros, 192 corner cases, bounded gain, sample clock, note stack and click DC OK. runtime=%zu bytes/track\n",sizeof(rus_t));
}
