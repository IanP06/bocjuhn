#!/bin/sh
set -eu
cd /work
mkdir -p build/rc8 build/screens
for test in fx_clock_test acid_clock_test; do
  clang -O1 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/$test.c -o build/rc8/$test -lm
  build/rc8/$test
done
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/rc8/host -lm
for test in interval flows latch playalong loopmodes looptie tracktransp setup backup; do build/rc8/host "$test"; done
for test in presets macros overdubs snaps balls bricks sessions mod; do build/rc8/host "$test" build/rc8; done
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/firmware-core.mjs
node tests/web/test_acid_clock_simulator.mjs
node tests/web/test_backup.mjs build/rc8/host
JIELI_TOOLCHAIN=/work/build/deps/jieli/jieli-linux-toolchains-20260730.1 AC79_SDK=/work/build/deps/sdk python3 tools/build_fx_release.py
python3 tests/web/python_facts.py build/rc8/bocjuhn-1.0-rc8.fwsc > build/rc8/package-facts.json
node tests/web/test_fm1ota.mjs build/rc8/bocjuhn-1.0-rc8.fwsc build/rc8/package-facts.json
