"""Offline archive integrity regression, including actual npm pack normalization."""
import hashlib
import importlib.util
import io
import os
from pathlib import Path, PureWindowsPath
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import MagicMock, patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def consumer_build_jobs():
    # Keep local packaged-consumer checks gentle; CI can explicitly opt in to
    # parallel compilation through CMake's standard environment setting.
    value = os.environ.get('CMAKE_BUILD_PARALLEL_LEVEL') or '1'
    if not value.isascii() or not value.isdecimal() or int(value) < 1:
        raise ValueError('CMAKE_BUILD_PARALLEL_LEVEL must be a positive integer')
    return str(int(value))


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


producer = load('package_bindings')
validator = load('test_binding_packages')


class SourceManifest(unittest.TestCase):
    def test_consumer_build_jobs(self):
        with patch.dict(os.environ, {}, clear=True):
            self.assertEqual(consumer_build_jobs(), '1')
            for setting, expected in [('', '1'), ('1', '1'), ('4', '4'), ('02', '2')]:
                os.environ['CMAKE_BUILD_PARALLEL_LEVEL'] = setting
                self.assertEqual(consumer_build_jobs(), expected)
            for setting in ('0', '-1', 'many', '1.5', ' 2', '٢'):
                os.environ['CMAKE_BUILD_PARALLEL_LEVEL'] = setting
                with self.subTest(setting=setting), self.assertRaises(ValueError):
                    consumer_build_jobs()

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
                for name, data in entries:
                    if mode == 'directory_source' and name == prefix + 'source.c':
                        archive.writestr(name + '/', b'')
                    else:
                        archive.writestr(name, data)
                if mode == 'link':
                    member = zipfile.ZipInfo(prefix + 'link')
                    member.create_system = 3
                    member.external_attr = (0o120777 << 16)
                    archive.writestr(member, 'source.c')
        else:
            with tarfile.open(path, 'w:gz') as archive:
                for name, data in entries:
                    member = tarfile.TarInfo(name)
                    if mode == 'directory_source' and name == prefix + 'source.c':
                        member.type = tarfile.DIRTYPE
                        archive.addfile(member)
                    else:
                        member.size = len(data)
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
                             'unlisted', 'unlisted_receipt', 'unsafe_archive', 'duplicate_archive', 'link', 'directory_source'):
                    with self.subTest(kind=kind, mode=mode), self.assertRaises(ValueError):
                        validator.verify_source_manifest(self.make_archive(directory, kind, mode))

    def test_tar_archive_order_streams_once_with_bounded_payload_reads(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'reordered.tgz'
            prefix = 'package/_source/'
            # Several chunks per file; reverse manifest order forces backwards
            # seeks in the old getmembers/extractfile implementation.
            payloads = {f'{i}.c': bytes([i]) * (150000 + i) for i in range(4)}
            manifest = ''.join(hashlib.sha256(data).hexdigest() + '  ' + name + '\n'
                               for name, data in reversed(list(payloads.items()))).encode()
            with tarfile.open(path, 'w:gz') as archive:
                for name, data in [*payloads.items(), ('SOURCE_FILES.sha256', manifest)]:
                    member = tarfile.TarInfo(prefix + name)
                    member.size = len(data)
                    archive.addfile(member, io.BytesIO(data))
            class ForwardOnly(io.BytesIO):
                read_bytes = 0
                def read(self, size=-1):
                    data = super().read(size)
                    self.read_bytes += len(data)
                    return data
                def seek(self, *args):
                    raise AssertionError('gzip input must never seek/replay')
            compressed = path.read_bytes()
            stream = ForwardOnly(compressed)
            real_open = tarfile.open
            real_extract = tarfile.TarFile.extractfile
            reads = {}
            def open_stream(*args, **kwargs):
                self.assertEqual(args[1], 'r|gz')
                return real_open(fileobj=stream, mode='r|gz')
            def extract(archive, member):
                source = real_extract(archive, member)
                if member.name.endswith('/SOURCE_FILES.sha256'):
                    return source
                original_read = source.read
                def read(size=-1):
                    self.assertGreater(size, 0)
                    self.assertLessEqual(size, 64 * 1024)
                    data = original_read(size)
                    reads[member.name] = reads.get(member.name, 0) + len(data)
                    return data
                source.read = read
                return source
            with patch.object(validator.tarfile, 'open', side_effect=open_stream), \
                    patch.object(tarfile.TarFile, 'extractfile', extract):
                self.assertEqual(validator.verify_source_manifest(path), len(payloads))
            self.assertLessEqual(stream.read_bytes, len(compressed))
            self.assertEqual(reads, {prefix + name: len(data) for name, data in payloads.items()})

    def test_tar_nonregular_entries_and_receipt_exclusions(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            prefix = 'package/_source/'
            for kind in (tarfile.LNKTYPE, tarfile.FIFOTYPE, tarfile.CHRTYPE):
                path = self.make_archive(directory, 'npm', 'valid')
                # Rebuild rather than append a gzip archive.
                with tarfile.open(path, 'r:gz') as archive:
                    entries = [(m, archive.extractfile(m).read()) for m in archive if m.isfile()]
                with tarfile.open(path, 'w:gz') as archive:
                    for member, data in entries:
                        archive.addfile(member, io.BytesIO(data))
                    member = tarfile.TarInfo('outside-source/special')
                    member.type = kind
                    member.linkname = prefix + 'source.c'
                    archive.addfile(member)
                with self.subTest(kind=kind), self.assertRaises(ValueError):
                    validator.verify_source_manifest(path)
            path = self.make_archive(directory, 'npm', 'valid')
            with tarfile.open(path, 'r:gz') as archive:
                entries = [(m, archive.extractfile(m).read()) for m in archive if m.isfile()]
            with tarfile.open(path, 'w:gz') as archive:
                for member, data in entries:
                    archive.addfile(member, io.BytesIO(data))
                for name in ('bindings/go/sdk/source/SOURCE_FILES.sha256',
                             'bindings/rust/sdk/source/SOURCE_FILES.sha256'):
                    member = tarfile.TarInfo(prefix + name)
                    member.size = 7
                    archive.addfile(member, io.BytesIO(b'receipt'))
            self.assertEqual(validator.verify_source_manifest(path), 1)

    def test_unsafe_paths(self):
        # Exercise the producer with a Windows relative path on every host.
        destination = MagicMock()
        source = MagicMock()
        source.name = 'source.c'
        source.is_file.return_value = True
        source.read_bytes.return_value = b'source'
        source.relative_to.return_value = PureWindowsPath('nested/source.c')
        destination.rglob.return_value = [source]
        producer.write_source_manifest(destination)
        destination.__truediv__.return_value.write_text.assert_called_once_with(
            hashlib.sha256(b'source').hexdigest() + '  nested/source.c\n')
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
            subprocess.run([shutil.which('npm'), 'pack', '--ignore-scripts', '--pack-destination', str(directory)],
                           cwd=node, env=env, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
            self.assertGreater(validator.verify_source_manifest(next(directory.glob('*.tgz'))), 1000)
            python = producer.stage('python', directory)
            source = python / 'cui_bridge/_source'
            tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT, timeout=30).decode().split('\0')
            expected = [name for name in tracked if Path(name).name == '.gitignore'
                        and name.split('/')[0] in producer.SOURCE_DIRS]
            self.assertTrue(expected)
            for name in expected:
                self.assertEqual((source / name).read_bytes(), (ROOT / name).read_bytes())
                self.assertIn('  ' + name + '\n', (source / 'SOURCE_FILES.sha256').read_text())
            # Reproduce the actual wheel's docless CMake configuration. Consumer
            # guide compilation must not become a package build dependency.
            self.assertFalse((source / 'docs').exists())
            if shutil.which('cmake'):
                configured = subprocess.run(
                    [shutil.which('cmake'), '-S', str(source / 'bindings/bridge'),
                     '-B', str(directory / 'docless-build'), '-DCUI_BUILD_NATIVE=OFF',
                     '-DCMAKE_BUILD_TYPE=Release'],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=120)
                self.assertEqual(configured.returncode, 0, configured.stdout[-8000:])
                for name in ('include/cui/cui_date_time_picker.h',
                             'src/base/cui_date_time_picker.c',
                             'src/base/cui_date_time_internal.h',
                             'src/base/cui_picker_composition_internal.h',
                             'src/components/cui_date_time_picker.c',
                             'src/components/cui_picker_paint_internal.h'):
                    self.assertTrue((source / name).is_file(), name)
                # The bridge excludes the CUI subdirectory from its default
                # build. Visual Studio cannot address those child test projects
                # from the bridge solution by bare target name. Configure the
                # same packaged, docless CUI source as its own test project;
                # retain the separate bridge configuration check above.
                test_build = directory / 'docless-tests'
                configured_tests = subprocess.run(
                    [shutil.which('cmake'), '-S', str(source), '-B', str(test_build),
                     '-DCUI_BUILD_NATIVE=OFF', '-DCMAKE_BUILD_TYPE=Release'],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=120)
                self.assertEqual(configured_tests.returncode, 0, configured_tests.stdout[-8000:])
                built = subprocess.run(
                    [shutil.which('cmake'), '--build', str(test_build), '--config', 'Release',
                     '--target', 'test_date_time_picker', 'test_date_time_picker_styled',
                     '--parallel', consumer_build_jobs()],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=300)
                self.assertEqual(built.returncode, 0, built.stdout[-8000:])
                tested = subprocess.run(
                    [__import__('sys').executable, str(ROOT / 'scripts/test_local.py'),
                     '--build-dir', str(test_build), '--', '-C', 'Release',
                     '-R', '^date_time_picker(_styled)?$', '--output-on-failure'],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=120)
                self.assertEqual(tested.returncode, 0, tested.stdout[-8000:])



if __name__ == '__main__': unittest.main()
