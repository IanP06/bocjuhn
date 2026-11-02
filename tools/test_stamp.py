"""Bind a successful test run to the exact source tree. No device access."""
from pathlib import Path
import hashlib, json, sys
base=Path(__file__).resolve().parents[1]
def inputs():
    paths=list((base/'firmware').rglob('*'))+list((base/'tests').rglob('*'))+list((base/'tools').rglob('*'))+list((base/'build/gen').glob('*.h'))
    return {str(p.relative_to(base)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)
            if p.is_file() and '__pycache__' not in p.parts and p.suffix in ('.c','.h','.S','.ld','.sh','.py','.json','.mjs','.js','.ttf')}
mode,name=sys.argv[1:]
start=base/f'build/{name}-started.json';done=base/f'build/{name}-verified.json'
now=inputs()
if mode=='start':
    done.unlink(missing_ok=True)
    start.write_text(json.dumps(now,indent=2)+'\n')
else:
    assert json.loads(start.read_text())==now,'Source changed during tests; rebuild and rerun'
    done.write_text(json.dumps({'passed':True,'inputs':now},indent=2)+'\n')
    print(f'{name}: exact source verified')
