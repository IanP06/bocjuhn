#!/bin/sh
set -eu
cd /work
mkdir -p build/rc7/screens build/gen
python3 tools/gen_tables.py build/gen/tonitor_tables.h
python3 tools/gen_font.py build/gen/tonitor_font.h
python3 tools/gen_pulse.py
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/rc7/host -lm
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/living_test.c -o build/rc7/living-test -lm
build/rc7/living-test
for t in interval flows latch playalong loopmodes looptie tracktransp setup backup; do build/rc7/host "$t"; done
for t in presets macros overdubs snaps balls bricks sessions; do build/rc7/host "$t" build/rc7/screens; done
clang -O1 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/acid_clock_test.c -o build/rc7/acid-clock-test -lm
build/rc7/acid-clock-test
node tests/web/test_backup.mjs build/rc7/host
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/firmware-core.mjs
node tests/web/test_acid_clock_simulator.mjs
JIELI_TOOLCHAIN=/work/build/deps/jieli/jieli-linux-toolchains-20260730.1 AC79_SDK=/work/build/deps/sdk python3 tools/build_living_release.py
python3 tests/web/python_facts.py build/rc7/bocjuhn-1.0-rc7.fwsc > build/rc7/package-facts.json
node tests/web/test_fm1ota.mjs build/rc7/bocjuhn-1.0-rc7.fwsc build/rc7/package-facts.json
echo 'RC7 software checks PASS; physical FM1 validation pending'
