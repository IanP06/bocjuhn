#!/bin/sh
set -eu
cd /work
python3 tools/test_stamp.py start release
sh tests/check_target_ir.sh
for test in interval flows latch playalong loopmodes looptie tracktransp setup backup; do
    build/host "$test"
done
for test in presets macros overdubs snaps balls bricks sessions; do
    build/host "$test" build/screens
done
ANCACHE=1 build/host anref > build/anima-cache.actual.txt
diff -q build/anima-cache.actual.txt tests/golden/anima_render.txt
python3 tests/web/python_facts.py build/bocjuhn-1.0-rc3.fwsc > build/package-facts.json
node tests/web/test_fm1ota.mjs build/bocjuhn-1.0-rc3.fwsc build/package-facts.json
node tests/web/test_backup.mjs build/host
echo 'release checks: OK (hardware validation still pending; upstream paths failure documented separately)'

python3 tools/test_stamp.py finish release
