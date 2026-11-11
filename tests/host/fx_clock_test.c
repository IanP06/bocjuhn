/* SPDX-License-Identifier: GPL-3.0-only */
#define main host_original_main
#include "host.c"
#undef main
#include <assert.h>
static void pump(unsigned n){while(n--)host_pump();}
static unsigned gates(unsigned l){unsigned n=0;for(unsigned k=0;k<NVOICE;k++)n+=vox[k].gate && vox_owner(&vox[k])==l;return n;}
int main(void) {
    grain_reset();
    assert(clock_snap(161,240,1)==160);
    assert(clock_snap(239,240,1)==240);
    assert(clock_snap(321,240,1)==320);
    assert(clock_snap(357,360,0)==360);
    assert(clock_snap(157,0,0)==157);
    /* Same pitch, separately owned gates: short second note must not release
     * the first, and a retrigger must not truncate it either. */
    loop[0].play=1;
    clock_post(0,CK_LOOP,60,100,clock_tick,500);
    host_pump();assert(gates(0)==1);
    clock_post(0,CK_LOOP,60,100,clock_tick,70);
    host_pump();assert(gates(0)==2);
    pump(65);assert(gates(0)==1);
    pump(400);assert(gates(0)==0);
    grain_reset();
    /* Exactly one-bar source, heterogeneous gates, final note wraps past bar.
     * Only start times change; events and gate lengths stay intact. */
    looper_t *p=&loop[0];p->play=1;p->ntrk=1;p->nev=6;
    p->trk[0].first=0;p->trk[0].n=6;p->trk[0].len=3840;
    p->ev[0]=LE(111,60,1);p->ev[1]=LE(188,60,0);
    p->ev[2]=LE(319,62,1);p->ev[3]=LE(802,62,0);
    p->ev[4]=LE(3800,64,1);p->ev[5]=LE(3838,64,0);
    uint32_t original[6];memcpy(original,p->ev,sizeof original);
    clk.mode[0][CK_LOOP]=1;clk.div[0][CK_LOOP]=10;
    loop_sync_run(0);uint32_t start=loop_sync[0][0].start;
    clock_tick=start+120;loop_sync_run(0);
    unsigned found=0;
    for(unsigned i=0;i<CLOCK_Q;i++)if(clock_q[i].state && clock_q[i].e.note==60){assert(clock_q[i].e.due==start+160 && clock_q[i].e.len==77);found++;}
    /* Scheduling window opens 16 ticks before the onset. */
    assert(!found);clock_tick=start+150;loop_sync_run(0);
    for(unsigned i=0;i<CLOCK_Q;i++)if(clock_q[i].state && clock_q[i].e.note==60){assert(clock_q[i].e.due==start+160 && clock_q[i].e.len==77);found++;}
    assert(found==1);clock_tick=start+310;loop_sync_run(0);
    for(unsigned i=0;i<CLOCK_Q;i++)if(clock_q[i].state && clock_q[i].e.note==62)assert(clock_q[i].e.len==483 && clock_q[i].e.due==start+320);
    clock_tick=start+3830;loop_sync_run(0);found=0;
    for(unsigned i=0;i<CLOCK_Q;i++)if(clock_q[i].state && clock_q[i].e.note==64){assert(clock_q[i].e.len==38 && clock_q[i].e.due==start+3840);found++;}
    assert(found==1 && !memcmp(original,p->ev,sizeof original));
    grain_reset();
    /* UI: four FX pages, then MOD. PRISM has its own second page. */
    for(unsigned i=0;i<4;i++){host_press=1u<<PANEL_BTN[B_SEQ];ui_input();assert(ui.rv_page==(i%2+1));assert(ui.fx==(i<2));ui.force=1;ui_draw();}
    host_press=1u<<PANEL_BTN[B_SEQ];ui_input();assert(ui.mod_page);
    ui.mod_page=0;ui.gr_page=1;host_press=1u<<PANEL_BTN[B_SEQ];ui_input();assert(ui.gr_page==2);
    host_enc[PANEL_ENC[EN_K4]]=20;ui_input();assert(fx.grain[0][3]==20);
    ui.force=1;ui_draw();ppm("build/rc8/prism.ppm");
    /* Global modulation sums routes but never changes saved controls. */
    rv_param(40,60,30,50);dl_param(30,40,20,50);
    mst[0].off[MD_RSIZE]=20;mst[1].off[MD_RSIZE]=-5;
    mst[0].off[MD_GPITCH]=7;fx_update();
    assert(rv.scale_t==2048+55*3072/100 && rv_base[0]==40);
    assert(fx_g[0][4]==7 && fx.grain[0][0]==0);
    /* Persistence, snapshot and old-session defaults are append-only. */
    fx.rv[0]=3;fx.dl[0]=2;fx.dl[1]=3;fx.dl[2]=3;
    fx.grain[2][0]=-12;fx.grain[2][3]=70;
    assert(snap_store(0));fx_config saved=fx;
    fx_defaults();snap_recall(0,0);assert(!memcmp(&saved,&fx,sizeof fx));
    ses_scan(1);assert(ses_save(22)==SES_OK);fx_defaults();
    assert(ses_load_slot(22)==SES_OK && !memcmp(&saved,&fx,sizeof fx));
    unsigned base=SES_LO+ses.sect[ses_find(22)]*SES_SECT;
    ses_head_t old;memcpy(&old,host_flash+base,sizeof old);
    old.format=32771;old.len-=2*sizeof(fx_config)+1+sizeof snap_rxguide; /* current + one snapshot */
    old.body_crc=ses_crc(host_flash+base+sizeof old,old.len-sizeof old,0);
    old.head_crc=ses_crc((const uint8_t *)&old,30,0);memcpy(host_flash+base,&old,sizeof old);
    assert(ses_load_slot(22)==SES_OK);
    assert(fx.rv[0]==0 && fx.rv[3]==100 && fx.grain[2][3]==0 && fx.dl[1]==0);
    snap_recall(0,0);assert(fx.rv[0]==0 && fx.dl[1]==0);
    /* Impulses through every character. Finite fixed-point outputs, audible
     * returns, and stereo width zero really folds to mono. */
    int32_t in[CTL],l[CTL],r[CTL],send[CTL];
    for(int mode=0;mode<6;mode++) {
        rv_init();fx_defaults();fx.rv[0]=mode;fx.rv[3]=0;fx_update();
        uint64_t energy=0;
        for(unsigned b=0;b<5000;b++) {
            memset(in,0,sizeof in);memset(l,0,sizeof l);memset(r,0,sizeof r);
            if(b==0)in[0]=24000;
            rv_block(in,l,r,CTL);
            for(unsigned j=0;j<CTL;j++){assert(l[j]==r[j]);assert(l[j]>-2000000 && l[j]<2000000);energy+=(uint32_t)abs(l[j]);}
        }
        assert(energy>100);
    }
    for(int mode=0;mode<5;mode++)for(int rhythm=0;rhythm<4;rhythm++) {
        fx_defaults();fx.dl[0]=mode;fx.dl[1]=3;fx.dl[2]=rhythm;dl_param(20,65,30,80);fx_update();dl_init();
        uint64_t energy=0;
        for(unsigned b=0;b<2500;b++) {
            memset(in,0,sizeof in);memset(l,0,sizeof l);memset(r,0,sizeof r);memset(send,0,sizeof send);
            if(b<4)for(unsigned j=0;j<CTL;j++)in[j]=(j&1)?12000:8000;
            dl_block(in,l,r,send);
            for(unsigned j=0;j<CTL;j++){assert(abs(l[j])<100000 && abs(r[j])<100000);energy+=abs(l[j])+abs(r[j]);}
        }
        assert(energy>100);
    }
    for(int pitch=-24;pitch<=24;pitch+=12) {
        grain_reset();fx.grain[0][0]=pitch;fx.grain[0][1]=100;fx.grain[0][3]=100;
        gr_edit[0][0]=100;gr_edit[0][1]=70;fx_update();
        uint64_t energy=0;
        for(unsigned b=0;b<2500;b++) {
            for(unsigned j=0;j<CTL;j++)in[j]=sin_ph((b*CTL+j)*23000000u)/2;
            gran_block(0,in,CTL,0,1);
            for(unsigned j=0;j<CTL;j++){assert(abs(in[j])<1000000);energy+=abs(in[j]);}
        }
        fprintf(stderr,"PRISM pitch %d energy %llu frozen %u mix %d\n",pitch,(unsigned long long)energy,gran[0].frozen,fx_g[0][7]);
        assert(energy>100000);
        fx.grain[0][2]=1;fx_update();uint32_t write=gran[0].w;
        for(unsigned b=0;b<100;b++){memset(in,0,sizeof in);gran_block(0,in,CTL,0,0);}
        assert(gran[0].w==write && gran[0].frozen);
        fx.grain[0][2]=0;fx_update();gran_block(0,in,CTL,0,0);assert(gran[0].w!=write);
    }
    puts("FX/CLOCK PASS: owned overlapping gates, mixed grid, wrapped notes, UI, modulation, sessions, snapshots, 6 reverbs, 20 delay/rhythm combinations, PRISM pitch extremes/freeze");
    return 0;
}
