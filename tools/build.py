"""Offline packager. Does not install, launch, connect, or access game saves."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def sha(data):
    return hashlib.sha256(data).hexdigest()

def tree(path):
    return {p.relative_to(path).as_posix(): p.read_bytes() for p in sorted(path.rglob('*'))
            if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc'}

def archive(files):
    output = io.BytesIO()
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(files.items()):
            member = zipfile.ZipInfo(name, date_time=(2026, 10, 19, 0, 0, 0))
            member.compress_type = zipfile.ZIP_DEFLATED
            member.external_attr = 0o644 << 16
            z.writestr(member, data)
    return output.getvalue()

def import_release(path):
    data = path.read_bytes()
    spec = json.loads((ROOT/'tools/release-inputs.json').read_text())
    if sha(data) != spec['apworld_sha256']:
        raise ValueError('Use the accepted Steam public-beta APWorld; release hash mismatch')
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        payload = z.read('kh02/client-payload.zip')
    with zipfile.ZipFile(io.BytesIO(payload)) as z:
        for name, expected in spec['files'].items():
            member = PurePosixPath(name)
            if member.is_absolute() or '..' in member.parts or ':' in name or '\\' in name:
                raise ValueError('Unsafe input filename')
            content = z.read(name)
            if sha(content) != expected:
                raise ValueError('Input hash mismatch: '+name)
            target = ROOT/'vendor'/name
            if target.exists() and target.read_bytes() != content:
                raise ValueError('Existing vendor input differs: '+name)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
    print('Verified release inputs imported into ignored vendor directory.')

def build(runtime_dir=None):
    spec = json.loads((ROOT/'tools/release-inputs.json').read_text())
    payload = {}
    for name, expected in spec['files'].items():
        p = ROOT/'vendor'/name
        if not p.is_file():
            raise ValueError('Missing release inputs. Run import-release first.')
        content = p.read_bytes()
        if sha(content) != expected:
            raise ValueError('Modified vendor input: '+name)
        payload[name] = content
    payload.update(tree(ROOT/'payload'))
    catalog = json.loads(payload['runtime-catalog.json'])
    if runtime_dir:
        for row in catalog['runtimes']:
            for key in ('startup','transport'):
                entry = row[key]
                relative = Path(entry['path']).relative_to('runtime')
                content = (runtime_dir/relative).read_bytes()
                payload[entry['path']] = content
                entry['sha256'] = sha(content)
        payload['runtime-catalog.json'] = (json.dumps(catalog, indent=2)+'\n').encode()
    for row in catalog['runtimes']:
        for key in ('startup','transport','prototype','menu'):
            entry = row[key]
            if sha(payload[entry['path']]) != entry['sha256']:
                raise ValueError('Catalog mismatch: '+entry['path'])
    packed = archive(payload)
    manifest = {'sha256':sha(packed), 'files':{n:sha(b) for n,b in sorted(payload.items())}}
    world = tree(ROOT/'worlds/kh02')
    world['client-payload.zip'] = packed
    world['client-manifest.json'] = (json.dumps(manifest, indent=2)+'\n').encode()
    output = ROOT/'dist'
    output.mkdir(exist_ok=True)
    (output/'kh02.apworld').write_bytes(archive({'kh02/'+n:b for n,b in world.items()}))
    (output/'kh02-poptracker-0.4.11.zip').write_bytes(archive(tree(ROOT/'poptracker/kh02-ap')))
    print('Built dist/kh02.apworld and dist/kh02-poptracker-0.4.11.zip')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    imp = commands.add_parser('import-release')
    imp.add_argument('apworld', type=Path)
    package = commands.add_parser('package')
    package.add_argument('--runtime-dir', type=Path,
                         help='Explicitly use rebuilt native DLLs (requires new live validation).')
    args = parser.parse_args()
    if args.command == 'import-release':
        import_release(args.apworld)
    else:
        build(args.runtime_dir)

if __name__ == '__main__':
    main()
