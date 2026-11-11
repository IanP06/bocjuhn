#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "tonitor_tables.h"
#include "dsp.h"
#include "ryuji_va.h"
int main(void) {
 for(unsigned family=1;family<=2;family++)for(unsigned model=1;model<=5;model++)
 for(unsigned tuning=0;tuning<3;tuning++)for(int damping=-64;damping<=64;damping+=64) {
  ry_va_t v;ry_va_init(&v,model);uint32_t ratio=tuning==0?128:tuning==1?32768:8388608;
  for(unsigned age=0;age<132300;age++) {
   int32_t x=ry_va_tick(&v,family,model==1?0:7,model,age,ratio,damping,64);
   assert(x>=-24000 && x<=24000);
  }
 }
 puts("VA UBSAN PASS: all models, families, register extremes and damping extremes");
}
