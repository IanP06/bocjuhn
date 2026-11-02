#define main host_original_main
#include "host.c"
#undef main
#include <assert.h>

static uint32_t screen_hash(void){uint32_t h=2166136261u;for(unsigned i=0;i<240*240;i++)h=(h^fb[i])*16777619u;return h;}
int main(void){
    grain_reset();
    uint32_t hashes[NENG];
    for(unsigned e=0;e<NENG;e++){
        ui.sel=0;ui.engine[0]=e;lay[0].meter=16000;ui.force=1;
        ui_draw();hashes[e]=screen_hash();
        char p[100];snprintf(p,sizeof p,"build/rc7/screens/engine-%u.ppm",e);ppm(p);
        uint32_t seed=ui.seed;
        fm1_ms+=84;ui_draw();assert(ui.seed==seed);assert(hashes[e]!=screen_hash());
        ui.engine[1]=e;assert(ui_color(0)==ui_color(1));
    }
    for(unsigned e=0;e<NENG;e++)for(unsigned j=0;j<e;j++)assert(hashes[e]!=hashes[j]);
    /* Quiet tracks must visibly animate too; hash only the organism area. */
    for(unsigned e=0;e<NENG;e++){
        ui.sel=0;ui.engine[0]=e;living_energy=0;lay[0].meter=256;ui.force=1;ui_draw();
        uint32_t before=2166136261u,after=2166136261u;
        for(int y=28;y<140;y++)for(int x=56;x<184;x++)before=(before^fb[y*240+x])*16777619u;
        fm1_ms+=84;ui_draw();
        for(int y=28;y<140;y++)for(int x=56;x<184;x++)after=(after^fb[y*240+x])*16777619u;
        assert(before!=after);
        for(unsigned frame=0;frame<36;frame++){
            lay[0].meter=256+(frame%12)*600;fm1_ms+=84;ui_draw();
            char p[120];snprintf(p,sizeof p,"build/rc7/screens/motion-%u-%02u.ppm",e,frame);ppm(p);
        }
    }
    ui.sel=0;ui.engine[0]=ENG_CAL;lay[0].meter=0;
    for(unsigned i=0;i<40;i++){fm1_ms+=84;ui_draw();}
    uint32_t still=screen_hash(),phase=living_phase;
    fm1_ms+=1000;ui_draw();assert(screen_hash()==still && living_phase==phase);
    ui.engine[0]=ENG_ANIMA;ui.dial=DIAL_MAC;ui.mac[0][ENG_ANIMA][0]=32;ui.dial_until=fm1_ms+1000;ui.force=1;ui_draw();
    ppm("build/rc7/screens/dial.ppm");assert(ui.shown_page==4);
    fm1_ms+=1001;ui_draw();assert(ui.shown_page==0);
    ui.acid_page=1;ui.force=1;ui_draw();ppm("build/rc7/screens/acid-menu.ppm");ui.acid_page=0;
    ui.filter_page=1;ui.force=1;ui_draw();ppm("build/rc7/screens/filter.ppm");ui.filter_page=0;
    ui.clock_page=1;ui.force=1;ui_draw();ppm("build/rc7/screens/clock.ppm");
    puts("RC7 living UI: eight engines, engine colors across tracks, motion, idle freeze, RNG isolation, dial return and menu screenshots PASS");
    return 0;
}
