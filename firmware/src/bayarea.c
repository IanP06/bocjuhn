/* SPDX-License-Identifier: GPL-3.0-only
 * RYUJI replaces BAYAREA; original design record layout retained internally. */
#define BA_HZ8(x) ((int32_t)((x) * 256))

enum { BK_BONGO, BK_PLUCK, BK_CHIME, BK_FOLDED, BK_SWELL, BK_DRONE, BK_N };
static const char *const BK_NAME[BK_N] = {"DRY", "METAL", "DUST", "RUBBER", "LOGIC", "VOID"};
static const uint8_t BK_W[PATH_N][BK_N] = {
    {1, 1, 1, 1, 1, 1},
    {3, 1, 3, 0, 3, 4},
    {1, 3, 1, 4, 1, 1},
};
/* each kind's ranges, slider positions x 100: shape, folds, FM low, FM high, peak, decay, drive, spring */
static const uint8_t BK_R[BK_N][8][2] = {
    {{0, 30}, {0, 15}, {0, 30}, {0, 20}, {20, 50}, {10, 35}, {10, 30}, {0, 30}},    /* BONGO */
    {{30, 80}, {0, 40}, {0, 40}, {0, 30}, {50, 90}, {15, 45}, {10, 40}, {0, 40}},   /* PLUCK */
    {{0, 40}, {0, 20}, {0, 30}, {30, 80}, {60, 100}, {40, 70}, {5, 25}, {20, 60}},  /* CHIME */
    {{20, 70}, {50, 100}, {0, 50}, {0, 50}, {40, 90}, {30, 60}, {15, 50}, {0, 40}}, /* FOLDED */
    {{0, 70}, {0, 50}, {0, 40}, {0, 40}, {40, 80}, {50, 80}, {5, 25}, {10, 50}},    /* SWELL */
    {{0, 80}, {0, 60}, {20, 60}, {0, 50}, {20, 70}, {55, 85}, {5, 30}, {10, 60}},   /* DRONE */
};

typedef struct {
    uint32_t seed;               /* what made it: ba_design(kind, seed, path, gen) + ba_measure give it again */
    uint8_t kind, path, gen, asr;   /* asr: the sustaining envelope (else the low-pass gate) */
    uint8_t lfo_shape, rsv[3];
    int32_t shape, folds, fm1, fm2;    /* Q15 sliders (folds 0 .. 3 as Q15 x 3) */
    int32_t fm1r, fm2r;          /* the modulators' ratios, Q16 */
    int32_t attack, decay;       /* ms */
    int32_t peak;                /* Hz */
    int32_t drive;               /* x 0 .. 11, Q12 */
    int32_t spring;              /* Q15 */
    int32_t lfo_hz;              /* Q16 */
    int32_t to[8];               /* the LFO's amounts on shape, folds, FM low, FM high, attack, peak, decay, spring: Q15 */
    int32_t out;                 /* Q12: the measured level brought to the target */
} ba_design_t;


#include "pulse_body.h"
