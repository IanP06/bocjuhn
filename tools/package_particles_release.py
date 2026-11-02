"""Publish verified RC4 and an exact RC3 rollback; no hardware access."""
from pathlib import Path
import hashlib,json,shutil,zipfile,sys
base=Path(__file__).resolve().parents[1];sys.path.insert(0,str(base/'tools'))
from fm1_install import load_package
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
proof=json.loads((base/'build/rc4/build-proof.json').read_text())
for rel,digest in proof['inputs'].items():assert sha(base/rel)==digest, 'stale build: '+rel
stamp=json.loads((base/'build/rc4-verified.json').read_text());assert stamp['passed']
for rel,digest in stamp['inputs'].items():assert sha(base/rel)==digest, 'stale tests: '+rel
log=(base/'build/rc4/regression.log').read_text();assert 'RC4 regression and package checks: OK' in log
pkg=base/'build/rc4'/proof['file'];assert sha(pkg)==proof['sha256']
assert load_package(str(pkg),False)[0]==proof['product']
rc3=base.parent/'release/bocjuhn-1.0-rc3/bocjuhn-1.0-rc3.fwsc'
assert sha(rc3)=='c54e4f6c34c02503c715b6e42563d35a18e1242864065a028c03ab46699400f8'
critical=json.loads((base/'tools/critical-original.json').read_text())
for rel,digest in critical.items():assert hashlib.sha256((base/rel).read_bytes().replace(b'\r\n',b'\n')).hexdigest()==digest, 'critical change: '+rel
old=json.loads((rc3.parent/'manifest.json').read_text());changed=[]
for rel,digest in old['inputs'].items():
 if rel.startswith('firmware/') and sha(base/rel)!=digest:changed.append(rel)
assert changed==['firmware/src/cal.c'],changed
out=base.parent/'release/bocjuhn-1.0-rc4';out.mkdir(exist_ok=True)
shutil.copy2(pkg,out/pkg.name);shutil.copy2(rc3,out/'bocjuhn-1.0-rc3-rollback.fwsc')
for name in ['INSTALL-ES.txt','PRUEBAS.txt']:shutil.copy2(base.parent/'work/rc4'/name,out/name)
shutil.copy2(base/'build/rc4/regression.log',out/'checks.txt')
shutil.copy2(base/'LICENSE',out/'LICENSE.txt')
shutil.copy2(base/'build/particles/tests.txt',out/'particle-checks.txt')
shutil.copy2(base/'build/particles/simulator.txt',out/'simulator-checks.txt')
shutil.copy2(base/'LICENSES/Plaits-MIT.txt',out/'Plaits-MIT.txt')
files=[]
for name in ['firmware','tools','tests','browser','assets/fonts','LICENSES','build/gen','build/upstream/firmware','build/upstream/tests']:
 files += [p for p in (base/name).rglob('*') if p.is_file() and '__pycache__' not in p.parts and '.git' not in p.parts]
files += [base/'LICENSE',base/'BOCJUHN.md',base/'INTERVAL.md',base/'docs/js/fm1ota.js',base/'docs/js/fm1backup.js']
with zipfile.ZipFile(out/'source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(set(files)):z.write(p,'tonitor/'+p.relative_to(base).as_posix())
 for name in ['serve.cjs','device.css']:z.write(base.parent/name,name)
proof.update({'rollback_file':'bocjuhn-1.0-rc3-rollback.fwsc','rollback_sha256':sha(rc3),'source_archive_sha256':sha(out/'source.zip'),'changed_existing_firmware_files':changed,'critical_sources_unchanged':True,'regression_log_sha256':sha(out/'checks.txt'),'sessions':'INT2/32770 unchanged layout; DROPLETS algorithm and macro semantics change; back up before testing'})
(out/'manifest.json').write_text(json.dumps(proof,indent=2)+'\n')
(out/'SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
zip_path=out.parent/(out.name+'.zip')
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(out.iterdir()):
  if p.is_file():z.write(p,out.name+'/'+p.name)
public=base.parent/'outputs/bocjuhn-1.0-rc4';public.mkdir(parents=True,exist_ok=True)
for name in [pkg.name,'bocjuhn-1.0-rc3-rollback.fwsc','INSTALL-ES.txt','PRUEBAS.txt','manifest.json','SHA256SUMS.txt']:shutil.copy2(out/name,public/name)
shutil.copy2(zip_path,public/zip_path.name)
assert sha(rc3)==proof['rollback_sha256']
print('RC4 ready:',out,'\nSHA256:',proof['sha256'],'\nRC3 preserved:',proof['rollback_sha256'])
