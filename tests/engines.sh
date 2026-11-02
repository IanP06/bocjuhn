#!/bin/sh
set -eu
cd /work
python3 tools/test_stamp.py start engines
mkdir -p build/engines
clang -O2 -std=gnu99 -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/engines_test.c -o build/engines-test -lm
stdbuf -oL build/engines-test
clang -O2 -std=gnu99 -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/blips_compare.c -o build/blips-current -lm
clang -O2 -std=gnu99 -Wno-unused-function -DLEGACY_BLIPS -Ibuild/upstream/firmware/src -Ibuild/gen tests/host/blips_compare.c -o build/blips-original -lm
build/blips-current > build/engines/blips-current.txt
build/blips-original > build/engines/blips-original.txt
diff -q build/engines/blips-current.txt build/engines/blips-original.txt
echo 'BLIPS: 36 original/current renders bit-identical at neutral macros'

python3 tools/test_stamp.py finish engines
