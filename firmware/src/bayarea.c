/* SPDX-License-Identifier: GPL-3.0-only
 * RYUJI replaces BAYAREA; original design record layout retained internally. */
#define BA_HZ8(x) ((int32_t)((x) * 256))

enum { BK_BONGO, BK_PLUCK, BK_CHIME, BK_FOLDED, BK_SWELL, BK_DRONE, BK_N=13 };
static const char *const BK_NAME[BK_N] = {"DRY","METAL","DUST","RUBBER","LOGIC","VOID","808 MIX","909 MIX","NIGHT","WAREHOUSE","LM ACCENT","LATIN GRID","HYBRID"};
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
