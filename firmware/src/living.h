/* SPDX-License-Identifier: GPL-3.0-only
 * bocjuhn RC6: bounded, display-only organisms. No audio state or RNG writes.
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
            uint32_t p=i*1640531527u, h=i*2654435761u+phase;
            int32_t r=12+(int32_t)(i*37u%34u), j=a/3;
            x=64+living_sin(p,r)+living_sin(h*29u,j);
            y=56+living_sin(p+0x40000000u,r*3/4)+living_sin(h*37u,j);
            cv_rect(x,y,i%6?2:3,i%6?2:3,col);
        }
    }else if(eng==ENG_ANIMA){ /* ACID: three elastic membranes. */
        for(uint32_t ring=0;ring<3;ring++)for(i=0;i<=32;i++){
            uint32_t p=i*0x08000000u;
            int32_t r=25+(int32_t)ring*8+living_sin(p*3u+phase,4)+living_sin(p*5u-phase,a/2);
            x=64+living_sin(p,r);y=56+living_sin(p+0x40000000u,r*2/3);
            if(i)living_line(px,py,x,y,col);px=x;py=y;
        }
        cv_rect(62,54,5,5,C_WHITE);
    }else if(eng==ENG_RUS){ /* FRACT: broken, displaced blocks. */
        for(i=0;i<12;i++){
            uint32_t p=i*357913941u;int32_t r=31+living_sin(p*3u+phase,a);
            x=64+living_sin(p,r);y=56+living_sin(p+0x40000000u,r);
            cv_rect(x,y,5+(int32_t)(i%3u)*2,3+(int32_t)(i%2u)*3,col);
        }
    }else if(eng==ENG_FM){ /* Six nodes: an abstract body, not a routing diagram. */
        for(i=0;i<6;i++){
            uint32_t p=i*715827882u;
            x=64+living_sin(p,31+a/2);y=56+living_sin(p+0x40000000u,31+a/2);
            living_line(x,y,64+living_sin(p+1431655764u,31+a/2),56+living_sin(p+0x40000000u+1431655764u,31+a/2),col);
            cv_frame(x-4,y-4,9,9,1,col);cv_rect(x-1,y-1,3,3,C_WHITE);
        }
    }else if(eng==ENG_BAY){ /* RYUJI: damped percussive plates. */
        for(i=0;i<8;i++){
            x=23+(int32_t)i*11;y=40+living_sin(phase*7u+i*600000001u,a);
            cv_rect(x,y,5,24+(int32_t)(i%3u)*4,col);
        }
    }else if(eng==ENG_DRUID){ /* Branching, bowed stems. */
        for(i=0;i<7;i++){
            x=28+(int32_t)i*12;y=30+living_sin(phase+i*700000001u,a);
            living_line(64,88,x,y,col);living_line(x,y,x-6,y+9,col);living_line(x,y,x+6,y+9,col);
        }
    }else if(eng==ENG_HIVE){ /* A bounded lattice of cells. */
        for(i=0;i<9;i++){
            x=34+(int32_t)(i%3u)*27;y=28+(int32_t)(i/3u)*27;
            int32_t r=7+living_sin(phase*2u+i*900000001u,a/4);
            cv_frame(x-r,y-r,2*r+1,2*r+1,1,col);
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
    int32_t target=clampi(lay[ui.sel].meter/100,0,255);
    if(living_last_engine!=eng || living_last_track!=ui.sel){force=1;living_energy=0;living_phase=0;}
    if(!force && (uint32_t)(fm1_ms-living_at)<83u)return; /* <=12 fps */
    if(!force && !target && !living_energy)return;
    living_at=fm1_ms;
    living_energy=target>living_energy?target:living_energy*3/4;
    if(living_energy)living_phase+=0x01000000u+(uint32_t)living_energy*180001u;
    CV_AREA(56,28,128,112,C_BLACK){living_shape(eng,living_phase,living_energy,ui_color(ui.sel));}
    living_last_engine=(uint8_t)eng;living_last_track=ui.sel;
}
