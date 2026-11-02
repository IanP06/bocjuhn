set -eu
cd /work
mkdir -p build/particles
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/particles_test.c -lm -o build/particles/test
stdbuf -oL build/particles/test > build/particles/tests.txt
clang --target=i686-unknown-linux-gnu -O2 -std=gnu99 -DDROPLETS_PARTICLES -ffreestanding -fno-builtin -Wall -Wno-unused-function -Werror -Ifirmware/hal -Ifirmware/src -Ibuild/gen -S -emit-llvm firmware/src/tonitor.c -o build/particles/tonitor.ll
python3 - <<'PY'
import re,pathlib
s=pathlib.Path('build/particles/tonitor.ll').read_text()
pattern=r'\b(udiv|sdiv|urem|srem) i64|\bf(add|sub|mul|div|cmp|neg)\b|fptosi|fptoui|sitofp|uitofp|fpext|fptrunc|\bdouble\b|\bfloat\b|(shl|lshr|ashr) i64 %[a-z0-9._]+, %'
assert not re.search(pattern,s), 'unsupported target operation'
print('IR: no float or unsupported 64-bit operations')
PY
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/particles/firmware-core.mjs
node tests/web/test_particles_simulator.mjs > build/particles/simulator.txt
cat build/particles/tests.txt
cat build/particles/simulator.txt
