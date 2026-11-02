#define main original_host_main
#include "host.c"
#undef main
int main(void){for(int f=0;f<CS_N;f++){cal_design_t d;cal_t c;cal_design(&d,f,731,PATH_OPEN,GEN_NOW);cal_init(&c,19);cal_note_on(&c,&d,48,1);uint64_t h=1469598103934665603ull;for(unsigned b=0;b<4000;b++){if(b==2000)cal_release(&c,1);int32_t out[CTL];cal_render(&c,&d,out);for(unsigned i=0;i<CTL;i++)h=(h^(uint32_t)out[i])*1099511628211ull;}printf("%d %llu\n",f,(unsigned long long)h);}return 0;}
