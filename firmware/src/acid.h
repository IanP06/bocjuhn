/* SPDX-License-Identifier: GPL-3.0-only
 * bocjuhn ACID experimental core. Fixed 44100 Hz; not a circuit emulation.
 * Independent of the current engine slots until integration is validated.
 */
#ifndef BOCJUHN_ACID_H
#define BOCJUHN_ACID_H
#include <stdint.h>
#include <string.h>
typedef struct {
    int32_t cutoff, resonance, env_mod, decay_ms;
    int32_t accent, slide_ms, drive; /* 0..32767, milliseconds, 0..32767 */
    uint8_t square;
} acid_params;
typedef struct {
    uint32_t phase, inc, target;
    int32_t pole[4], env, amp, accented;
    uint8_t gate;
} acid_voice;
static int32_t acid_clip(int64_t x, int32_t limit) {
    return x > limit ? limit : x < -limit ? -limit : (int32_t)x;
}
static int32_t acid_mul(int32_t a, int32_t b) {
    return (int32_t)((int64_t)a * b / 32768);
}
static int32_t acid_sat(int32_t x) {
    int64_t mag = x < 0 ? -(int64_t)x : x;
    return (int32_t)((int64_t)x * 32768 / (32768 + mag));
}
static void acid_init(acid_voice *v) { memset(v, 0, sizeof(*v)); }
/* Increment is frequency * 2^32 / 44100. Callers own held-note priority.
 * Slide keeps both envelopes and phase; a new articulated note retriggers.
 */
static void acid_on(acid_voice *v, uint32_t increment, int accent, int slide) {
    if (increment > 0x70000000u) increment = 0x70000000u;
    v->target = increment;
    if (!slide || !v->gate) {
        v->inc = increment;
        v->env = 32767;
    }
    v->accented = accent != 0;
    v->gate = 1;
}
static void acid_off(acid_voice *v) { v->gate = 0; }
/* Q15 polyBLEP around a phase wrap. */
static int32_t acid_blep(uint32_t phase, uint32_t inc) {
    int32_t t;
    if (!inc) return 0;
    if (phase < inc) {
        t = (int32_t)((uint64_t)phase * 32768 / inc);
        return 2*t - acid_mul(t,t) - 32768;
    }
    if (phase > UINT32_MAX - inc) {
        t = -(int32_t)(((uint64_t)UINT32_MAX - phase + 1) * 32768 / inc);
        return acid_mul(t,t) + 2*t + 32768;
    }
    return 0;
}
static int32_t acid_sample(acid_voice *v, const acid_params *p) {
    int32_t slide = acid_clip(p->slide_ms, 1000);
    int32_t decay = acid_clip(p->decay_ms, 4000);
    int32_t cut = acid_clip(p->cutoff, 32767);
    int32_t res = acid_clip(p->resonance, 32767);
    int32_t mod = acid_clip(p->env_mod, 32767);
    int32_t acc = acid_clip(p->accent, 32767);
    int32_t drive = acid_clip(p->drive, 32767);
    int32_t x, g, hz, amp_target, d;
    int i;
    if (slide < 1) slide = 1;
    if (decay < 5) decay = 5;
    if (cut < 0) cut = 0;
    if (res < 0) res = 0;
    if (mod < 0) mod = 0;
    if (acc < 0) acc = 0;
    if (drive < 0) drive = 0;
    v->inc = (uint32_t)((int64_t)v->inc + ((int64_t)v->target-v->inc)/(slide*44/4+1));
    if (v->env > 0) { d = v->env/(decay*44/5+1); v->env -= d ? d : 1; }
    amp_target = v->gate ? 22000 + (v->accented ? acid_mul(acc,10767) : 0) : 0;
    d = (amp_target-v->amp)/(v->gate ? 40 : 220);
    if (!d && amp_target != v->amp) d = amp_target > v->amp ? 1 : -1;
    v->amp += d;
    x = (int32_t)(v->phase >> 16)-32768-acid_blep(v->phase,v->inc);
    if (p->square) x = (v->phase < 0x80000000u ? 24576 : -24576)
        + acid_mul(24576,acid_blep(v->phase,v->inc)-acid_blep(v->phase+0x80000000u,v->inc));
    v->phase += v->inc;
    cut = acid_clip((int64_t)cut + acid_mul(v->env,mod)
        + (v->accented ? acid_mul(acid_mul(v->env,acc),8192) : 0),32767);
    hz = 60 + acid_mul(acid_mul(cut,cut),10000);
    /* Bounded one-pole coefficient and nonlinear feedback, two substeps.
     * This is a sonic prototype, not a calibrated TB-303 ladder model. */
    g = (int32_t)((int64_t)hz*32768/(14037+hz));
    for (i=0; i<2; ++i) {
        int j;
        int32_t y = acid_sat(acid_clip((int64_t)x*(32768+drive)/32768
            - (int64_t)v->pole[3]*res*4/32768,262144));
        for (j=0;j<4;++j) {
            v->pole[j] += acid_mul(g,y-v->pole[j]);
            y=v->pole[j];
        }
    }
    return acid_clip((int64_t)acid_mul(v->pole[3],v->amp)*2,32767);
}
#endif
