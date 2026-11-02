/* SPDX-License-Identifier: GPL-3.0-only */
#define CAL_DLN 768u             /* each delay line: samples at FS / 2 (~35 ms), mu-law */

enum { CS_SPORE, CS_THORN, CS_HIVE, CS_CARRION, CS_VEIL, CS_RUST,
       CS_N0,                    /* (generation 0 draws these six only) */
       CS_LITANY = CS_N0, CS_UNDERTOW, CS_WIRE, CS_MOTH, CS_N };
static const char *const CS_NAME[CS_N] = {"CLICK", "DROP", "GLASS", "RAIN", "POOL", "METAL", "CHIME", "DEEP", "SPARK", "MIST"};
enum { CSRC_PARTNER, CSRC_SIDE, CSRC_CROSS, CSRC_TOTAL };
enum { CLNK_A, CLNK_AND, CLNK_OR, CLNK_XOR };

typedef struct {
    uint32_t seed;               /* what made it: cal_design(kind, seed, path, gen) + cal_measure give it again */
    uint8_t kind;
    uint8_t src[4];              /* each pair's FM source */
    uint8_t link, lfo_sq, gate_pairs, del_src;   /* hyper LFO; its shape; the pairs it gates; the delay's mod */
    int16_t tune[8];             /* each oscillator, from the root: 1/16 semitone */
    int32_t att[4], rel[4];      /* each pair's envelope: ms */
    int32_t sharp[4], mod[4];    /* Q15 */
    int32_t lfo_idx[4];          /* how much the hyper LFO moves each pair's FM depth, Q15 */
    int32_t hold[2];             /* each side: the floor its oscillators hold at while a sensor of it is down */
    int32_t lfo_a, lfo_b;        /* Hz Q16 */
    int32_t gate_depth;          /* Q15 */
    int32_t vib;                 /* vibrato, Q15 of a semitone */
    int32_t del_t[2];            /* each line's time: samples at FS / 2 */
    int32_t del_mod, del_fb, del_mix;   /* Q15 */
    int32_t drive;               /* Q12 */
    int32_t total_fb;            /* Q15 */
    int32_t burst;               /* bursts a second while awake, Q8 */
    int32_t surge;               /* surges a minute while awake, Q8 */
    int32_t drift;               /* how far depth and sharpness wander, Q15 */
    int32_t out;                 /* Q12: the measured level brought to the target */
    /* the hand */
    int32_t touch;               /* fingers landing a second, Q8 (on average: the waits are irregular) */
    int32_t hold_lo, hold_hi;    /* how long a finger stays, ms */
    int32_t tap;                 /* the share of touches that are quick taps, Q15 */
    uint8_t fingers;             /* at most this many down at once */
    uint8_t home;                /* the plates it favours, a bit each */
    uint8_t first;               /* plates it lands on at once, 1 .. 3 */
    uint8_t path;                /* PATH_*: what it was drawn on */
    /* (generation 1 on: fields after here, so generation 0's stay where they were) */
    uint8_t gen;                 /* the generators' generation that drew it */
    uint8_t motif[4];            /* the hand's phrase: the plates it comes back to (generation 1) */
    uint8_t mlen;                /* .. how many (0: no phrase, every plate by chance) */
    uint8_t mfollow;             /* .. how often it follows it, Q8 */
    uint8_t rhythm;              /* .. its touches on a pulse (the phrase's own) rather than at random */
    uint8_t rsv1;
} cal_design_t;

typedef struct {
    uint32_t ph[8], inc[8];
    int32_t env[8];              /* Q23 (Q15 and 8 bits more: a swell of seconds moves less than 1 a sample) */
    uint16_t burst[8];           /* blocks of a burst left */
    uint8_t gate[8];             /* the plates the hand holds down */
    uint16_t ftime[8];           /* blocks each finger stays (0xFFFF: until the key is let go) */
    uint8_t ftap[8];             /* that finger is a quick tap (a fast envelope) */
    uint8_t note[8], nsrc[8], nheld;   /* the keys held (the last on top), and who holds them: 1 keys, 2 loop */
    uint32_t touch_wait;         /* blocks to the next finger */
    int32_t osc[8];              /* each oscillator's last output (Q15) */
    int32_t pair[4];             /* each pair's last output */
    int32_t fb;                  /* the total feedback: the output, saturated, low-passed */
    uint32_t la, lb, vph;        /* the hyper LFO's two, the vibrato */
    int32_t gate_s;              /* the gate, slewed */
    int32_t wd[4], ws[4];        /* the wanderings of depth and sharpness, Q15 */
    int32_t wd_t[4], ws_t[4];
    uint32_t awake;              /* blocks it stays awake */
    uint32_t surge_left;
    int32_t base, base_t;        /* the note it sounds (MIDI note x 16), and where it is going (a quick snap) */
    uint32_t dw;
    int32_t dhalf[2], dlast[2];
    uint8_t odd, live;
    uint8_t hush;                /* overload (layers.c): its tail, no key on it, let go in ~30 ms */
    uint8_t mstep;               /* where the hand is in its phrase */
    int8_t mac[NMAC];            /* EDIT's macros (layers.c sets them each block; 0: the design as drawn) */
    rnd_t r;
} cal_t;

static FAST int32_t cal_ex(int32_t ms)            /* a per-block factor: e^(-4.6 x 0.7256 / ms), Q16 */
{
    return an_expneg((int32_t)(218760u / (uint32_t)clampi(ms, 1, 600000)));
}
static __typeof__(&cal_ex) volatile cal_ex_fp = cal_ex;   /* (the flash's way into the RAM code) */


/* DROPLETS: original fixed-point impulse/resonator engine, inspired by the
 * musical behaviour of Particles. No Plaits code, floating point or diffuser.
 * Eight bounded resonating impulses per track; held chords share these slots.
 * cal_* names and the design record remain the stable session slot. */
static void cal_init(cal_t *c, uint32_t seed) { memset(c, 0, sizeof *c); rnd_seed(&c->r, seed); }
AINL int cal_fingers_down(const cal_t *c) { return c->nheld; }
static const char *cal_mac_name(const cal_design_t *d, uint32_t j)
{
    static const char *const name[4] = {"DENSITY", "COLOR", "RESONANCE", "SPREAD"};
    (void)d; return name[j & 3u];
}
static void cal_land(cal_t *c, const cal_design_t *d, uint32_t k, int for_good)
{
    int32_t spread = clampi(d->drift + c->mac[3] * 3, 0, 384);
    int32_t note = c->base_t;
    if (!for_good && c->nheld) note = (int32_t)c->note[rnd_below(&c->r, c->nheld)] * 16;
    note += spread ? rnd_in(&c->r, -spread, spread) : 0;
    c->inc[k] = pitch_inc(clampi(note, 12*16, 115*16));
    c->ph[k] = 0;
    c->env[k] = 1 << 23;
    c->burst[k] = 16;
    c->gate[k] = 1;
}
static void cal_note_on(cal_t *c, const cal_design_t *d, uint32_t note, uint32_t who)
{
    uint32_t k;
    for (k = 0; k < c->nheld; k++)                        /* (already held: to the top) */
        if (c->note[k] == note && c->nsrc[k] == who) {
            for (; k + 1u < c->nheld; k++) {
                c->note[k] = c->note[k + 1u];
                c->nsrc[k] = c->nsrc[k + 1u];
            }
            c->nheld--;
            break;
        }
    if (c->nheld == 8u) {
        for (k = 0; k + 1u < 8u; k++) {
            c->note[k] = c->note[k + 1u];
            c->nsrc[k] = c->nsrc[k + 1u];
        }
        c->nheld--;
    }
    c->note[c->nheld] = (uint8_t)note;
    c->nsrc[c->nheld++] = (uint8_t)who;
    c->base_t = (int32_t)note * 16;
    c->live = 1;
    c->awake = 1;
    c->hush = 0;
    c->base = c->base_t;
    cal_land(c, d, c->dw++ & 7u, 1);
    c->touch_wait = 1;

}
/* everything the hand holds let go: the organism rings out */
static void cal_lift(cal_t *c)
{
    memset(c->gate, 0, sizeof c->gate);
    memset(c->burst, 0, sizeof c->burst);
    c->awake = 0;
    c->surge_left = 0;
}
static void cal_note_off(cal_t *c, uint32_t note, uint32_t who)
{
    uint32_t k;
    for (k = 0; k < c->nheld; k++)
        if (c->note[k] == note && c->nsrc[k] == who) {
            for (; k + 1u < c->nheld; k++) {
                c->note[k] = c->note[k + 1u];
                c->nsrc[k] = c->nsrc[k + 1u];
            }
            c->nheld--;
            if (c->nheld)
                c->base_t = (int32_t)c->note[c->nheld - 1u] * 16;   /* back to the key still held */
            else
                cal_lift(c);
            return;
        }
}
/* every note one holder holds (the loop, stopped), let go */
static void cal_release(cal_t *c, uint32_t who)
{
    uint32_t k = 0;
    while (k < c->nheld) {
        if (c->nsrc[k] == who)
            cal_note_off(c, c->note[k], who);
        else
            k++;
    }
}


static FAST int cal_render(cal_t *c, const cal_design_t *d, int32_t *out)
{
    uint32_t i, k;
    int32_t ms = clampi(mulq16(d->rel[0], (int32_t)exp2_q16(c->mac[2] * (3*Q16/MAC_MAX))), 3, 6000);
    int32_t color = clampi(d->sharp[0] + c->mac[1]*240, 0, Q15);
    int32_t decay = cal_ex(c->hush ? 20 : ms);
    uint32_t active = 0;
    memset(out, 0, CTL * sizeof *out);
    if (!c->live) return 0;
    if (c->nheld && (!c->touch_wait || !--c->touch_wait)) {
        int32_t rate = clampi(mulq16(d->touch, (int32_t)exp2_q16(c->mac[0]*(4*Q16/MAC_MAX))), 16, 256*120);
        if (c->mac[0] > -MAC_MAX) cal_land(c, d, c->dw++ & 7u, 0);
        c->touch_wait = (uint32_t)rnd_in(&c->r, 1, clampi((1378*512)/rate, 2, 44096));
    }
    for (k = 0; k < 8u; k++) if (c->env[k] > 128) {
        int32_t env = c->env[k], end = (int32_t)(((int64_t)env * decay) >> 16);
        int32_t step = (end-env) / CTL;
        /* Gentle downward bend of the resonant attack gives the water-drop
         * articulation; COLOR adds a second harmonic and a short noisy click. */
        uint32_t inc = c->inc[k] + (uint32_t)(((uint64_t)c->inc[k] * (uint32_t)(env >> 12) * (uint32_t)d->mod[0]) >> 27);
        for (i = 0; i < CTL; i++) {
            int32_t sample = sin_ph(c->ph[k]);
            sample += mulq15(sin_ph(c->ph[k]*2u), color)/2;
            if (c->burst[k]) {
                sample += mulq15(rnd_bi(&c->r), color) * c->burst[k] / 16;
                c->burst[k]--;
            }
            out[i] += (int32_t)(((int64_t)sample * env) >> 25);
            c->ph[k] += inc;
            env += step;
        }
        c->env[k] = end;
        active++;
    } else { c->env[k] = 0; c->gate[k] = 0; }
    c->base = c->base_t;
    c->live = (uint8_t)(active || c->nheld);
    return c->live;
}
static __typeof__(&cal_render) volatile cal_render_fp = cal_render;
static void cal_design(cal_design_t *d, int32_t kind, uint32_t seed, uint32_t path, uint32_t gen)
{
    static const uint16_t decay[CS_N] = {8, 180, 650, 95, 1100, 420, 1800, 900, 30, 280};
    rnd_t r; rnd_seed(&r, seed); memset(d, 0, sizeof *d);
    /* Consume the family draw even when recalling an explicit family, so a
     * preset regenerated from its seed is byte-identical to its first draw. */
    uint32_t drawn = (uint32_t)rnd_below(&r, CS_N);
    d->seed = seed; d->kind = (uint8_t)(kind < 0 ? drawn : (uint32_t)kind % CS_N);
    d->path = (uint8_t)path; d->gen = (uint8_t)gen;
    for (uint32_t j=0;j<4;j++) { d->att[j]=1; d->rel[j]=decay[d->kind]; }
    d->rel[0] += rnd_in(&r, 0, decay[d->kind]/3);
    d->sharp[0] = d->kind == CS_SPORE || d->kind == CS_WIRE ? 24000 : rnd_in(&r, 1000, 14000);
    d->mod[0] = d->kind == CS_THORN || d->kind == CS_VEIL || d->kind == CS_UNDERTOW ? rnd_in(&r, 800, 2400) : 0;
    d->touch = rnd_in(&r, 2*256, 12*256);
    d->drift = d->kind == CS_LITANY ? 0 : rnd_in(&r, 0, 64);
    d->out = 4096; d->fingers = 8; d->first = 1;
}
static void cal_measure(cal_design_t *d) { d->out = 4096; }
