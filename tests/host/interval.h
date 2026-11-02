/* Behavioral tests for the extension. Original sound/sequence tests stay in host.c. */
static uint32_t interval_gr_hash(int side, int parameter)
{
    uint32_t h = 2166136261u, seed = 123u;
    gran_init();
    if (parameter >= 0) gr_edit[0][parameter] = 80;
    for (unsigned b = 0; b < 2000; b++) {
        int32_t x[CTL];
        for (unsigned i = 0; i < CTL; i++) {
            seed = seed * 1664525u + 1013904223u;
            x[i] = (int32_t)(seed >> 18) - 8192;
        }
        gran_block(0, x, CTL, side * 40, 1);
        for (unsigned i = 0; i < CTL; i++) h = (h ^ (uint32_t)x[i]) * 16777619u;
    }
    return h;
}
static int cmd_interval(void)
{
    unsigned fail = 0;
#define IC(ok, msg) do { if (!(ok)) { printf("FAIL interval: %s\n", msg); fail++; } } while (0)
    const unsigned rates[] = {25, 50, 97, 100, 103, 200, 400};
    for (unsigned a = 0; a < sizeof rates / sizeof *rates; a++) {
        memset(loop, 0, sizeof loop);
        for (unsigned l = 0; l < NLAYER; l++) {
            loop[l].play = 1; loop[l].ntrk = 2;
            loop[l].speed_pct = l ? 100 : rates[a];
            for (unsigned k = 0; k < 2; k++) {
                loop[l].trk[k].len = 1000000;
                loop[l].trk[k].dir = 1;
                loop[l].trk[k].speed_pct = k ? 150 : 100;
            }
        }
        for (unsigned b = 0; b < 10000; b++) loops_run();
        for (unsigned l = 0; l < NLAYER; l++) {
            unsigned expected = l ? 10000 : rates[a] * 100;
            IC(abs(loop[l].trk[0].pos - (int)expected) <= 2, "track clock has wrong duration");
            IC(abs(loop[l].trk[1].pos - (int)(expected * 3 / 2)) <= 3, "independent loop clock has wrong duration");
        }
    }
    memset(loop, 0, sizeof loop);
    layers_init(); ui_init(); sounds_init();
    memset(host_flash, 0xFF, sizeof host_flash); ses_scan(1);
    press_btn(B_FX);
    press_btn(B_REC); press_key(0, 1); run_blocks(100, 0);
    press_key(0, 0); run_blocks(100, 0); press_btn(B_REC); run_blocks(1, 0);
    IC(loop[0].ntrk == 1, "free recording");
    press_phys(KEY_HALF, 1); press_phys(KEY_HALF, 0);
    IC(ui.speed_page, "speed page opens");
    host_enc[PANEL_ENC[EN_K1]] = 3; ui_input(); run_blocks(1, 0);
    IC(loop[0].speed_pct > 100 && loop[1].speed_pct == 0, "knob changes selected track only");
    ev_post(EV_LOOP_SPEED, 0, LOOP_TRK, 137); ev_post(EV_LOOP_SPEED, 0, 0, 73); run_blocks(1, 0);
    unsigned period = lm_period(0), len = loop[0].trk[0].len;
    IC(abs((int)period - (int)(len * 10000 / (137 * 73))) <= 3, "combined clocks preserve loop timing");
    ui.force = 1; ui_draw(); ppm("build/screens/interval-speed.ppm");
    press_btn(B_FX);
    IC(!ui.speed_page, "track selection closes speed page");
    press_phys(KEY_SHIFT, 1);
    host_enc[PANEL_ENC[EN_K2]] = 1; ui_input();
    press_phys(KEY_SHIFT, 0);
    IC(ui.gr_page && !ui.gr[0], "SHIFT + GRAINS opens parameters without changing effect amount");
    host_enc[PANEL_ENC[EN_K1]] = 25; ui_input();
    host_enc[PANEL_ENC[EN_K2]] = -18; ui_input();
    host_enc[PANEL_ENC[EN_K3]] = 42; ui_input();
    host_enc[PANEL_ENC[EN_K4]] = 16; ui_input();
    IC(gr_edit[0][0] == 25 && gr_edit[0][1] == -18 && gr_edit[0][2] == 42 && gr_edit[0][3] == 16, "four granular controls");
    IC(!gr_edit[1][0] && !ui.dist[0] && !ui.dsend[0], "granular controls are per track and do not change other effects");
    ui.force = 1; ui_draw(); ppm("build/screens/interval-grains.ppm");
    press_btn(B_FX);
    IC(!ui.gr_page, "track selection leaves granular page");
    IC(snap_store(0), "snapshot store");
    ev_post(EV_LOOP_SPEED, 0, LOOP_TRK, 200); ev_post(EV_LOOP_SPEED, 0, 0, 200); run_blocks(1, 0);
    snap_recall(0, 0); run_blocks(1, 0);
    IC(loop[0].speed_pct == 137 && loop[0].trk[0].speed_pct == 73, "snapshot restores both speeds");
    IC(ses_save(1) == SES_OK, "session save");
    loop[0].speed_pct = 200; loop[0].trk[0].speed_pct = 200;
    IC(ses_load(0) == SES_OK, "session load");
    IC(loop[0].speed_pct == 137 && loop[0].trk[0].speed_pct == 73, "session restores both speeds");
    loop[0].speed_pct = 200;
    snap_recall(0, 0); run_blocks(1, 0);
    IC(loop[0].speed_pct == 137, "session keeps snapshot speed");
    IC(gr_edit[0][0] == 25 && gr_edit[0][3] == 16, "session and snapshot keep granular parameters");
    clip_copy(0); clip_paste(1); run_blocks(1, 0);
    IC(loop[1].speed_pct == 137 && loop[1].trk[0].speed_pct == 73, "copy keeps clocks");
    IC(gr_edit[1][2] == 42, "copy keeps granular parameters");
    const char *labels[] = {"SATURATION 1", "INHARMONIC 6", "BRICKLAYER", "GRAINS", "FEEDBACK", "LOOP SPEED", "ARPEGGIO", "INSTABILITY", "BRIGHTNESS"};
    IC(FONT_XL.h <= 98 && FONT_L.h <= 48 && FONT_M.h <= 44 && FONT_XS.h <= 16,
       "font heights fit the existing firmware bands");
    for (unsigned i = 0; i < sizeof labels / sizeof *labels; i++)
        IC(text_w(text_fit_font(&FONT_L, labels[i], 232), labels[i]) <= 232, "label fits the display");
    loop_cmd(0, EV_LOOP_CLEAR);
    IC(loop[0].speed_pct == 100, "clear resets track clock");
    for (int side = -1; side <= 1; side += 2) {
        uint32_t baseline = interval_gr_hash(side, -1);
        for (int p = 0; p < 4; p++)
            IC(interval_gr_hash(side, p) != baseline, "granular parameter must audibly alter the DSP output");
    }
    printf("interval: %s\n", fail ? "FAILED" : "ok");
    return fail ? 1 : 0;
#undef IC
}
