#!/bin/sh
set -eu
cd /work
mkdir -p build/rc10 build/rc9
python3 tools/gen_pulse.py
for t in ryuji_hybrid_test ryuji_kit_test accompaniment_filter_test fx_clock_test; do
  clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/$t.c -o build/rc10/$t -lm
  build/rc10/$t
done
clang -O1 -fsanitize=undefined -fsanitize-undefined-trap-on-error -DHOST -Ifirmware/src -Ibuild/gen tests/host/ryuji_va_safety.c -o build/rc10/va-safety
build/rc10/va-safety
sh tests/build_browser.sh
node tests/web/test_ryuji_kits.mjs
node tests/web/test_acid_clock_simulator.mjs
JIELI_TOOLCHAIN=/work/build/deps/jieli/jieli-linux-toolchains-20260730.1 AC79_SDK=/work/build/deps/sdk python3 tools/build_ryuji_release.py
