#define main original_host_main
#ifdef LEGACY_BLIPS
#include "../../build/upstream/tests/host/host.c"
#else
#include "host.c"
#endif
#undef main
int main(void)
{
    static rus_t c;rus_design_t d;int32_t out[CTL];
    for(unsigned p=0;p<3;p++)for(unsigned s=0;s<3;s++)for(unsigned note=36;note<=72;note+=12) {
#ifdef LEGACY_BLIPS
        rus_design(&d,RK_BLIPS,317+s*997,p,GEN_NOW);
#else
        rus_design(&d,0,317+s*997,p,GEN_NOW);
#endif
        rus_measure(&d);rus_init(&c,719+s);rus_note_on(&c,&d,note);
        uint64_t hash=1469598103934665603ull;
        for(unsigned b=0;b<6000;b++) {
            if(b==1600)rus_note_off(&c,note);memset(out,0,sizeof out);rus_render(&c,&d,out);
            for(unsigned i=0;i<CTL;i++)hash=(hash^(uint32_t)out[i])*1099511628211ull;
        }
        printf("%u %u %u %016llx\n",p,s,note,(unsigned long long)hash);
    }
}
