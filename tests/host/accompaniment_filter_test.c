/* SPDX-License-Identifier: GPL-3.0-only */
#define main host_original_main
#include "host.c"
#undef main
#include <assert.h>
#include <time.h>
static void loop_guide(void) {
    looper_t *p=&loop[0];p->ntrk=1;p->play=1;p->nev=8;
    loop_trk_t *t=&p->trk[0];t->first=0;t->n=8;t->len=1600;
    for(unsigned i=0;i<4;i++){p->ev[2*i]=LE(i*400,60+i*2,1);p->ev[2*i+1]=LE(i*400+250,60+i*2,0);}
    loop_rewind(p);
}
static double response(int mode,int poles,int hz,int q) {
    svf_t a={0},b={0};svf_c ca,cb;svf_coef(&ca,1000,q);svf_coef(&cb,1000,2896);
    double sum=0;uint32_t ph=0,inc=(uint32_t)((uint64_t)hz*4294967296ull/44100);
    for(unsigned i=0;i<44100;i++) {
        ph+=inc;int32_t y=svf_run(&a,&ca,sin_ph(ph)/4,mode);
        if(poles==4)y=svf_run(&b,&cb,y,mode);
        assert(abs(y)<2000000 && abs(a.ic1)<4000000 && abs(b.ic1)<4000000);
        if(i>=22050)sum+=(double)y*y;
    }
    return sqrt(sum/22050)/5792.6;
}
int main(void) {
    for(unsigned mode=0;mode<RXM_N;mode++) {
        grain_reset();ui.sel=0;rx.mask=1u<<4;rx_mode_set(mode);rx_set(1);loop_guide();
        rx.k[0]=100;rx.ak[5]=100;
        pa_run(5000);unsigned heard=rx.hn;
        assert(heard>0 && rx.guide==0);
        ui.rx_page=0;press_btn(B_EDIT);pa_run(10);
        assert(ui.sel==4 && rx.on && rx.guide==0 && rx_ear==0 && rx.hn>=heard);
        memset(pa_seen,0,sizeof pa_seen);memset(pa_prev,0,sizeof pa_prev);pa_run(7000);
        assert(pa_seen[4]>0 && !pa_seen[0]);
        press_btn(B_ENV);pa_run(10);assert(ui.sel==2 && rx.guide==0 && rx_ear==0);
        ui.rx_page=1;ui.force=1;ui_leds();assert(host_led[PANEL_BTN[B_FX]]);
        unsigned mask=rx.mask;rx_pad_hit(0);assert(rx.mask==mask && !strcmp(ui.msg,"GUIDE TRACK"));
        press_phys(KEY_SHIFT,1);press_btn(B_SEL);press_phys(KEY_SHIFT,0);
        assert(rx.guide==1 && rx_ear==1 && ui.sel==1 && !rx.hn);
        rx_set(0);assert(rx_ear==255 && !rx_q_busy());
        printf("%s: guide loop continues on follower and third-track selection PASS\n",RXM_NAME[mode]);
    }
    grain_reset();ses_scan(1);ui.sel=0;rx_set(1);ui.sel=4;
    flt_edit[4][0]=6;flt_edit[4][1]=101;assert(snap_store(0));assert(ses_save(23)==SES_OK);
    rx.guide=2;rx_reset();flt_edit[4][0]=0;
    assert(ses_load_slot(23)==SES_OK && rx.guide==0 && rx_ear==0 && flt_edit[4][0]==6);
    rx.guide=2;rx_reset();snap_recall(0,0);assert(rx.guide==0 && rx_ear==0 && flt_edit[4][0]==6);
    /* Old RC8 records have no pinned guide extension: use the saved selection. */
    unsigned base=SES_LO+ses.sect[ses_find(23)]*SES_SECT;ses_head_t old;
    memcpy(&old,host_flash+base,sizeof old);old.format=32772;old.len-=1+sizeof snap_rxguide;
    old.body_crc=ses_crc(host_flash+base+sizeof old,old.len-sizeof old,0);
    old.head_crc=ses_crc((const uint8_t*)&old,30,0);memcpy(host_flash+base,&old,sizeof old);
    assert(ses_load_slot(23)==SES_OK && rx.guide==ui.sel);
    for(int mode=1;mode<=3;mode++)for(int q=2896;q<=49152;q+=11564)for(int hz=20;hz<=18000;hz*=3)response(mode,4,hz,q);
    double lp12=response(1,2,8000,2896),lp24=response(1,4,8000,2896);
    double hp12=response(3,2,125,2896),hp24=response(3,4,125,2896);
    assert(lp24<lp12/8 && hp24<hp12/8);
    double lowq=response(1,2,1000,16384),highq=response(1,2,1000,49152);assert(highq>lowq*2.8);
    printf("Filter stopband: LP12 %.2f dB, LP24 %.2f dB; HP12 %.2f dB, HP24 %.2f dB; resonance Q4->Q12 %.2f dB\n",20*log10(lp12),20*log10(lp24),20*log10(hp12),20*log10(hp24),20*log10(highq/lowq));
    /* Exercise the complete six-track mixer through every filter and cutoff sweep. */
    for(unsigned type=0;type<=6;type++) {
        grain_reset();uint64_t energy=0;
        for(unsigned l=0;l<NLAYER;l++) {ui.sel=l;flt_edit[l][0]=type;flt_edit[l][1]=101;press_key(l*2,1);}
        for(unsigned b=0;b<4000;b++) {
            for(unsigned l=0;l<NLAYER;l++){ui.flt[l]=(int32_t)(b%(2*FLT_MAX+1))-FLT_MAX;level_pan_apply(l);}
            host_pump();
            for(unsigned j=0;j<2*CTL;j++){assert(mix_out[j]>=-32768 && mix_out[j]<=32767);energy+=(int64_t)mix_out[j]*mix_out[j];}
        }
        assert(energy>0);
    }
    puts("Six-track full-mix cutoff sweeps, seven types at maximum resonance PASS");
    volatile int32_t checksum=0;
    for(unsigned poles=2;poles<=4;poles+=2) {
        svf_t a[6]={{0}},b[6]={{0}};svf_c c;svf_coef(&c,2000,2896);clock_t start=clock();
        for(unsigned block=0;block<100000;block++)for(unsigned l=0;l<6;l++)for(unsigned i=0;i<32;i++) {
            int32_t y=svf_run(&a[l],&c,((block+i)&127)*100-6400,1);
            if(poles==4)y=svf_run(&b[l],&c,y,1);checksum^=y;
        }
        printf("Host benchmark, six tracks %u poles: %.3f seconds (not FM1 timing)\n",poles,(double)(clock()-start)/CLOCKS_PER_SEC);
    }
    puts("Accompaniment, persisted guide, RC8 migration, filter slopes and resonance PASS");return checksum==123456789;
}
