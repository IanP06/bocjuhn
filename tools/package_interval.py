"""Package verified local build and upstream rollback. Does not access hardware."""
from pathlib import Path
import hashlib, json, shutil, sys, zipfile
base = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(base / 'tools'))
from fm1_install import load_package
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
proof = json.loads((base/'build/interval-build.json').read_text())
for rel, sha in proof['inputs'].items():
    assert digest(base/rel) == sha, f'Stale build: {rel}'
pkg = base/'build'/proof['file']
assert digest(pkg) == proof['sha256']
assert load_package(str(pkg),False)[0] == proof['product']
original = base/'docs/firmware/tonitor.fwsc'
assert digest(original) == 'd44a89680f1ab9588f64b2c1991af593c6a73908663718df11c1d040640b3b32'
critical = json.loads((base/'tools/critical-original.json').read_text())
for rel, sha in critical.items():
    assert hashlib.sha256((base/rel).read_bytes().replace(b'\r\n',b'\n')).hexdigest()==sha, f'Critical source changed: {rel}'
assert 'release checks: OK' in (base/'build/release-checks.log').read_text()
assert 'Replacement engines: OK' in (base/'build/engines/results.txt').read_text()
assert 'BLIPS: 36 original/current renders bit-identical' in (base/'build/engines/results.txt').read_text()
for name in ('engines','release'):
    stamp=json.loads((base/f'build/{name}-verified.json').read_text())
    assert stamp['passed']
    for rel,sha in stamp['inputs'].items():
        assert digest(base/rel)==sha,f'Stale {name} tests: {rel}'
out = base.parent/'release/bocjuhn-1.0-rc3'
out.mkdir(parents=True,exist_ok=True)
shutil.copy2(pkg,out/pkg.name)
shutil.copy2(original,out/'tonitor-original-build68.fwsc')
previous = base.parent/'release/interval-1.0-rc2/interval-1.0-rc2.fwsc'
assert digest(previous) == 'c776b60a2662a440f8c4e4e439a9bca0bae7fa77553b9f342f0a4e7ce662dfd5'
shutil.copy2(previous,out/'interval-1.0-rc2-rollback.fwsc')
shutil.copy2(base/'INSTALL-ES.txt',out/'INSTALL-ES.txt')
shutil.copy2(base/'LICENSE',out/'LICENSE.txt')
shutil.copy2(base/'build/release-checks.log',out/'checks.txt')
shutil.copy2(base/'build/engines/results.txt',out/'sound-checks.txt')
assert 'simulator: eight engines' in (base/'build/simulator-checks.log').read_text()
shutil.copy2(base/'build/simulator-checks.log',out/'simulator-checks.txt')
for p in (base/'build/screens').glob('bocjuhn-*.png'):
    shutil.copy2(p,out/p.name)
for name in ('pulse-dry.wav','fract-buffer.wav','ryuji-macros.wav','ryuji-release.wav','droplets-families.wav'):
    shutil.copy2(base/'build/engines'/name,out/name)
shutil.copy2(base/'BOCJUHN.md',out/'DESIGN.md')
# Corresponding sources, fonts, browser build and reference implementation for BLIPS.
files=[]
for name in ('firmware','tools','tests','browser','assets/fonts','build/upstream/firmware','build/upstream/tests'):
    files += [p for p in (base/name).rglob('*') if p.is_file() and '__pycache__' not in p.parts and '.git' not in p.parts]
files += [base/p for p in ('LICENSE','BOCJUHN.md','INTERVAL.md','INSTALL-ES.txt','docs/firmware/tonitor.fwsc','docs/js/fm1ota.js','docs/js/backup.js') if (base/p).is_file()]
with zipfile.ZipFile(out/'source.zip','w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(set(files)):
        z.write(p,'tonitor/'+str(p.relative_to(base)).replace('\\','/'))
    for name in ('README.md','README-legacy.md','serve.cjs','device.css','index.html','style.css','sequencer.js','app.js','device.js'):
        if (base.parent/name).is_file():z.write(base.parent/name,name)
proof['upstream_commit']='0dccfcaed372368dd1519db22f4f4c5a109132d0'
proof['critical_sources_unchanged']=critical
proof['rollback_sha256']=digest(original)
proof['interval_rc2_rollback_sha256']=digest(previous)
proof['source_archive_sha256']=digest(out/'source.zip')
proof['engines']={'CALIGULA':'DROPLETS','BAYAREA':'RYUJI','RUSSOLO':'FRACT','preserved_sound':'BLIPS (36 neutral renders bit-identical)'}
proof['session_namespace']='INT2 / format 32770; reads INTERVAL 32769; CALIGULA becomes DROPLETS; upstream namespace remains separate'
(out/'manifest.json').write_text(json.dumps(proof,indent=2)+'\n')
(out/'SHA256SUMS.txt').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in sorted(out.iterdir()) if p.suffix in ('.fwsc','.zip','.wav')))
with zipfile.ZipFile(out.parent/(out.name+'.zip'),'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(out.iterdir()):
        if p.is_file():z.write(p,out.name+'/'+p.name)
print(f'Prepared {out}\nSHA256 {proof["sha256"]}\nHardware validation: PENDING')
