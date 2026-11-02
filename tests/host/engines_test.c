/* Production DSP, UI, storage and sequencing integration for the replacement engines. */
#define main original_host_main
#include "host.c"
#undef main
#include <assert.h>
static uint64_t hash_sample(uint64_t h,int32_t x){return (h^(uint32_t)x)*1099511628211ull;}
static uint64_t pulse_check(unsigned family,unsigned pad,int macro,int value,int release,double *flux)
{
    ba_design_t d;ba_voice_t v;int32_t out[CTL];
    ba_design(&d,(int)family,77491,PATH_OPEN,GEN_NOW);ba_init(&v,7);
    if(macro>=0)v.mac[macro]=value;
    ba_note_on(&v,&d,48+pad,127,1);
    uint64_t hash=1469598103934665603ull;double power=0,delta=0;int32_t last=0,peak=0;
    for(unsigned b=0;b<1600;b++) {
        if(release&&b==1)ba_note_off(&v,&d);
        memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);
        for(unsigned i=0;i<CTL;i++) {int32_t x=out[i];hash=hash_sample(hash,x);power+=(double)x*x;delta+=(double)(x-last)*(x-last);last=x;if(abs(x)>peak)peak=abs(x);}
    }
    assert(!v.live&&peak>100&&peak<30000);
    if(flux)*flux=delta/power;
    return hash;
}
static uint64_t fract_check(unsigned family,int macro,int value,int grains)
{
    rus_design_t d;static rus_t c;int32_t out[CTL];
    rus_design(&d,(int)family,81991,PATH_OPEN,GEN_NOW);rus_measure(&d);rus_init(&c,17);rus_note_on(&c,&d,48);
    c.mac[1]=grains;if(macro>=0)c.mac[macro]=value;
    uint64_t hash=1469598103934665603ull;int peak=0;
    for(unsigned b=0;b<6000;b++) {
        if(b==2500)rus_note_off(&c,48);
        memset(out,0,sizeof out);rus_render(&c,&d,out);
        for(unsigned i=0;i<CTL;i++) {hash=hash_sample(hash,out[i]);if(abs(out[i])>peak)peak=abs(out[i]);assert(abs(out[i])<32760);}
    }
    for(unsigned tail=0;c.live&&tail<60000;tail++)rus_render(&c,&d,out);
    if(peak<=100||c.live)printf("FRACT failed family=%u macro=%d value=%d grains=%d peak=%d env=%d live=%u\n",family,macro,value,grains,peak,c.env,c.live);
    assert(peak>100&&!c.live);return hash;
}
static int test_load(unsigned l,unsigned e,unsigned slot)
{
    ui.sel=l;ui.pre_eng=e;ui.pre_ix=slot;ui.pre_page=SP_LIST;
    press_btn(B_ARP);run_blocks(24,0);
    assert(ui.engine[l]==e&&!ui.pre_page);return SES_OK;
}
/* Correlation removes gain: a macro must change waveform, not just volume. */
static void ryuji_macros(void)
{
    static int32_t a[8192],b[8192];
    for(unsigned pad=0;pad<16;pad++)for(unsigned m=0;m<4;m++) {
        for(unsigned side=0;side<2;side++) {
            ba_design_t d;ba_voice_t v;ba_design(&d,0,77491,PATH_OPEN,GEN_NOW);ba_init(&v,0);
            ba_note_on(&v,&d,48+pad,127,1);v.hold=1;v.mac[m]=side?64:-64;
            for(unsigned j=0;j<8192;j+=CTL)ba_render(&v,&d,(side?b:a)+j,CTL);
        }
        double aa=0,bb=0,ab=0;
        for(unsigned j=2048;j<8192;j++){aa+=(double)a[j]*a[j];bb+=(double)b[j]*b[j];ab+=(double)a[j]*b[j];}
        double corr=ab/sqrt(aa*bb);
        printf("RYUJI pad %u macro %u: correlation %.4f, RMS %.1f / %.1f\n",pad,m,corr,sqrt(aa/6144),sqrt(bb/6144));
        assert(aa>6144*100.0 && bb>6144*100.0 && corr<0.995);
    }
}
static int ryuji_voice(void)
{
    for(unsigned k=0;k<NVOICE;k++)if(vox[k].sounding&&vox[k].layer==5&&vox[k].eng==ENG_BAY)return (int)k;
    return -1;
}
static void ryuji_start(int a,int d,int s,int r)
{
    for(unsigned l=0;l<NLAYER;l++)ev_post(EV_STOP,l,0,0);run_blocks(40,0);
    ui.sel=5;ui.oct[5]=0;ui.chord_on[5]=0;
    ui.env_on[5][ENG_BAY]=1;
    ui.env[5][ENG_BAY][0]=a;ui.env[5][ENG_BAY][1]=d;
    ui.env[5][ENG_BAY][2]=s;ui.env[5][ENG_BAY][3]=r;mac_apply(5);
    note_on(5,48,SRC_KEY);run_blocks(3,0);assert(ryuji_voice()>=0);
}
static void ryuji_controls(void)
{
    /* Full production ADSR and voice lifetime, not just the standalone source. */
    ryuji_start(75,50,100,100);int k=ryuji_voice();run_blocks(100,0);
    assert(vox[k].est==AE_ATT && vox[k].elv<(1<<23));
    run_blocks(2400,0);assert(vox[k].est==AE_DEC && vox[k].elv>(1<<23));
    int32_t high=vox[k].elv;ui.env[5][ENG_BAY][2]=20;ui.env[5][ENG_BAY][1]=10;mac_apply(5);
    run_blocks(300,0);assert(vox[k].elv<high/3 && vox[k].elv>(1<<24)/10);
    ryuji_start(0,95,0,100);k=ryuji_voice();run_blocks(200,0);int32_t slow=vox[k].elv;
    ryuji_start(0,10,0,100);k=ryuji_voice();run_blocks(200,0);assert(vox[k].elv<slow/10);
    ryuji_start(0,50,100,100);k=ryuji_voice();run_blocks(1600,0);
    double energy=0;for(unsigned j=0;j<CTL;j++)energy+=(double)vr_buf[k][j]*vr_buf[k][j];assert(energy>10000);
    uint32_t base=vmem[k].ba.inc;press_btn(B_OCTUP);run_blocks(4,0);assert(vmem[k].ba.inc==base*2);
    press_btn(B_OCTDN);press_btn(B_OCTDN);run_blocks(4,0);assert(vmem[k].ba.inc==base/2);
    press_btn(B_OCTUP);run_blocks(4,0);assert(vmem[k].ba.inc==base);
    for(int oct=BB_LO;oct<=BB_HI;oct++) {bb.base[5]=oct;unsigned n=bb_note(5,2,0);assert((n&15)==8);assert((int)(n/16)-3==oct+1);}
    bb.base[5]=-1;
    note_off(5,48,SRC_KEY);run_blocks(1600,0);assert(vox[k].sounding&&vox[k].est==AE_REL);
    energy=0;for(unsigned j=0;j<CTL;j++)energy+=(double)vr_buf[k][j]*vr_buf[k][j];assert(energy>10000);
    run_blocks(46000,0);assert(ryuji_voice()<0);
    ryuji_start(0,50,100,0);note_off(5,48,SRC_KEY);run_blocks(200,0);assert(ryuji_voice()<0);
    char name[24];pre_name(ENG_BAY,1,name,sizeof name);assert(!strcmp(name,"KANAGAWA"));
    assert(!strcmp(ENG_NAME[ENG_BAY],"RYUJI"));
    ui.env_on[5][ENG_BAY]=0;mac_apply(5);
    puts("RYUJI: attack, decay, sustain, >1s audible release, short release, held-note OCT +/-, BALLSY register and KANAGAWA OK");
}
#include "bocjuhn_checks.h"
int main(void)
{
    ryuji_macros();
    uint64_t hashes[16];double spectrum[16];
    for(unsigned k=0;k<6;k++)for(unsigned p=0;p<16;p++) {
        uint64_t h=pulse_check(k,p,-1,0,0,k==0?&spectrum[p]:0);
        assert(h==pulse_check(k,p,-1,0,1,0)); /* release cannot chop one-shots */
        if(k==0) {for(unsigned j=0;j<p;j++)assert(h!=hashes[j]);hashes[p]=h;}
    }
    assert(spectrum[0]<spectrum[2]&&spectrum[1]<spectrum[2]&&spectrum[2]<spectrum[4]);
    printf("PULSE spectrum derivative-energy: kick %.5f, sub %.5f, snare %.5f, hat %.5f\n",spectrum[0],spectrum[1],spectrum[2],spectrum[4]);
    for(unsigned m=0;m<4;m++)assert(pulse_check(0,2,-1,0,0,0)!=pulse_check(0,2,m,48,0,0));
    puts("PULSE: 96 kit/pad renders; distinct pads, low/mid/high ordering, one-shots and four macros OK");
    for(unsigned k=0;k<FRACT_N;k++) {
        uint64_t dry=fract_check(k,-1,0,0),wet=fract_check(k,1,48,0),rev=fract_check(k,1,-48,0);
        assert(dry==fract_check(k,-1,0,0));assert(dry!=wet&&dry!=rev&&wet!=rev);
        for(unsigned m=0;m<4;m++)assert(fract_check(k,-1,0,32)!=fract_check(k,m,60,32));
    }
#ifdef FRACT_DATA
    puts("FRACT DATA: six families, deterministic replay, event rate and four macros, bounded peaks and note-off OK");
#else
    puts("FRACT: six families, deterministic replay, forward/reverse grains, four macros, bounded peaks and note-off OK");
#endif
    ses_head_t old;memset(&old,0,sizeof old);old.magic=0x31534341u;old.len=sizeof old;old.head_crc=ses_crc((const uint8_t *)&old,30,0);assert(!ses_head_ok(&old));
    memset(host_flash,255,sizeof host_flash);grain_reset();ses_scan(1);
    assert(ui.engine[3]==ENG_RUS&&ui.engine[5]==ENG_BAY);
    /* Actual front panel and generator note paths: all sixteen pads, every scale/root. */
    ui.sel=5;
    for(unsigned scale=0;scale<NSCALE;scale++)for(unsigned pad=0;pad<16;pad++) {
        ui.scale=scale;ui.root=(scale+7)%12;ui.oct[5]=(int)(scale%7)-3;ui.lx=5;ui.lxt[5]=-3;
        press_key(pad,1);
        int physical=-1;for(unsigned j=0;j<NKEY;j++)if(white_ix(j)==(int)pad){physical=(int)j;break;}
        assert(physical>=0&&ui.key_note[physical]==48+pad);press_key(pad,0);
        bl.w[5][0]=pad;bl.o[5][0]=3;assert(bl_pitch(5,0,0)==48+pad);
        assert(layer_note(5,48+pad,-7,0xab5)==48+pad);
        assert(layer_note(5,80+pad,0,0xab5)==80+pad);
        for(unsigned wall=0;wall<4;wall++)for(unsigned segment=0;segment<SCALE_N[scale];segment++) {
            unsigned note=bb_note(5,wall,segment);assert(note>=48+wall*4&&note<52+wall*4);
        }
    }
    ui.chord_on[5]=1;ui.chord_form[5]=0;chord_press(5,48,1,1);run_blocks(4,0);
    for(unsigned j=0;j<NVOICE;j++)if(vox[j].gate&&vox[j].layer==5)assert(vox[j].note==48);
    chord_press(5,48,1,0);run_blocks(1200,0);
    note_on(5,53,SRC_KEY);run_blocks(4,0);note_on(5,52,SRC_KEY);
    int choked=0;for(unsigned j=0;j<NVOICE;j++)choked+=vmem[j].ba.live&&vox[j].eng==ENG_BAY&&vmem[j].ba.choke==128;
    assert(choked);for(unsigned l=0;l<NLAYER;l++)ev_post(EV_STOP,l,0,0);run_blocks(30,0);
    ryuji_controls();
    puts("Sequencing: keyboard, all scales/roots/octaves, loop transpose, BALLSY, BRICKLAYER, chord suppression and hat choke OK");
    /* Native preset list: 24 defaults each, user overrides survive a rescan, sessions retain edits. */
    for(unsigned e=0;e<NENG;e++)if(e==ENG_BAY||e==ENG_RUS)for(unsigned slot=1;slot<=24;slot++) {
        assert(pre_kind(e,slot)==(int)((slot-1)%6));assert(test_load(0,e,slot)==SES_OK);run_blocks(24,0);
        assert(ui.engine[0]==e);
        press_key(0,1);int peak=0;
        for(unsigned b=0;b<700;b++){host_pump();for(unsigned i=0;i<CTL*2;i++){if(abs(mix_out[i])>peak)peak=abs(mix_out[i]);assert(abs(mix_out[i])<32760);}}
        press_key(0,0);assert(peak>10);
        ev_post(EV_STOP,0,0,0);run_blocks(30,0);
        char name[24];pre_name(e,slot,name,sizeof name);assert(text_w(&FONT_XS,name)<=182);
    }
    assert(pre_kind(ENG_BAY,25)<0&&pre_kind(ENG_FM,1)<0);
    assert(test_load(0,ENG_RUS,8)==SES_OK);run_blocks(24,0);ui.mac[0][ENG_RUS][1]=43;
    assert(pre_save(0,ENG_RUS,8)==SES_OK);memset(ui.mac[0][ENG_RUS],0,4);ses_scan(1);
    assert(test_load(0,ENG_RUS,8)==SES_OK&&ui.mac[0][ENG_RUS][1]==43);
    loop[0].speed_pct=137;loop[0].trk[0].speed_pct=81;ui.env_on[0][ENG_RUS]=1;ui.env[0][ENG_RUS][3]=17;gr_edit[0][2]=31;
    assert(ses_save(1)==SES_OK);loop[0].speed_pct=100;ui.mac[0][ENG_RUS][1]=0;gr_edit[0][2]=0;
    assert(ses_load_slot(1)==SES_OK&&ui.engine[0]==ENG_RUS&&ui.mac[0][ENG_RUS][1]==43);
    assert(loop[0].speed_pct==137&&gr_edit[0][2]==31&&ui.env_on[0][ENG_RUS]&&ui.env[0][ENG_RUS][3]==17);
    puts("Presets: 48 native factory defaults, user overrides, session recall, ADSR, grains and track speed OK");
    /* Audible factory examples through the full production mixer. */
    for(unsigned e=0;e<2;e++) {
        grain_reset();ses_scan(1);unsigned engine=e?ENG_RUS:ENG_BAY;
        assert(test_load(0,engine,e?10:1)==SES_OK);run_blocks(24,0);ui.sel=0;master_q12=1500;wav_ch=2;
        for(unsigned b=0;b<11025;b++) {
            if(e) {if(b==0||b==5512)press_key(4,1);if(b==4000||b==10000)press_key(4,0);}
            else {
                unsigned pos=b%345,step=b/345;
                if(pos==0){press_key(4,1);if(step%8==0)press_key(0,1);if(step%8==4)press_key(2,1);if(step%7==6)press_key(11,1);}
                if(pos==18){press_key(4,0);press_key(0,0);press_key(2,0);press_key(11,0);}
            }
            host_pump();for(unsigned j=0;j<CTL*2;j++)wav_put(mix_out[j]);
        }
        wav_write(e?"build/engines/fract-buffer.wav":"build/engines/pulse-dry.wav");
    }
    wav_ch=1;
    /* Four groups, one macro per group: -64, 0, +64 on a sustained metal body. */
    for(unsigned m=0;m<4;m++)for(int value=-64;value<=64;value+=64) {
        ba_design_t d;ba_voice_t v;int32_t b[CTL];ba_design(&d,1,77491,PATH_OPEN,GEN_NOW);ba_init(&v,0);
        ba_note_on(&v,&d,58,127,1);v.hold=1;v.mac[m]=value;
        for(unsigned n=0;n<1378;n++){ba_render(&v,&d,b,CTL);for(unsigned i=0;i<CTL;i++)wav_put(b[i]);}
        for(unsigned n=0;n<4410;n++)wav_put(0);
    }
    wav_write("build/engines/ryuji-macros.wav");
    /* Same kick through the mixer, short then long release; held for ~145 ms. */
    wav_ch=2;
    for(unsigned pass=0;pass<2;pass++) {
        ryuji_start(0,50,100,pass?85:10);run_blocks(200,1);note_off(5,48,SRC_KEY);run_blocks(9000,1);
    }
    wav_write("build/engines/ryuji-release.wav");
    bocjuhn_checks();
    puts("Replacement engines: OK; hardware performance validation remains pending");
    return 0;
}
