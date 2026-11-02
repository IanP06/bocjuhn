/* SPDX-License-Identifier: GPL-3.0-only
 * Internal musical clock: audio-block owned, 960 ticks per quarter note.
 * FREE is zero, both on cold boot and when importing older sessions.
 */
#pragma once
enum { CK_LOOP, CK_BALL, CK_BRICK, CK_TARGETS };
typedef struct {
    uint16_t bpm;
    uint8_t mode[6][3], div[6][3], bars[6], take_bars[6][10];
} clock_config;
static clock_config clk = {120};
static volatile uint32_t clock_tick;
static uint32_t clock_rem;
static volatile uint32_t clock_cancel[6][3];
static const uint16_t clock_grid[7]={960,480,320,240,160,120,0};
static const char *const clock_label[7]={"1/4","1/8","1/8T","1/16","1/16T","1/32","OFF"};
static uint32_t clock_div(uint32_t l,uint32_t target) {
    return clock_grid[clk.div[l][target]<7?clk.div[l][target]:3];
}
static uint32_t clock_next(uint32_t grid) {
    uint32_t t=clock_tick; return grid?t+(grid-t%grid):t;
}
static uint32_t clock_ms(uint32_t ticks) { return ticks*625u/((uint32_t)clk.bpm*10u); }
static uint32_t clock_from_ms(uint32_t ms) { return ms*(uint32_t)clk.bpm*16u/1000u; }
static void clock_advance(void) {
    clock_rem+=32u*(uint32_t)clk.bpm*960u;
    clock_tick+=clock_rem/2646000u;
    clock_rem%=2646000u;
}
static void clock_defaults(void) {
    memset(&clk,0,sizeof clk); clk.bpm=120;
    for(uint32_t l=0;l<6;l++) { clk.bars[l]=1; for(uint32_t k=0;k<3;k++) clk.div[l][k]=3; }
    clock_tick=clock_rem=0;
}
/* Main-loop producer, audio consumer. Each request owns its eventual release;
 * a full queue rejects a complete note, never just its note-off. */
typedef struct { uint32_t due,len,epoch; uint8_t l,target,note,vel,take; } clock_req;
#define CLOCK_Q 96u
static clock_req clock_in[CLOCK_Q] POOL;
static volatile uint32_t clock_w,clock_r,clock_dropped;
static struct { clock_req e; uint8_t state; } clock_q[CLOCK_Q] POOL;
static void clock_post(uint32_t l,uint32_t target,uint32_t note,uint32_t vel,uint32_t due,uint32_t len) {
    if(clock_w-clock_r>=CLOCK_Q) { clock_dropped++; return; }
    clock_req e={due,len?len:1,clock_cancel[l][target],(uint8_t)l,(uint8_t)target,(uint8_t)note,(uint8_t)vel};
    clock_in[clock_w%CLOCK_Q]=e; RING_PUBLISH(); clock_w++;
}

static void clock_sanitize(clock_config *c) {
    c->bpm=(uint16_t)clampi(c->bpm,30,300);
    for(uint32_t l=0;l<6;l++) {for(uint32_t k=0;k<10;k++)c->take_bars[l][k]=(uint8_t)clampi(c->take_bars[l][k],0,64);c->bars[l]=(uint8_t)clampi(c->bars[l],1,64);
        for(uint32_t t=0;t<3;t++){c->mode[l][t]=!!c->mode[l][t];c->div[l][t]=(uint8_t)clampi(c->div[l][t],0,t?5:6);}}
}
