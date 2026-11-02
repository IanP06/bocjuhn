#!/bin/sh
set -eu
cd /work
mkdir -p build/gen build/screens build/sounds
python3 tools/gen_tables.py build/gen/tonitor_tables.h
python3 tools/gen_font.py build/gen/tonitor_font.h
python3 tools/gen_pulse.py
clang -O2 -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-unused-function -Werror -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/host -lm
