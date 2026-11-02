#!/bin/sh
# Download build dependencies only. Never opens MIDI or USB devices.
set -eu
cd /work
mkdir -p build/deps
if [ ! -x build/deps/jieli/toolchain/pi32v2/bin/clang ]; then
    sh tools/get_toolchain.sh /work/build/deps/jieli
fi
if [ ! -f build/deps/sdk/cpu/wl82/tools/uboot.boot ]; then
    python3 -c "import hashlib,pathlib; assert hashlib.sha256(pathlib.Path('docs/firmware/tonitor.fwsc').read_bytes()).hexdigest() == 'd44a89680f1ab9588f64b2c1991af593c6a73908663718df11c1d040640b3b32', 'upstream package changed'"
    python3 tools/sdk_from_package.py docs/firmware/tonitor.fwsc build/deps/sdk
fi
