set -eu
cd /work
mkdir -p build/fract-data/screens build/screens build/engines
exec > build/fract-data/validation.log 2>&1
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/fract_data_test.c -o build/fract-data/test -lm
stdbuf -oL build/fract-data/test
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/fract-data/host -lm
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/engines_test.c -o build/fract-data/engines -lm
build/fract-data/engines
for test in interval flows latch playalong loopmodes looptie tracktransp setup backup; do
 build/fract-data/host "$test"
done
for test in presets overdubs snaps balls bricks sessions obrador druid hive mod cores reverb filter crush grain delay; do
 build/fract-data/host "$test" build/fract-data/screens
done
ANCACHE=1 build/fract-data/host anref > build/fract-data/anima-cache.txt
diff -q build/fract-data/anima-cache.txt tests/golden/anima_render.txt
node tests/web/test_backup.mjs build/fract-data/host
clang --target=i686-unknown-linux-gnu -O2 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -ffreestanding -fno-builtin -Wall -Wno-unused-function -Werror -Ifirmware/hal -Ifirmware/src -Ibuild/gen -S -emit-llvm firmware/src/tonitor.c -o build/fract-data/tonitor.ll
python3 - <<'PY'
import re,pathlib
s=pathlib.Path('build/fract-data/tonitor.ll').read_text()
p=r'\b(udiv|sdiv|urem|srem) i64|\bf(add|sub|mul|div|cmp|neg)\b|fptosi|fptoui|sitofp|uitofp|fpext|fptrunc|\bdouble\b|\bfloat\b|(shl|lshr|ashr) i64 %[a-z0-9._]+, %'
assert not re.search(p,s),'unsupported target operation'
print('IR: no float or unsupported 64-bit operations')
PY
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -o browser/fract-data/firmware-core.mjs
node tests/web/test_fract_data_simulator.mjs
clang -O1 -std=gnu99 -DDROPLETS_PARTICLES -DFRACT_DATA -fsanitize=undefined -fsanitize-undefined-trap-on-error -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/fract_data_test.c -o build/fract-data/sanitized -lm
build/fract-data/sanitized
printf 'FRACT DATA validations complete; physical FM1 timing still pending.\n'
