"""Compile experimental FRACT without overwriting or installing RC4."""
import build as b
b.OUT=b.SRC/'build/fract-data/target';b.OUT.mkdir(parents=True,exist_ok=True)
b.CFLAGS+=['-DDROPLETS_PARTICLES','-DFRACT_DATA']
b.PRODUCT='FM-1_197100';b.VERSION='F001'
img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt);errors+=b.mmio_check()
for n in notes:print(n)
for e in errors:print('FAIL',e)
(b.OUT/'checks.txt').write_text('\n'.join(notes+errors)+'\n')
if errors:raise SystemExit(1)
print('Experimental target build only. Hardware timing not measured. No device accessed.')
