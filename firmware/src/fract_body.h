/* FRACT sources: musical digital fragments, with a two-head microgranulator.
 * BLIPS uses the original algorithm, design RNG, envelope and level measurement.
 * The existing 4096-byte buffer is reused; no audio memory is allocated at runtime.
 */
static int32_t fract_source(rus_t *c,const rus_design_t *d,int32_t color,int32_t rate,int32_t chaos)
{
    uint32_t inc=c->inc;
    c->ph[0]+=inc;
    c->ph[1]+=(uint32_t)(((uint64_t)inc*(92000u+(uint32_t)color*3u+(uint32_t)rate*2u))>>16);
    c->ph[2]+=(uint32_t)(((uint64_t)inc*177800u)>>16);
    c->clk[1]+=rus_hz(rus_lerp(128,80*256,rate));
    int32_t a=sin_ph(c->ph[0]),b=sin_ph(c->ph[1]);
    switch(d->kind) {
    case 1: /* PACKET: short phase-modulated carriers in clocked packets */
        if(c->trig || c->clk[1]<rus_hz(rus_lerp(128,80*256,rate))) {c->trig=0;c->a=32767;}
        c->a-=c->a/190+1;if(c->a<0)c->a=0;
        return mulq15(sin_ph(c->ph[0]+(uint32_t)((int64_t)b*color)),c->a)>>6;
    case 2: { /* LOGIC: periodic, seed-stable byte wave; COLOR bends its bit structure */
        uint32_t p=c->ph[0]>>24,q=c->ph[1]>>24;
        int32_t x=(int32_t)((p^(q>>(1+(color>>13))))&255u)-128;
        return x*3;
    }
    case 3: { /* BUFFER: one cycle drawn analytically, continuous pitch with discrete internal steps */
        uint32_t mask=0xffffffffu<<(8+(color>>12));
        int32_t x=sin_ph((c->ph[0]&mask)+(uint32_t)((int64_t)b*(1000+color)));
        return (x+mulq15(a,16384))/96;
    }
    case 4: /* GLASS: three inharmonic partials and a slow digital tremor */
        return (a+mulq15(b,color)+mulq15(sin_ph(c->ph[2]),32767-color))/128;
    default: { /* STATIC: pitched impulses through a decaying high-frequency carrier */
        if(c->trig || rus_chance(c,rus_lerp(2*256,150*256,rate),chaos)) {c->trig=0;c->a=32767;}
        c->a-=c->a/(40+(color>>8))+1;if(c->a<0)c->a=0;
        return mulq15((a+b)/2,c->a)>>6;
    }
    }
}
static void rus_design(rus_design_t *d,int32_t kind,uint32_t seed,uint32_t path,uint32_t gen)
{
    uint32_t family=kind<0||kind>=FRACT_N?(seed^(seed>>11))%FRACT_N:(uint32_t)kind;
    blips_design(d,family? (int32_t)family:RK_BLIPS,seed,path,gen);
    d->kind=(uint8_t)family;
    if(family) {d->bits=10;d->hold=1;d->att=path==PATH_AMBIENT?150:3;d->rel=path==PATH_AMBIENT?900:180;d->chaos=LV(.35);}
}
static const char *rus_mac_name(const rus_design_t *d,uint32_t j)
{(void)d;static const char *const names[4]={"COLOR","GRAINS","RATE","SCATTER"};return names[j&3];}
static void rus_measure(rus_design_t *d)
{uint8_t kind=d->kind;if(!kind)d->kind=RK_BLIPS;blips_measure(d);d->kind=kind;}

static int rus_render(rus_t *c,const rus_design_t *d,int32_t *out)
{
    int8_t mac[4];memcpy(mac,c->mac,4);
    rus_design_t dry=*d;if(!dry.kind)dry.kind=RK_BLIPS;
    c->mac[1]=mac[2];c->mac[2]=0;
    int live=blips_render(c,&dry,out);memcpy(c->mac,mac,4);
    if(!live)return 0;
    int32_t amount=absi(mac[1])*512;
    uint32_t length=(uint32_t)clampi(256-mac[2]*3,64,512);
    for(uint32_t i=0;i<CTL;i++) {
        if(++c->gr_sub==RUS_SUB) {
            c->gr_sub=0;c->buf[c->gr_write&(RUS_BUF-1)]=(int8_t)clampi(out[i]/256,-128,127);
            c->gr_write++; /* unsigned wrap is harmless for the power-of-two ring */
            if(c->gr_valid<RUS_BUF)c->gr_valid++;
            int32_t sum=0,weights=0;
            if(c->gr_valid>length*2) for(uint32_t h=0;h<2;h++) {
                if(!c->gr_len[h] || c->gr_age[h]>=c->gr_len[h]) {
                    c->gr_seed=c->gr_seed*1664525u+1013904223u;
                    uint32_t spread=(uint32_t)(mac[3]+64)*8u;
                    uint32_t back=length+1+(spread?c->gr_seed%(spread+1):0);
                    if(back>c->gr_valid-length)back=c->gr_valid-length;
                    c->gr_phase[h]=(c->gr_write-back)&(RUS_BUF-1);
                    c->gr_len[h]=(uint16_t)length;
                    c->gr_age[h]=(uint16_t)(!c->gr_age[h]&&h?length/2:0);
                }
                uint32_t age=c->gr_age[h]++,ln=c->gr_len[h];
                int32_t w=(int32_t)(age<ln/2?age:ln-age)+1;
                sum+=(int32_t)c->buf[c->gr_phase[h]&(RUS_BUF-1)]*256*w;weights+=w;
                c->gr_phase[h]=(c->gr_phase[h]+(mac[1]<0?RUS_BUF-1:1))&(RUS_BUF-1);
            }
            c->gr_last=weights?sum/weights:out[i];
        }
        c->gr_mix+=(amount-c->gr_mix)/16;
        /* Let the final envelope close the grains too: no frozen tail after note-off. */
        int32_t wet=mulq15(c->gr_last,c->nheld?Q15:c->env>>8);
        out[i]+=mulq15(wet-out[i],c->gr_mix);
    }
    return 1;
}
