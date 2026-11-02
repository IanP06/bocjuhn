/* Independent double-precision SVF and full production engine stress tests. */
#define main original_host_main
#include "host.c"
#undef main
#include <assert.h>
static void filter_check(void) {
    double worst=0;
    for(unsigned fq=0;fq<6;fq++)for(unsigned qq=0;qq<5;qq++) {
        const double freq[]={16,55,220,1000,5000,11025},qs[]={1.5,4,32,256,1024.5};
        double f=freq[fq]/44100,q=qs[qq],g=f*(M_PI+0.3736*M_PI*M_PI*M_PI*f*f),r=1/q,h=1/(1+g*(g+r));
        double s1=0,s2=0,err=0,energy=0;dp_filter_t p={0};dp_coef(&p,(uint32_t)llround(f*DP_ONE),(int32_t)llround(r*DP_ONE));
        for(unsigned i=0;i<44100;i++) {
            double in=i==0?1:0,hp=(in-(r+g)*s1-s2)*h,bp=g*hp+s1;
            s1=g*hp+bp;double lp=g*bp+s2;s2=g*bp+lp;
            double b=dp_step(&p,i==0?(1<<18):0,0)/(double)(1<<18);
            err+=(bp-b)*(bp-b);energy+=bp*bp;
        }
        double rel=sqrt(err/energy);if(rel>worst)worst=rel;
        printf("SVF %g Hz Q=%g relative RMS error %.6f\n",freq[fq],q,rel);
        assert(rel<0.06); /* proposed engineering bound, not a perceptual fidelity claim */
    }
    printf("SVF worst relative RMS %.6f\n",worst);
}
static void render_demo(void) {
    wav_n=0;wav_ch=1;
    for(unsigned section=0;section<4;section++) {
        cal_design_t d;cal_t c;cal_design(&d,CS_THORN,17031,PATH_OPEN,GEN_NOW);cal_init(&c,19);
        d.touch=section==3?27000:14000;d.drift=section==0?0:23000;d.sharp[0]=section==0?4000:section==1?17000:27000;
        cal_note_on(&c,&d,section==2?43:60,1);
        for(unsigned b=0;b<5512;b++) {
            if(b==4134)cal_release(&c,1);
            int32_t out[CTL];cal_render(&c,&d,out);
            for(unsigned i=0;i<CTL;i++) wav_put(out[i]);
        }
    }
    wav_write("build/particles/droplets-dry.wav");
}
typedef struct {double g,r,h,s1,s2;} reference_filter;
static void reference_coef(reference_filter *p,double f,double q) {
 p->g=f*(M_PI+0.3736*M_PI*M_PI*M_PI*f*f);p->r=1/q;p->h=1/(1+p->g*(p->g+p->r));
}
static double reference_step(reference_filter *p,double x,int low) {
 double hp=(x-(p->r+p->g)*p->s1-p->s2)*p->h,bp=p->g*hp+p->s1;
 p->s1=p->g*hp+bp;double lp=p->g*bp+p->s2;p->s2=p->g*bp+lp;return low?lp:bp;
}
static void ab_reference(void) {
 const unsigned count=44100*8;int32_t *fixed=calloc(count,sizeof *fixed),*ref=calloc(count,sizeof *ref);
 double error=0,power=0;unsigned events=0; rnd_t rng;rnd_seed(&rng,821);
 dp_filter_t p[6]={0},post={0};reference_filter a[6]={0},ap={0};
 double f0=220./44100,q=64.5,t=.6,density=pow(261.6255653005986*pow(2,6*t*t)/48000,2)/6;
 dp_coef(&post,(uint32_t)llround(f0*DP_ONE),2*DP_ONE);reference_coef(&ap,f0,.5);
 for(unsigned n=0;n<count;n+=CTL) {
  double sum[CTL]={0};int32_t isum[CTL]={0};
  for(unsigned k=0;k<6;k++) {
   int changed=0;double gain=0;
   for(unsigned j=0;j<CTL;j++) {
    double u=rnd_next(&rng)/4294967296.;double input=0;
    int trigger=(n%22048==0&&j==0);
    if(trigger||u<density*48000/44100) {
     if(!changed) {
      double f=f0*pow(2,(2*rnd_next(&rng)/4294967296.-1)*2);if(f>.25)f=.25;
      reference_coef(&a[k],f,q);dp_coef(&p[k],(uint32_t)llround(f*DP_ONE),(int32_t)llround(DP_ONE/q));
      gain=.5/sqrt(q*(f*44100/48000)*sqrt(density));changed=1;
     }
     input=gain*(trigger?1:u/(density*48000/44100));events++;
    }
    isum[j]+=dp_step(&p[k],(int32_t)llround(input*(1<<18)),0);sum[j]+=reference_step(&a[k],input,0);
   }
  }
  for(unsigned j=0;j<CTL&&n+j<count;j++) {
   double x=reference_step(&ap,sum[j],1);int32_t y=dp_step(&post,isum[j],1);
   error+=pow(x-y/(double)(1<<18),2);power+=x*x;
   fixed[n+j]=y>>8;ref[n+j]=(int32_t)llround(x*1024);
  }
 }
 printf("A/B dry core: %u events, relative RMS error %.6f (same excitation, no LPG/diffuser)\n",events,sqrt(error/power));
 assert(sqrt(error/power)<.02);
 wav_ch=1;wav_n=0;for(unsigned n=0;n<count;n++)wav_put(ref[n]);wav_write("build/particles/A-float-reference.wav");
 wav_n=0;for(unsigned n=0;n<count;n++)wav_put(fixed[n]);wav_write("build/particles/B-fixed-core.wav");
 free(fixed);free(ref);
}
int main(void) {
    filter_check();
    long peak=0;unsigned cases=0;
    for(int note=0;note<=120;note+=24)for(int t=-64;t<=64;t+=64)for(int h=-64;h<=64;h+=64)for(int m=-64;m<=64;m+=64) {
        cal_design_t d;cal_t c;cal_design(&d,CS_THORN,17031,PATH_OPEN,GEN_NOW);cal_init(&c,19);
        c.mac[1]=h;c.mac[2]=t;c.mac[3]=m;cal_note_on(&c,&d,note,1);
        for(unsigned b=0;b<1400;b++) {
            int32_t out[CTL];cal_render(&c,&d,out);
            for(unsigned i=0;i<CTL;i++){long a=labs(out[i]);if(a>peak)peak=a;assert(a<32768);}
        }
        cal_release(&c,1);unsigned n=0;while(c.live&&n++<19000){int32_t out[CTL];cal_render(&c,&d,out);}assert(!c.live);cases++;
    }
    printf("%u extreme cases: peak %ld, bounded tails; cal_t=%zu bytes/track\n",cases,peak,sizeof(cal_t));
    cal_design_t d;cal_t c;cal_design(&d,0,77,0,GEN_NOW);cal_init(&c,9);
    cal_note_on(&c,&d,48,1);cal_note_on(&c,&d,48,2);cal_release(&c,1);assert(c.nheld==1&&c.nsrc[0]==2);cal_release(&c,2);assert(!c.nheld);
    assert(!strcmp(cal_mac_name(&d,0),"FREQUENCY")&&!strcmp(cal_mac_name(&d,1),"HARMONICS")&&!strcmp(cal_mac_name(&d,2),"TIMBRE")&&!strcmp(cal_mac_name(&d,3),"MORPH"));
    /* Repeated triggers and moving all four controls, including extremes. */
    cal_init(&c,9);cal_note_on(&c,&d,24,1);
    for(unsigned b=0;b<30000;b++) {
        for(unsigned j=0;j<4;j++)c.mac[j]=(int8_t)((b/(13+j*17))%129)-64;
        if(b%7==0)cal_land(&c,&d,0,1);
        int32_t out[CTL];cal_render(&c,&d,out);
        for(unsigned i=0;i<CTL;i++)assert(abs(out[i])<=32768);
    }
    puts("rapid triggers + moving controls: bounded output");
    ab_reference();render_demo();puts("particles: OK (host, not physical CPU timing)");return 0;
}
