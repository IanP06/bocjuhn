"""Package tested RC7, source and publication material. Does not access hardware."""
from pathlib import Path
import hashlib,json,shutil,zipfile
from fm1_install import load_package
b=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
proof=json.loads((b/'build/rc7/build-proof.json').read_text())
for rel,h in proof['inputs'].items():assert sha(b/rel)==h,'Stale build: '+rel
log=(b/'build/rc7-checks.log').read_text()
assert 'RC7 software checks PASS' in log
assert 'RC7 living UI:' in log and 'web installer: ok' in log
pkg=b/'build/rc7'/proof['file'];assert sha(pkg)==proof['sha256']
assert load_package(str(pkg),False)[0]==proof['product']
old=json.loads((b.parent/'release/bocjuhn-1.0-rc5/manifest.json').read_text())
def matches_old(r,h):
 raw=(b/r).read_bytes()
 return h in {hashlib.sha256(raw).hexdigest(),hashlib.sha256(raw.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')).hexdigest()}
changed=[r for r,h in old['inputs'].items() if r.startswith('firmware/') and not matches_old(r,h)]
assert set(changed)<=set(['firmware/src/ui.c','firmware/src/balls.c','firmware/src/bricks.c','firmware/src/living.h']),changed
critical=json.loads((b/'tools/critical-original.json').read_text())
for rel,h in critical.items():assert hashlib.sha256((b/rel).read_bytes().replace(b'\r\n',b'\n')).hexdigest()==h,'Critical source changed: '+rel
out=b/'release/bocjuhn-1.0-rc7';out.mkdir(parents=True,exist_ok=True)
shutil.copy2(pkg,out/pkg.name)
shutil.copy2(b.parent/'release/bocjuhn-1.0-rc5/tonitor-original-build68.fwsc',out/'tonitor-original-build68.fwsc')
guide=(b.parent/'release/bocjuhn-1.0-rc5/INSTALL-ES.txt').read_text(encoding='utf-8').replace('RC5','RC7').replace('rc5','rc7')
guide='bocjuhn RC7 — actualización visual; audio y sesiones de RC5 conservados.\nPrueba física RC7 PENDIENTE. Consultar docs/HARDWARE-TEST.md.\n\n'+guide
(out/'INSTALL-ES.txt').write_text(guide,encoding='utf-8')
for src,dest in [('docs/RC7.md','RELEASE-NOTES.md'),('docs/HARDWARE-TEST.md','HARDWARE-TEST.md'),('docs/HUB-SUBMISSION.txt','HUB-SUBMISSION.txt'),('ACID-CLOCK-RC5.txt','CONTROLES-ES.txt'),('LICENSE','LICENSE.txt'),('LICENSES/Plaits-MIT.txt','Plaits-MIT.txt'),('assets/fonts/Rajdhani-OFL.txt','Rajdhani-OFL.txt'),('build/rc7-checks.log','checks.txt'),('docs/card.png','droplets.png')]:shutil.copy2(b/src,out/dest)
for name in ['engine-1','engine-0','engine-3','dial','acid-menu','filter','clock']:shutil.copy2(b/'build/rc7/screens'/f'{name}.png',out/f'{name}.png')
proof.update({'hardware_validated':False,'session_format':32771,'changed_existing_firmware_files_since_rc5':changed,'new_firmware_files':['firmware/src/living.h'],'critical_sources_unchanged':True,'software_checks_log_sha256':sha(out/'checks.txt'),'tests':{str(p.relative_to(b)):sha(p) for p in [b/'tests/living.sh',b/'tests/host/living_test.c',b/'tests/host/host.c',b/'tests/host/acid_clock_test.c',b/'browser/bridge.c']}})
card=json.loads((b/'fmhub.json').read_text());assert 20<=len(card['what'])<=300;assert set(card)<=set(['what','author','kinds','play','editor','cheat','image']);assert (b/card['image']).stat().st_size<300000
# Complete browsable source is also committed; this archive is an extra.
files=[]
for folder in ['firmware','tools','tests','browser','assets','LICENSES']:
 files += [p for p in (b/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
files += [p for p in b.iterdir() if p.is_file()]
files += [b/'docs'/n for n in ['RC7.md','HARDWARE-TEST.md','HUB-SUBMISSION.txt','card.png']]
files += list((b/'docs/js').rglob('*'))
files += [b/'docs/firmware/tonitor.fwsc'] # pinned upstream SDK extraction input
with zipfile.ZipFile(out/'source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(set(files)):
  if p.is_file():z.write(p,'bocjuhn/'+p.relative_to(b).as_posix())
proof['source_archive_sha256']=sha(out/'source.zip')
(out/'manifest.json').write_text(json.dumps(proof,indent=2)+'\n')
(out/'SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
local=b.parent/'release'/out.name;shutil.copytree(out,local,dirs_exist_ok=True)
with zipfile.ZipFile(local.parent/(out.name+'.zip'),'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(out.iterdir()):
  if p.is_file():z.write(p,out.name+'/'+p.name)
print('RC7 ready:',out,'SHA256',proof['sha256'],'changed firmware:',changed)
