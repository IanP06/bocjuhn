from pathlib import Path
import build as b
b.OUT=b.SRC/'build/particles/target';b.OUT.mkdir(parents=True,exist_ok=True)
b.CFLAGS.append('-DDROPLETS_PARTICLES')
b.PRODUCT='FM-1_197100';b.VERSION='P001'
img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt);errors+=b.mmio_check()
for n in notes:print(n)
for e in errors:print('FAIL',e)
(b.OUT/'checks.txt').write_text('\n'.join(notes+errors)+'\n')
if errors:raise SystemExit(1)
print('Experimental target app fits. No installable package generated; no device accessed.')
