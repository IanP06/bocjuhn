#define main host_main
#include "host.c"
#undef main
#include <assert.h>
static ba_kit_t getkit(unsigned l){ba_kit_t k;ba_kit_get(&ba_buf[l][ba_pub[l]],&k);return k;}
static void checkkit(unsigned l,const ba_kit_t *want){ba_kit_t k=getkit(l);assert(!memcmp(&k,want,sizeof k));}
int main(void){
    grain_reset();ses_scan(1);ui.sel=5;run_blocks(100,0);
    assert(sizeof(ba_kit_t)==36 && PULSE_N==48);
    ba_design_t d;ba_design(&d,8,123,PATH_OPEN,GEN_NOW);ba_kit_t k;ba_kit_get(&d,&k);
    for(unsigned j=0;j<16;j++)assert(PULSE_ROLE[k.source[j]]==j);
    k.source[0]=PULSE_MAP[1][0];k.tune[0]=-7;k.locks=1;
    ba_kit_t a=k,b=k;ba_kit_random(&a,1234,-1);ba_kit_random(&b,1234,-1);assert(!memcmp(&a,&b,sizeof a));
    assert(a.source[0]==k.source[0] && a.tune[0]==-7 && a.locks==1);
    unsigned changed=0;for(unsigned j=0;j<16;j++){assert(PULSE_ROLE[a.source[j]]==j);changed+=a.source[j]!=k.source[j];}assert(changed>2);
    unsigned primary=0;for(unsigned seed=1;seed<=1000;seed++){ba_kit_t q=k;q.locks=0;ba_kit_random(&q,seed,2);primary+=PULSE_FAMILY[q.source[2]]==1 || PULSE_FAMILY[q.source[2]]==2;}assert(primary>700 && primary<950);
    b=k;b.locks=65535;ba_kit_t locked=b;ba_kit_random(&b,9,-1);assert(!memcmp(&b,&locked,sizeof b));
    assert(ba_edit_kit(5,&a));run_blocks(300,0);checkkit(5,&a);
    assert(pre_save(5,ENG_BAY,40)==SES_OK);assert(snap_store(0));assert(ses_save(24)==SES_OK);
    b=a;b.source[2]=PULSE_MAP[3][2];b.tune[2]=12;assert(ba_edit_kit(5,&b));run_blocks(300,0);
    assert(snap_store(1));snap_recall(0,0);run_blocks(500,0);morph_poll();checkkit(5,&a);
    snap_recall(1,0);run_blocks(500,0);morph_poll();checkkit(5,&b);
    assert(pre_load(5,ENG_BAY,40)==SES_OK);run_blocks(500,0);checkkit(5,&a);
    assert(ba_edit_kit(5,&b));run_blocks(300,0);assert(ses_load_slot(24)==SES_OK);run_blocks(500,0);checkkit(5,&a);
    snap_recall(0,0);run_blocks(500,0);morph_poll();checkkit(5,&a);
    /* Dormant RYUJI on another engine must retain its edited kit. */
    assert(ba_edit_kit(0,&b));run_blocks(300,0);assert(ses_save(25)==SES_OK);assert(ba_edit_kit(0,&a));run_blocks(300,0);
    assert(ses_load_slot(25)==SES_OK);run_blocks(500,0);checkkit(0,&b);
    /* All sources at tune extremes render bounded non-silent one-shots. */
    for(unsigned source=0;source<PULSE_N;source++)for(int tune=-24;tune<=24;tune+=24){
        ba_design(&d,6,123,PATH_OPEN,GEN_NOW);ba_kit_get(&d,&k);unsigned pad=PULSE_ROLE[source];k.source[pad]=source;k.tune[pad]=tune;ba_kit_put(&d,&k);
        ba_voice_t v;ba_init(&v,1);ba_note_on(&v,&d,48+pad,127,1);uint64_t energy=0;int32_t out[CTL];
        assert(v.source==source);for(unsigned block=0;block<1500;block++){memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);for(unsigned j=0;j<CTL;j++){assert(abs(out[j])<=24000);energy+=(int64_t)out[j]*out[j];}}
        assert(energy>10000);
    }
    /* ADSR must not turn PCM into a pitched loop: hold mode is sample-identical,
       finite, and advances at precisely the same rate for every source. */
    for(unsigned source=0;source<PULSE_N;source++) {
        ba_design(&d,6,123,PATH_OPEN,GEN_NOW);ba_kit_get(&d,&k);unsigned pad=PULSE_ROLE[source];k.source[pad]=source;ba_kit_put(&d,&k);
        ba_voice_t native,held;ba_init(&native,1);native.mac[3]=19;ba_note_on(&native,&d,48+pad,127,1);held=native;held.hold=1;
        int32_t x[CTL],y[CTL];
        for(unsigned block=0;block<5000;block++){
            memset(x,0,sizeof x);memset(y,0,sizeof y);ba_render(&native,&d,x,CTL);ba_render(&held,&d,y,CTL);
            assert(!memcmp(x,y,sizeof x));assert(native.pos==held.pos && native.live==held.live);
        }
        assert(!native.live && !held.live);
        assert(PULSE_MU[PULSE_PCM[PULSE_OFF[source]]]==0 && PULSE_MU[PULSE_PCM[PULSE_OFF[source+1]-1]]==0);
    }
    puts("RYUJI ENVELOPE PASS: all 48 sources sample-identical with hold on/off; finite tails; zero sample endpoints");
    grain_reset();ui.sel=5;ui.env_on[5][ENG_BAY]=1;
    ui.env[5][ENG_BAY][0]=0;ui.env[5][ENG_BAY][1]=0;ui.env[5][ENG_BAY][2]=100;ui.env[5][ENG_BAY][3]=100;mac_apply(5);
    press_key(0,1);run_blocks(5000,0);
    for(unsigned v=0;v<NVOICE;v++)assert(vox[v].layer!=5 || !vox[v].sounding);
    press_key(0,0);
    puts("RYUJI MIXER ENVELOPE PASS: held maximum-sustain pad ends with its one-shot");
    /* The editor opens on a held EDIT; pad keyboard selects without stealing other tracks. */
    grain_reset();ui.sel=5;run_blocks(100,0);press_phys(KEY_EDIT,1);run_blocks(900,0);ui_input();assert(ui.kit_page);press_phys(KEY_EDIT,0);
    press_key(7,1);assert(ui.kit_pad==7);press_key(7,0);assert(ui.kit_page);
    press_btn(B_FX);assert(!ui.kit_page && ui.sel==0);
    /* A reproducible audition: four families, one second each kick/snare/hat/percussion. */
    wav_ch=1;
    for(unsigned family=1;family<5;family++)for(unsigned beat=0;beat<4;beat++) {
        unsigned pad=family==4?(unsigned[]){7,13,10,14}[beat]:family==3?(unsigned[]){0,2,3,6}[beat]:(unsigned[]){0,2,4,7}[beat];
        ba_design(&d,6,123,PATH_OPEN,GEN_NOW);ba_kit_get(&d,&k);k.source[pad]=PULSE_MAP[family][pad];ba_kit_put(&d,&k);
        ba_voice_t v;ba_init(&v,1);v.mac[0]=-48;ba_note_on(&v,&d,48+pad,127,1);
        int32_t out[CTL];for(unsigned block=0;block<44100/CTL;block++){memset(out,0,sizeof out);ba_render(&v,&d,out,CTL);for(unsigned j=0;j<CTL;j++)wav_put(out[j]);}
    }
    wav_write("build/rc9/ryuji-families.wav");
    puts("RYUJI KIT PASS: sources/tune, weighted random, locks, presets, snapshots, sessions, dormant engine, envelope identity and editor");
    return 0;
}
