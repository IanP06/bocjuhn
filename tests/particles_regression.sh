set -eu
cd /work
mkdir -p build/particles/legacy-src
cp tests/fixtures/droplets-rc3.c build/particles/legacy-src/cal.c
clang -O2 -std=gnu99 -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/particles_legacy.c -lm -o build/particles/legacy-current
clang -O2 -std=gnu99 -Wno-unused-function -Ibuild/particles/legacy-src -Ifirmware/src -Ibuild/gen tests/host/particles_legacy.c -lm -o build/particles/legacy-baseline
build/particles/legacy-current > build/particles/legacy-current.txt
build/particles/legacy-baseline > build/particles/legacy-baseline.txt
diff -u build/particles/legacy-baseline.txt build/particles/legacy-current.txt
printf 'Legacy RC3: ten families bit-identical\n'
clang -O1 -g -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=gnu99 -DDROPLETS_PARTICLES -Wno-unused-function -Ifirmware/src -Ibuild/gen tests/host/particles_test.c -lm -o build/particles/sanitize
build/particles/sanitize > build/particles/sanitize.txt
printf 'Undefined behavior sanitizer: passed\n'
