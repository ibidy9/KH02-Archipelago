"""File-only regression checks; never install or touch a running game."""
import ast
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('packager', ROOT/'tools/build.py')
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)

class PackagingTests(unittest.TestCase):
    def test_python_syntax(self):
        for directory in ('worlds','payload','tools','tests'):
            for path in (ROOT/directory).rglob('*.py'):
                ast.parse(path.read_text(encoding='utf-8-sig'), filename=str(path))

    def test_archive_reproducible(self):
        first = packager.archive({'b.txt':b'b','a.txt':b'a'})
        self.assertEqual(first, packager.archive({'a.txt':b'a','b.txt':b'b'}))

    def test_wrong_release_refused(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'wrong.apworld'
            path.write_bytes(b'not the accepted release')
            with self.assertRaisesRegex(ValueError, 'hash mismatch'):
                packager.import_release(path)

    def test_native_source_manifest(self):
        config = json.loads((ROOT/'native/build-spec.json').read_text())
        for kind, group in config.items():
            for variant, expected in group['source_hashes'].items():
                files = packager.tree(ROOT/'native'/kind/'common')
                files.update(packager.tree(ROOT/'native'/kind/'variants'/variant))
                self.assertEqual(set(files), set(expected))
                for name, digest in expected.items():
                    self.assertEqual(hashlib.sha256(files[name]).hexdigest(), digest, name)

    def test_built_package(self):
        path = ROOT/'dist/kh02.apworld'
        if not path.exists():
            self.skipTest('Run package first to validate generated archives')
        with zipfile.ZipFile(path) as z:
            manifest = json.loads(z.read('kh02/client-manifest.json'))
            data = z.read('kh02/client-payload.zip')
        self.assertEqual(hashlib.sha256(data).hexdigest(), manifest['sha256'])
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            self.assertEqual(set(z.namelist()), set(manifest['files']))
            for name, digest in manifest['files'].items():
                self.assertEqual(hashlib.sha256(z.read(name)).hexdigest(), digest, name)
            catalog = json.loads(z.read('runtime-catalog.json'))
            self.assertEqual(len(catalog['runtimes']), 8)
            for row in catalog['runtimes']:
                for field in ('startup','transport','prototype','menu'):
                    item = row[field]
                    self.assertEqual(hashlib.sha256(z.read(item['path'])).hexdigest(), item['sha256'])

if __name__ == '__main__':
    unittest.main()
