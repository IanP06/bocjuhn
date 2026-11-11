#!/bin/sh
set -eu
cd /work
mkdir -p build/rc9
python3 tools/gen_pulse.py
for t in ryuji_kit_test accompaniment_filter_test fx_clock_test; do
  clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/$t.c -o build/rc9/$t -lm
  build/rc9/$t
done
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/firmware-core.mjs
node tests/web/test_ryuji_kits.mjs
node tests/web/test_acid_clock_simulator.mjs
JIELI_TOOLCHAIN=/work/build/deps/jieli/jieli-linux-toolchains-20260730.1 AC79_SDK=/work/build/deps/sdk python3 tools/build_filter_release.py
