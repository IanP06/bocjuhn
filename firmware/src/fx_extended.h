/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Appended session extension: never change the legacy on-disk structures. */
typedef struct {
    int8_t rv[4]; /* type, predelay, diffusion, width */
    int8_t dl[4]; /* type, division, rhythm, width */
    int8_t grain[6][4]; /* pitch (-24..24), reverse %, freeze, PRISM mix */
} fx_config;
static fx_config fx = {{0,100,100,100},{0,0,0,100},{{0}}};
static int32_t fx_rctl[4], fx_d[4], fx_g[6][8];
static uint32_t fx_generation;
static int8_t rv_base[4], dl_base[4];
static const char *const FX_RTYPE[]={"PLATE","DIGITAL","SHIMMER","GRAIN CLOUD","REVERSE","FROZEN HALL"};
static const char *const FX_DTYPE[]={"TAPE","DIGITAL","PING PONG","REVERSE","MULTITAP"};
static const char *const FX_DIV[]={"FREE","1/2","1/4","1/8","1/16","1/32"};
static const char *const FX_RHYTHM[]={"STRAIGHT","DOTTED","TRIPLET","HYBRID"};
static void fx_defaults(void) {
    fx_generation++;
    memset(&fx,0,sizeof fx);
    fx.rv[1]=fx.rv[2]=fx.rv[3]=fx.dl[3]=100;
}
static void fx_sanitize(fx_config *f) {
    for(unsigned k=0;k<4;k++) {
        f->rv[k]=(int8_t)clampi(f->rv[k],0,k?100:5);
        f->dl[k]=(int8_t)clampi(f->dl[k],0,k==0?4:k==1?5:k==2?3:100);
        for(unsigned l=0;l<6;l++) f->grain[l][k]=(int8_t)clampi(f->grain[l][k],k?0:-24,k==0?24:k==2?1:100);
    }
}
