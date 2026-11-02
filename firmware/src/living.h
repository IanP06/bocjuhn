/* SPDX-License-Identifier: GPL-3.0-only
 * bocjuhn RC7: bounded, display-only organisms. No audio state or RNG writes.
 * Coordinates are local to a 128 x 112 area, rendered in existing LCD bands.
 * Integer sine lookup; no allocation, framebuffer, floating point or FFT.
 */
static uint32_t living_at, living_phase;
static int32_t living_energy;
static uint8_t living_last_engine = 255, living_last_track = 255;

static void living_line(int32_t x, int32_t y, int32_t xx, int32_t yy, uint16_t c)
{
    int32_t dx = xx > x ? xx-x : x-xx, sx = x < xx ? 1 : -1;
    int32_t dy = yy > y ? y-yy : yy-y, sy = y < yy ? 1 : -1, err=dx+dy;
    for (;;) {
        cv_rect(x,y,1,1,c);
        if(x==xx && y==yy) break;
        int32_t e=2*err;
        if(e>=dy){err+=dy;x+=sx;}
        if(e<=dx){err+=dx;y+=sy;}
    }
}
static int32_t living_sin(uint32_t p, int32_t r) { return mulq15(r,sin_ph(p)); }
static void living_shape(uint32_t eng, uint32_t phase, int32_t energy, uint16_t col)
{
    int32_t a=energy/16, x,y,px=0,py=0;
    uint32_t i;
    if(eng==ENG_CAL){ /* DROPLETS: independent trembling particles, 2-3 pixels. */
        for(i=0;i<26;i++){
            uint32_t p=i*1640531527u+phase*(1u+i%3u), h=i*2654435761u+phase;
            int32_t r=12+(int32_t)(i*37u%34u), j=2+a/2;
            x=64+living_sin(p,r)+living_sin(h*5u,j);
            y=56+living_sin(p+0x40000000u,r*3/4)+living_sin(h*7u,j);
            cv_rect(x,y,i%6?2:3,i%6?2:3,col);
        }
    }else if(eng==ENG_ANIMA){ /* ACID: three elastic membranes. */
        for(uint32_t ring=0;ring<3;ring++)for(i=0;i<=32;i++){
            uint32_t p=i*0x08000000u;
            int32_t r=25+(int32_t)ring*8+living_sin(p*3u+phase*3u,4+a/3)+living_sin(p*5u-phase*2u,2+a/2);
            x=64+living_sin(p,r);y=56+living_sin(p+0x40000000u,r*2/3);
            if(i)living_line(px,py,x,y,col);px=x;py=y;
        }
        cv_rect(62,54,5,5,C_WHITE);
    }else if(eng==ENG_RUS){ /* FRACT: broken, displaced blocks. */
        for(i=0;i<12;i++){
            uint32_t p=i*357913941u+phase*(1u+i%3u);int32_t r=31+living_sin(p*3u+phase,a);
            x=64+living_sin(p,r);y=56+living_sin(p+0x40000000u,r);
            cv_rect(x,y,5+(int32_t)(i%3u)*2,3+(int32_t)(i%2u)*3,col);
        }
    }else if(eng==ENG_FM){ /* Six independently orbiting operators; open woven chain. */
        int32_t nx[6],ny[6];
        for(i=0;i<6;i++){
            uint32_t p=i*1640531527u+phase*(2u+i%3u);
            nx[i]=64+living_sin(p,22+(int32_t)i*3)+living_sin(phase*3u+i*900000001u,5+a/3);
            ny[i]=56+living_sin(p+0x31000000u,16+(int32_t)(i%4u)*6)+living_sin(phase*2u+i*700000001u,4+a/4);
        }
        for(i=1;i<6;i++){
            int32_t mx=(nx[i-1]+nx[i])/2+living_sin(phase*3u+i*500000001u,9);
            int32_t my=(ny[i-1]+ny[i])/2+living_sin(phase*2u+i*800000001u,9);
            for(uint32_t t=0;t<=8;t++){
                int32_t u=8-(int32_t)t;
                x=(u*u*nx[i-1]+2*u*(int32_t)t*mx+(int32_t)(t*t)*nx[i])/64;
                y=(u*u*ny[i-1]+2*u*(int32_t)t*my+(int32_t)(t*t)*ny[i])/64;
                if(t)living_line(px,py,x,y,col);px=x;py=y;
            }
        }
        for(i=0;i<6;i++){
            cv_frame(nx[i]-3,ny[i]-3,7,7,1,col);
            cv_rect(nx[i]-1,ny[i]-1,3,3,C_WHITE);
        }
    }else if(eng==ENG_BAY){ /* RYUJI: damped percussive plates. */
        for(i=0;i<8;i++){
            x=23+(int32_t)i*11;y=40+living_sin(phase*7u+i*600000001u,a);
            cv_rect(x,y,5,24+(int32_t)(i%3u)*4,col);
        }
    }else if(eng==ENG_DRUID){ /* Five flexible, interlacing resonant ribbons. */
        for(uint32_t ribbon=0;ribbon<5;ribbon++)for(i=0;i<=24;i++){
            uint32_t p=i*0x09000000u+phase*(2u+ribbon%2u)+ribbon*600000001u;
            x=20+(int32_t)i*88/24;
            y=38+(int32_t)ribbon*9+living_sin(p,8+a/2)+living_sin(p*2u+phase,3);
            if(i)living_line(px,py,x,y,col);px=x;py=y;
            if(i==8+ribbon*2)cv_rect(x-1,y-1,3,3,C_WHITE);
        }
    }else if(eng==ENG_HIVE){ /* A drifting swarm, each cell with its own orbit. */
        for(i=0;i<13;i++){
            uint32_t p=i*1640531527u+phase*(1u+i%3u);
            int32_t r=12+(int32_t)(i*17u%31u);
            x=64+living_sin(p,r)+living_sin(phase*5u+i*700000001u,2+a/3);
            y=56+living_sin(p+0x40000000u,r*3/4)+living_sin(phase*3u+i*900000001u,2+a/3);
            int32_t w=3+(int32_t)(i%3u);
            living_line(x-w,y,x,y-w,col);living_line(x,y-w,x+w,y,col);
            living_line(x+w,y,x,y+w,col);living_line(x,y+w,x-w,y,col);
            if(i%4u==0)cv_rect(x,y,2,2,C_WHITE);
        }
    }else{ /* OBRADOR: interleaved oscillating slats. */
        for(i=0;i<9;i++){
            y=24+(int32_t)i*8;x=living_sin(phase*3u+i*400000001u,a);
            living_line(27+x,y,100-x,y,col);cv_rect(62+x,y-1,3,3,C_WHITE);
        }
    }
}
static void draw_living(int force)
{
    uint32_t eng=ui.engine[ui.sel];
    int32_t peak=lay[ui.sel].meter;
    int32_t target=peak>32 ? clampi((log2_q16(peak)-(5<<16))/2048,0,255) : 0;
    if(living_last_engine!=eng || living_last_track!=ui.sel){force=1;living_energy=0;living_phase=0;}
    if(!force && (uint32_t)(fm1_ms-living_at)<83u)return; /* <=12 fps */
    if(!force && !target && !living_energy)return;
    living_at=fm1_ms;
    living_energy=target>living_energy?target:living_energy*3/4;
    if(living_energy)living_phase+=0x01000000u+(uint32_t)living_energy*180001u;
    CV_AREA(56,28,128,112,C_BLACK){living_shape(eng,living_phase,living_energy,ui_color(ui.sel));}
    living_last_engine=(uint8_t)eng;living_last_track=ui.sel;
}
