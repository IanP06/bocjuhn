/* Real production code; the cost model below tests admission, not FM1 timing. */
static void bocjuhn_checks(void)
{
    int32_t out[CTL]; uint64_t hashes[4][2];
    for (unsigned family=0;family<CS_N;family++) {
        cal_design_t d; cal_t c;
        cal_design(&d,family,17031,PATH_OPEN,GEN_NOW); cal_init(&c,19);
#ifdef DROPLETS_PARTICLES
        c.mac[2]=-MAC_MAX; c.mac[1]=-MAC_MAX; /* new TIMBRE / HARMONICS */
#else
        c.mac[0]=-MAC_MAX; c.mac[3]=-MAC_MAX;
#endif
        cal_note_on(&c,&d,48,1); cal_render(&c,&d,out);
        int peak=0; for(unsigned i=0;i<CTL;i++) if(abs(out[i])>peak)peak=abs(out[i]);
        assert(peak>5); /* immediate sound, no stochastic wait */
        uint32_t pitch=c.inc[0]; cal_note_on(&c,&d,60,1);
        assert(c.inc[1]>=pitch*2-2 && c.inc[1]<=pitch*2+2);
        cal_release(&c,1);
        unsigned b=0; while(c.live && b++<24000)cal_render(&c,&d,out);
        assert(!c.live && !c.nheld);
    }
    for(unsigned m=0;m<4;m++)for(unsigned side=0;side<2;side++) {
        cal_design_t d; cal_t c; cal_design(&d,CS_THORN,17031,PATH_OPEN,GEN_NOW); cal_init(&c,19);
        c.mac[m]=side?MAC_MAX:-MAC_MAX; cal_note_on(&c,&d,55,1);
        uint64_t h=1469598103934665603ull;
        for(unsigned b=0;b<2756;b++) {
            cal_render(&c,&d,out);
            for(unsigned i=0;i<CTL;i++) {assert(abs(out[i])<200000);h=(h^(uint32_t)out[i])*1099511628211ull;}
        }
        hashes[m][side]=h;
    }
    for(unsigned m=0;m<4;m++)assert(hashes[m][0]!=hashes[m][1]);
    puts("DROPLETS: 10 families, immediate impulses, octave ratio, four audible macro changes and finite release OK");
    layers_init(); ui_init(); sounds_init(); grain_reset(); ses_scan(1);
    ui.sel=0; ui.edit=0; fm1_in.notes=1u<<KEY_SHIFT; host_enc[PANEL_ENC[EN_K1]]++;
    ui_input(); assert(ui.filter_page && !ui.gr_page);
    fm1_in.notes=0; host_enc[PANEL_ENC[EN_K2]]+=31; host_enc[PANEL_ENC[EN_K3]]+=2; ui_input();
    assert(flt_edit[0][0]==2 && flt_edit[0][1]==31);
    ui.force=1; ui_draw(); ppm("build/screens/bocjuhn-filter.ppm");
    fm1_in.notes=1u<<KEY_SHIFT; host_enc[PANEL_ENC[EN_K1]]++; ui_input(); assert(!ui.filter_page);
    host_enc[PANEL_ENC[EN_K2]]++; ui_input(); assert(ui.gr_page);
    host_enc[PANEL_ENC[EN_K2]]++; ui_input(); assert(!ui.gr_page);
    fm1_in.notes=0; ui_input();
    assert(ses_save(1)==SES_OK); memset(flt_edit,0,sizeof flt_edit);
    assert(ses_load_slot(1)==SES_OK && flt_edit[0][0]==2 && flt_edit[0][1]==31);
    snap_store(0); flt_edit[0][0]=0; flt_edit[0][1]=0;
    assert(snap_filter[0][0][0]==2 && snap_filter[0][0][1]==31);
    assert(ses_save(2)==SES_OK); memset(snap_filter,0,sizeof snap_filter);
    assert(ses_load_slot(2)==SES_OK && snap_filter[0][0][1]==31);
    /* Rebuild the exact previous-format record (same body minus the new tail),
     * with its valid checksums. Old sessions must default to the original filter. */
    unsigned base=SES_LO+ses.sect[ses_find(2)]*SES_SECT;
    ses_head_t old; memcpy(&old,host_flash+base,sizeof old);
    old.format=32769; old.len-=sizeof flt_edit+sizeof snap_filter[0];
    old.body_crc=ses_crc(host_flash+base+sizeof old,old.len-sizeof old,0);
    old.head_crc=ses_crc((const uint8_t *)&old,30,0);
    memcpy(host_flash+base,&old,sizeof old);
    assert(ses_load_slot(2)==SES_OK && !flt_edit[0][0] && !flt_edit[0][1]);
    puts("FILTER: SHIFT routing, GRAINS preserved, session and scene persistence OK");
    layers_init(); ui_init(); sounds_init(); grain_reset();
    host_cost_model[ENG_FM]=180; eng_cost[ENG_FM]=180; lay[0].engine=lay[0].want_engine=ENG_FM;
    overload_skipped=0;
    for(unsigned n=0;n<12;n++)note_on(0,48+n,SRC_KEY);
    unsigned active=0;for(unsigned k=0;k<NVOICE;k++)active+=vox[k].sounding;
    assert(active<=4 && active>0 && overload_skipped>0);
    run_blocks(32,0); assert(blk_last<=BUDGET);
    for(unsigned n=0;n<EVQ;n++)ev_post(EV_ON,0,100,127);
    ev_post(EV_OFF,0,48,EV_UNHEARD); assert(ev_off_lost[0][1] & (1u<<16));
    events_take();
    for(unsigned k=0;k<NVOICE;k++)assert(!(vox[k].gate && vox[k].layer==0 && vox[k].note==48));
    /* Interrupt between the producer's heard flag and its release publication. */
    ev_off_heard[0][1]=1u<<16;events_take();assert(ev_off_heard[0][1] & (1u<<16));
    ev_off_lost[0][1]=1u<<16;events_take();assert(!ev_off_heard[0][1]);
    for(unsigned n=0;n<12;n++)note_off(0,48+n,SRC_KEY);
    memset(host_cost_model,0,sizeof host_cost_model);
    puts("Overload model: simultaneous note burst bounded before render; note-offs preserved (not a hardware benchmark)");
    layers_init(); ui_init(); sounds_init(); grain_reset();
    unsigned worst=0, audible=0;
    host_cost_model[ENG_FM]=eng_cost[ENG_FM]=100;
    host_cost_model[ENG_ANIMA]=eng_cost[ENG_ANIMA]=150;
    host_cost_model[ENG_CAL]=eng_cost[ENG_CAL]=200;
    for(unsigned l=0;l<3;l++) {
        bb_toggle(l); bb.n[l]=BB_MAX; bb.speed[l]=90; bb.unst[l]=55; bb_start(l); bb_set_run(l,1);
        ui.chord_on[l]=1;ui.chord_form[l]=0;
        ui.gr[l]=35;ui.dist[l]=40;ui.send[l]=65;ui.dsend[l]=55;level_pan_apply(l);
        lay_mod[l].lfo[0].rate=55;lay_mod[l].lfo[1].rate=70;
        lay_mod[l].s[0]=(mslot_t){MSRC_L1,MD_FLT,35};
        lay_mod[l].s[1]=(mslot_t){MSRC_L2,MD_MAC2,40};
    }
    for(unsigned b=0;b<12000;b++) {
        bb_tick();host_pump();if(blk_last>worst)worst=blk_last;
        for(unsigned i=0;i<CTL*2;i++){assert(abs(mix_out[i])<=32767);audible+=abs(mix_out[i])>10;}
    }
    assert(audible>100 && worst<=BLK_TICKS);
    for(unsigned l=0;l<3;l++)bb_set_run(l,0);
    run_blocks(64,0);
    for(unsigned k=0;k<NVOICE;k++)assert(!vox[k].gate);
    assert(!cal_rt[2].nheld);
    memset(host_cost_model,0,sizeof host_cost_model);
    printf("Stress: 3 BALLSY x 8 balls, chords, grains, distortion, sends and matrix; peak model cost %u/1000; no stuck gates OK\n",worst);
    layers_init(); ui_init(); sounds_init(); grain_reset();
    splash_draw("BOCJ UHN"); ppm("build/screens/bocjuhn-splash.ppm");
    ui.force=1; ui.dial_until=0; ui_draw(); ppm("build/screens/bocjuhn-home.ppm");
    assert(BB_X0==6 && BB_Y0==30 && BB_W==228 && BB_H==204);
    bb_toggle(0);bb.n[0]=4;bb_start(0);bb_set_run(0,1);
    for(unsigned b=0;b<700;b++){bb_tick();host_pump();}
    draw_bb_page(); ppm("build/screens/bocjuhn-ballsy.ppm");
    assert(!cv_dust_off);
    wav_ch=1;
    for(unsigned family=0;family<CS_N;family++) {
        cal_design_t d; cal_t c; cal_design(&d,family,17031,PATH_OPEN,GEN_NOW); cal_init(&c,19);
        cal_note_on(&c,&d,60,1);
        for(unsigned b=0;b<2756;b++) { if(b==1800)cal_release(&c,1);cal_render(&c,&d,out);for(unsigned i=0;i<CTL;i++)wav_put(out[i]); }
        for(unsigned i=0;i<4410;i++)wav_put(0);
    }
    wav_write("build/engines/droplets-families.wav");
    puts("bocjuhn checks: OK");
}
