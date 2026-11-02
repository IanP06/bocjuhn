/* SPDX-License-Identifier: GPL-3.0-only
 * Browser hardware adapter. Sound, screen, controls and sessions are TONITOR C. */
#define main tonitor_test_main
#include "../tests/host/host.c"
#undef main
#include <emscripten.h>

static float web_audio[4096];
static int32_t web_state[56];
static uint32_t web_frame_at;

EMSCRIPTEN_KEEPALIVE uint8_t *sim_flash(void) { return host_flash; }
EMSCRIPTEN_KEEPALIVE uint32_t sim_flash_version(void) { return host_flash_writes + host_erases; }
EMSCRIPTEN_KEEPALIVE void sim_boot(void)
{
    grain_reset();
    ses_scan(1);
    setup_load();
    master_q12 = 1229; /* Start at 30 percent. */
    ui.force = 1;
    ui_draw(); ui_leds();
}
EMSCRIPTEN_KEEPALIVE void sim_key(int key, int down)
{
    if (key >= 0 && key < 27) press_phys((uint32_t)key, down != 0);
    ui_draw(); ui_leds();
}
EMSCRIPTEN_KEEPALIVE void sim_button(int label, int down)
{
    if (label < 0 || label >= NB) return;
    uint32_t mask = 1u << PANEL_BTN[label];
    if (down) { if (!(fm1_in.buttons & mask)) host_press |= mask; fm1_in.buttons |= mask; }
    else fm1_in.buttons &= ~mask;
    ui_input(); ui_draw(); ui_leds();
}
EMSCRIPTEN_KEEPALIVE void sim_encoder(int role, int delta)
{
    if (role < 0 || role >= NE) return;
    host_enc[PANEL_ENC[role]] += clampi(delta, -100, 100);
    ui_input(); ui_draw(); ui_leds();
}
EMSCRIPTEN_KEEPALIVE void sim_volume(int percent)
{ master_q12 = clampi(percent, 0, 100) * 4096 / 100; }
EMSCRIPTEN_KEEPALIVE void sim_release(void)
{
    fm1_in.notes = fm1_in.buttons = 0; host_press = 0;
    ui_input();
}
EMSCRIPTEN_KEEPALIVE void sim_panic(void)
{
    sim_release();
    for (uint32_t l = 0; l < NLAYER; l++) {
        if (bb.on[l]) bb_set_run(l, 0);
        if (bl.on[l]) bl_set_run(l, 0);
        if (loop[l].play) loop_cmd(l, EV_LOOP_PLAY);
        ev_post(EV_STOP, l, 0, 0);
    }
    rx.on = 0;
    memset(ui.latch, 0, sizeof ui.latch);
}
EMSCRIPTEN_KEEPALIVE float *sim_render(int frames)
{
    frames = clampi(frames, CTL, 2048) / CTL * CTL;
    for (int at = 0; at < frames; at += CTL) {
        if (!(at % 128)) ui_input();
        host_pump();
        for (unsigned i = 0; i < 2 * CTL; i++)
            web_audio[at * 2 + i] = (float)mix_out[i] / 32768.0f;
    }
    if (fm1_ms - web_frame_at >= 33) {
        web_frame_at = fm1_ms; ui_draw(); ui_leds();
    }
    return web_audio;
}
EMSCRIPTEN_KEEPALIVE uint16_t *sim_screen(void) { return fb; }
EMSCRIPTEN_KEEPALIVE uint8_t *sim_leds(void) { return host_led; }
EMSCRIPTEN_KEEPALIVE int32_t *sim_state(void)
{
    web_state[48]=ui.clock_page;web_state[49]=ui.acid_page;web_state[50]=clk.bpm;
    web_state[51]=ui.clock_target;web_state[52]=clk.mode[ui.sel][ui.clock_target];
    web_state[53]=clk.div[ui.sel][ui.clock_target];web_state[54]=clk.bars[ui.sel];
    web_state[0] = ui.sel; web_state[1] = ui.engine[ui.sel];
    web_state[2] = ui.edit; web_state[3] = ui.gr_page; web_state[4] = ui.speed_page;
    web_state[5] = ui.bb_page; web_state[6] = ui.bl_page; web_state[7] = ui.rx_page;
    web_state[8] = loop[ui.sel].ntrk; web_state[9] = loop_pct(loop[ui.sel].speed_pct);
    web_state[10] = loop_pct(loop[ui.sel].trk[ui.speed_loop].speed_pct);
    web_state[11] = ui.speed_loop; web_state[12] = ui.gr[ui.sel]; web_state[13] = fm1_ms;
    web_state[14] = master_q12 * 100 / 4096; web_state[15] = ses.n;
    for (int j = 0; j < 4; j++) web_state[16+j] = gr_edit[ui.sel][j];
    for (int l = 0; l < NLAYER; l++) {
        web_state[24+l] = loop[l].ntrk;
        web_state[30+l] = loop[l].play;
    }
    web_state[36] = bb.on[ui.sel]; web_state[37] = bl.on[ui.sel];
    web_state[38] = loop[ui.sel].rec; web_state[39] = rx.on;
    web_state[40] = ui.pre_page; web_state[41] = 0; web_state[42] = ui.pre_ix;
    return web_state;
}
