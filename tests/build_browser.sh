#!/bin/sh
set -eu
cd /work
mkdir -p build/gen
python3 tools/gen_tables.py build/gen/tonitor_tables.h
python3 tools/gen_font.py build/gen/tonitor_font.h
python3 tools/gen_pulse.py
emcc -O3 -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen browser/bridge.c \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createTonitor \
  -sENVIRONMENT=web,worker,node -sINVOKE_RUN=0 -sFILESYSTEM=0 \
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 \
  -o browser/firmware-core.mjs
