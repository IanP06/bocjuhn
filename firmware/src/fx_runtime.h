/* SPDX-License-Identifier: GPL-3.0-only */
/* Global returns sum all six tracks' routes. Saved knob values stay untouched. */
static int32_t fx_global(uint32_t dst,int32_t base) {
    int32_t v=base;
    for(uint32_t l=0;l<NLAYER;l++)v+=mst[l].off[dst];
    return clampi(v,0,100);
}
static void fx_update(void) {
    int32_t r[4],d[4];
    for(unsigned k=0;k<4;k++) {
        r[k]=fx_global(MD_RSIZE+k,rv_base[k]);
        d[k]=fx_global(MD_DTIME+k,dl_base[k]);
        fx_rctl[k]=fx.rv[k];fx_d[k]=fx.dl[k];
    }
    fx_rctl[1]=fx_global(MD_RPRE,fx.rv[1]);
    fx_rctl[2]=fx_global(MD_RDIFF,fx.rv[2]);
    fx_rctl[3]=fx_global(MD_RWIDTH,fx.rv[3]);
    fx_d[3]=fx_global(MD_DWIDTH,fx.dl[3]);
    /* Cache expensive RT60 exponentials when the effective controls are steady. */
    static int32_t last[8]={-1,-1,-1,-1,-1,-1,-1,-1};
    static int32_t free_time;
    static uint32_t generation;
    if(generation!=fx_generation){last[0]=last[4]=-1;generation=fx_generation;}
    if(memcmp(last,r,sizeof r)) {rv_apply(r[0],r[1],r[2],r[3]);memcpy(last,r,sizeof r);}
    if(memcmp(last+4,d,sizeof d)) {dl_apply(d[0],d[1],d[2],d[3]);free_time=dl_t_want;memcpy(last+4,d,sizeof d);}
    if(fx_d[1]) {
        uint32_t ticks=1920u>>(fx_d[1]-1);
        if(fx_d[2]==1 || fx_d[2]==3)ticks=ticks*3/2;
        else if(fx_d[2]==2)ticks=ticks*2/3;
        dl_t_want=clampi((int32_t)(ticks*22050u/((uint32_t)clk.bpm*16u))*256,512,(DL_N-4)*256);
    } else {
        /* Restore free time when switching out of sync without turning TIME. */
        dl_t_want=free_time;
    }
    for(unsigned l=0;l<NLAYER;l++) {
        for(unsigned k=0;k<4;k++)fx_g[l][k]=mod_val(l,MD_GSIZE+k,gr_edit[l][k],-100,100);
        fx_g[l][4]=mod_val(l,MD_GPITCH,fx.grain[l][0],-24,24);
        fx_g[l][5]=mod_val(l,MD_GREV,fx.grain[l][1],0,100);
        fx_g[l][6]=fx.grain[l][2];
        fx_g[l][7]=mod_val(l,MD_GMIX,fx.grain[l][3],0,100);
    }
}
