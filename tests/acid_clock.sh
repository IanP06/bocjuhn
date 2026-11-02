#!/bin/sh
set -eu
cd /work
mkdir -p build/rc5 build/screens
clang -O2 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/acid-host -lm
clang -O1 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/acid_clock_test.c -o build/acid-clock-test -lm
build/acid-clock-test
failed=0
for t in interval flows latch playalong loopmodes looptie tracktransp setup backup; do
    build/acid-host "$t" || failed=1
done
for t in presets macros overdubs snaps balls bricks sessions; do
    build/acid-host "$t" build/screens || failed=1
done
test "$failed" = 0
python3 -c 'from PIL import Image; Image.open("build/screens/clock.ppm").resize((720,720)).save("build/screens/clock.png"); Image.open("build/screens/acid.ppm").resize((720,720)).save("build/screens/acid.png")'
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/firmware-core.mjs
JIELI_TOOLCHAIN=/work/build/deps/jieli/jieli-linux-toolchains-20260730.1 AC79_SDK=/work/build/deps/sdk python3 tools/build_acid_release.py
