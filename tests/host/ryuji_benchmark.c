#define main host_main
#include "host.c"
#undef main
#include <time.h>
int main(void) {
 int32_t out[CTL];ba_design_t d;volatile uint32_t sink=0;
 for(unsigned family=1;family<=2;family++)for(unsigned pad=0;pad<6;pad+=2) {
  ba_design(&d,(int)family+5,123,PATH_OPEN,GEN_NOW);clock_t start=clock();
  for(unsigned rep=0;rep<400;rep++) {ba_voice_t v;ba_init(&v,1);ba_note_on(&v,&d,48+pad,127,1);
   for(unsigned b=0;b<128;b++){ba_render(&v,&d,out,CTL);sink+=out[b%CTL];}}
  printf("HOST %u pad %u: %.2f ns/sample\n",family,pad,1e9*(clock()-start)/CLOCKS_PER_SEC/(400.0*128*CTL));
 }return sink==0x12345678;
}
