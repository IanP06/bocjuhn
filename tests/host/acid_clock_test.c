#define main host_original_main
#include "host.c"
#undef main
#include <assert.h>
static void pump_blocks(unsigned n) {while(n--)host_pump();}
static unsigned voices(unsigned l) {unsigned n=0;for(unsigned k=0;k<NVOICE;k++)n+=vox[k].gate && vox[k].layer==l;return n;}
static int current(unsigned l) {for(unsigned k=0;k<NVOICE;k++)if(vox[k].gate && vox[k].layer==l)return vox[k].note;return -1;}
int main(void) {
    grain_reset();
    for(unsigned l=0;l<6;l++)for(unsigned t=0;t<3;t++)assert(!clk.mode[l][t]);
    assert(clk.bpm==120);
    /* No wall-clock/UI dependency or cumulative millisecond rounding. */
    for(unsigned bpm=30;bpm<=300;bpm+=27) {
        clock_defaults();clk.bpm=bpm;
        unsigned blocks=100000;
        for(unsigned i=0;i<blocks;i++)clock_advance();
        uint64_t expected=(uint64_t)blocks*32*bpm*960/2646000;
        assert(clock_tick==expected);
    }
    grain_reset();
    lay[1].engine=lay[1].want_engine=ENG_ANIMA;
    note_vel=100;note_on(1,48,SRC_KEY);pump_blocks(4);
    assert(voices(1)==1 && current(1)==48);
    note_on(1,60,SRC_KEY);pump_blocks(4);
    assert(voices(1)==1 && current(1)==60);
    note_off(1,60,SRC_KEY);pump_blocks(4);assert(current(1)==48);
    note_off(1,48,SRC_KEY);pump_blocks(400);assert(!voices(1));
    /* Delayed onset must wait, and stop invalidates both future and active notes. */
    uint32_t due=clock_tick+240;
    clock_post(1,CK_BALL,55,120,due,120);
    host_pump();assert(!voices(1));
    while((int32_t)(clock_tick-due)<0)host_pump();
    host_pump();assert(current(1)==55);
    clock_cancel[1][CK_BALL]++;host_pump();assert(!voices(1));
    clock_post(1,CK_BRICK,60,110,clock_tick+480,120);
    clock_cancel[1][CK_BRICK]++;pump_blocks(800);assert(!voices(1));
    /* Loop view must not edit its events. */
    looper_t *lp=&loop[1];lp->ntrk=1;lp->play=1;lp->nev=2;
    lp->trk[0].first=0;lp->trk[0].n=2;lp->trk[0].len=2756;
    lp->ev[0]=LE(83,48,1);lp->ev[1]=LE(1300,48,0);
    uint32_t original[2]={lp->ev[0],lp->ev[1]};
    clk.mode[1][CK_LOOP]=1;clk.div[1][CK_LOOP]=3;
    pump_blocks(6000);assert(!memcmp(original,lp->ev,sizeof original));
    loop_cmd(1,EV_LOOP_PLAY);host_pump();assert(!voices(1));
    /* UI page traversal and controls. */
    ui.sel=1;ui.clock_page=1;ui.clock_target=CK_BRICK;
    host_enc[PANEL_ENC[EN_K1]]=8;ui_input();assert(clk.bpm==128);
    host_enc[PANEL_ENC[EN_K3]]=1;ui_input();assert(clk.mode[1][CK_BRICK]);
    host_enc[PANEL_ENC[EN_K4]]=1;ui_input();assert(clk.div[1][CK_BRICK]==4);
    ui.force=1;ui_draw();ppm("build/screens/clock.ppm");
    ui.clock_page=0;ui.acid_page=1;ui.force=1;ui_draw();ppm("build/screens/acid.ppm");
    /* Save/load clock and extended ACID settings. */
    ses_scan(1);acid_edit[1][2]=123;bl_accent[1][0]=1;bl_slide[1][0]=1;
    assert(ses_save(12)==SES_OK);
    clk.bpm=90;acid_edit[1][2]=1;
    assert(ses_load_slot(12)==SES_OK);
    assert(clk.bpm==128 && clk.mode[1][CK_BRICK] && clk.div[1][CK_BRICK]==4);
    assert(acid_edit[1][2]==123);
    assert(bl_accent[1][0] && bl_slide[1][0]);
    assert(snap_store(0));clk.bpm=88;acid_edit[1][2]=9;bl_slide[1][0]=0;
    snap_recall(0,0);assert(clk.bpm==128 && acid_edit[1][2]==123 && bl_slide[1][0]);
    /* Independent FREE/SYNC targets, two-bar recording and immediate first repeat. */
    grain_reset();clk.mode[0][CK_LOOP]=1;clk.bars[0]=2;
    loop_cmd(0,EV_LOOP_REC);assert(loop[0].rec==LR_ARMED);
    while(loop[0].rec==LR_ARMED)host_pump();
    loop_hear(0,1,48);pump_blocks(80);loop_hear(0,0,48);
    loop_cmd(0,EV_LOOP_REC);assert(loop[0].rec==LR_REC);
    while(loop[0].rec==LR_REC)host_pump();
    assert(loop[0].ntrk==1 && clk.take_bars[0][0]==2);
    assert(!clk.mode[0][CK_BALL] && !clk.mode[0][CK_BRICK]);
    assert(loop_sync[0][0].start<=clock_tick && clock_tick-loop_sync[0][0].start<8);
    /* Complete-note queue overflow cannot strand a release. */
    grain_reset();
    for(unsigned i=0;i<CLOCK_Q+20;i++)clock_post(1,CK_BALL,48+i%12,110,clock_tick+20,30);
    assert(clock_dropped>=20);pump_blocks(500);assert(!voices(1));
    /* Exercise and render the integrated core, not the earlier standalone sketch. */
    an_design_t design;an_voice_t voice;int32_t out[CTL];
    an_design(&design,0,123,PATH_OPEN,GEN_NOW);an_init(&voice,1);
    const int phrase[16]={36,36,48,39,36,51,43,39,60,63,67,72,63,60,55,48};
    wav_ch=1;wav_n=0;
    for(unsigned block=0;block<11024;block++) {
        unsigned step=block/172,at=block%172;
        if(!at){voice.ktmul=step%4==0;an_note_on(&voice,&design,pitch_inc(phrase[step%16]*16),step%4!=2);}
        if(at==125 && (step+1)%4!=2)an_note_off(&voice);
        design.tableA=block>=5512;
        an_render(&voice,&design,out,CTL);
        for(unsigned j=0;j<CTL;j++)wav_put(out[j]);
    }
    wav_write("build/rc5/acid-demo.wav");
    printf("ACID/CLOCK PASS: drift, mono priority, queue cancellation, non-destructive loop, UI, persistence\n");
    return 0;
}
