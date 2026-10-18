"""Offline archive integrity regression, including actual npm pack normalization."""
import hashlib
import importlib.util
import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


producer = load('package_bindings')
validator = load('test_binding_packages')


class SourceManifest(unittest.TestCase):
    def make_archive(self, directory, kind, mode):
        path = directory / ('sample.whl' if kind == 'wheel' else 'sample.tgz')
        prefix = 'cui_bridge/_source/' if kind == 'wheel' else 'package/_source/'
        digest = hashlib.sha256(b'original').hexdigest()
        manifest = digest + '  source.c\n'
        entries = [(prefix + 'source.c', b'changed' if mode == 'changed' else b'original')]
        if mode == 'absent': entries = []
        if mode == 'duplicate': manifest += manifest
        if mode == 'unsafe_manifest': manifest = digest + '  ../source.c\n'
        if mode == 'malformed': manifest = 'invalid\n'
        if mode == 'unlisted': entries.append((prefix + 'extra.c', b'extra'))
        if mode == 'unlisted_receipt': entries.append((prefix + 'unexpected/SOURCE_FILES.sha256', b'extra'))
        if mode == 'unsafe_archive': entries.append(('../escape', b'bad'))
        if mode == 'duplicate_archive': entries += entries
        if mode != 'no_manifest':
            entries.append((prefix + 'SOURCE_FILES.sha256', b'' if mode == 'empty' else manifest.encode()))
        if kind == 'wheel':
            with zipfile.ZipFile(path, 'w') as archive:
                for name, data in entries: archive.writestr(name, data)
                if mode == 'link':
                    member = zipfile.ZipInfo(prefix + 'link')
                    member.create_system = 3
                    member.external_attr = (0o120777 << 16)
                    archive.writestr(member, 'source.c')
        else:
            with tarfile.open(path, 'w:gz') as archive:
                for name, data in entries:
                    member = tarfile.TarInfo(name); member.size = len(data)
                    archive.addfile(member, io.BytesIO(data))
                if mode == 'link':
                    member = tarfile.TarInfo(prefix + 'link')
                    member.type = tarfile.SYMTYPE; member.linkname = 'source.c'
                    archive.addfile(member)
        return path

    def test_archive_validation(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for kind in ('wheel', 'npm'):
                self.assertEqual(validator.verify_source_manifest(self.make_archive(directory, kind, 'valid')), 1)
                for mode in ('changed', 'absent', 'no_manifest', 'empty', 'duplicate', 'unsafe_manifest', 'malformed',
                             'unlisted', 'unlisted_receipt', 'unsafe_archive', 'duplicate_archive', 'link'):
                    with self.subTest(kind=kind, mode=mode), self.assertRaises(ValueError):
                        validator.verify_source_manifest(self.make_archive(directory, kind, mode))

    def test_unsafe_paths(self):
        for path in ('', '/absolute', '../a', 'a/../b', './a', 'a//b', 'a\\b', 'C:/a', 'a\0b'):
            with self.subTest(path=path), self.assertRaises(ValueError): validator.archive_path(path)

    @unittest.skipUnless(shutil.which('npm'), 'npm required for actual offline pack regression')
    def test_real_npm_pack_and_python_source_retention(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            node = producer.stage('node', directory)
            node_source = node / '_source'
            self.assertFalse(list(node_source.rglob('.gitignore')))
            self.assertNotIn('.gitignore', (node_source / 'SOURCE_FILES.sha256').read_text())
            env = dict(os.environ, npm_config_cache=str(directory / 'npm-cache'), npm_config_offline='true')
            subprocess.run(['npm', 'pack', '--ignore-scripts', '--pack-destination', str(directory)],
                           cwd=node, env=env, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertGreater(validator.verify_source_manifest(next(directory.glob('*.tgz'))), 1000)
            python = producer.stage('python', directory)
            source = python / 'cui_bridge/_source'
            tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
            expected = [name for name in tracked if Path(name).name == '.gitignore'
                        and name.split('/')[0] in producer.SOURCE_DIRS]
            self.assertTrue(expected)
            for name in expected:
                self.assertEqual((source / name).read_bytes(), (ROOT / name).read_bytes())
                self.assertIn('  ' + name + '\n', (source / 'SOURCE_FILES.sha256').read_text())


if __name__ == '__main__': unittest.main()
