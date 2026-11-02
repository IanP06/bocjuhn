set -eu
cd /work
mkdir -p build/rc4/screens build/screens build/engines
exec > build/rc4/regression.log 2>&1
python3 tools/test_stamp.py start rc4
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/rc4/host -lm
clang -O2 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/engines_test.c -o build/rc4/engines -lm
for test in interval flows latch playalong loopmodes looptie tracktransp setup backup; do
  build/rc4/host "$test"
done
for test in presets macros overdubs snaps balls bricks sessions; do
  build/rc4/host "$test" build/rc4/screens
done
ANCACHE=1 build/rc4/host anref > build/rc4/anima-cache.txt
diff -q build/rc4/anima-cache.txt tests/golden/anima_render.txt
node tests/web/test_backup.mjs build/rc4/host
build/rc4/engines
clang -O2 -std=gnu99 -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/blips_compare.c -o build/rc4/blips-current -lm
clang -O2 -std=gnu99 -Wno-unused-function -DLEGACY_BLIPS -Ibuild/upstream/firmware/src -Ibuild/gen tests/host/blips_compare.c -o build/rc4/blips-original -lm
build/rc4/blips-current > build/rc4/blips-current.txt
build/rc4/blips-original > build/rc4/blips-original.txt
diff -q build/rc4/blips-current.txt build/rc4/blips-original.txt
printf 'BLIPS: 36 reference renders bit-identical\n'
sh tests/particles.sh
sh tests/particles_regression.sh
python3 tests/web/python_facts.py build/rc4/bocjuhn-1.0-rc4.fwsc > build/rc4/package-facts.json
node tests/web/test_fm1ota.mjs build/rc4/bocjuhn-1.0-rc4.fwsc build/rc4/package-facts.json
python3 tools/test_stamp.py finish rc4
printf 'RC4 regression and package checks: OK; hardware timing pending\n'
