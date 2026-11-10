"""Build RC8 package without accessing a device or overwriting RC7 outputs."""
from pathlib import Path
import os,json,hashlib
import build as b
b.OUT=b.SRC/'build/rc8';b.OUT.mkdir(parents=True,exist_ok=True)
b.LDR=b.OUT/'loader'
b.CFLAGS.append('-DDROPLETS_PARTICLES')
b.PRODUCT='FM-1_197100';b.VERSION='B108'
b.fm1pkg_make.SDK=os.environ.get('AC79_SDK',str(b.SRC/'build/deps/sdk'))
b.fm1pkg_make.sdk_file('uboot.boot')
ota=b.build_loader()
img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt);errors+=b.mmio_check()
for n in notes:print(n)
for e in errors:print('FAIL',e)
if errors:raise SystemExit(1)
pkg=b.fm1pkg_make.ufw(b.fm1pkg_make.flash_image(img,b.fm1pkg_make.KEY),ota,b.PRODUCT)
name='bocjuhn-1.0-rc8.fwsc';(b.OUT/name).write_bytes(pkg)
paths=list(b.FW.rglob('*'))+list(b.GEN.glob('*.h'))+[Path(__file__),b.SRC/'tools/build.py',b.SRC/'tools/fm1pkg_make.py']
inputs={str(p.relative_to(b.SRC)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
proof={'file':name,'product':b.PRODUCT,'version':b.VERSION,'size':len(pkg),'sha256':hashlib.sha256(pkg).hexdigest(),'hardware_validated':False,'compile_defines':['DROPLETS_PARTICLES'],'checks':notes,'inputs':inputs}
(b.OUT/'build-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
print(name,len(pkg),'bytes; version',b.VERSION,'SHA256',proof['sha256'])
