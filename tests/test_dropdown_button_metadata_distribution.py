"""Prove legacy/new standalone entries define the additive API themselves."""
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]


class DropdownDistributionTests(unittest.TestCase):
    def test_package_configures_without_optional_guide(self):
        if not shutil.which('cmake'):
            self.skipTest('CMake unavailable')
        spec = importlib.util.spec_from_file_location('dropdown_package', ROOT / 'scripts/package_bindings.py')
        package = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(package)
        with tempfile.TemporaryDirectory(prefix='cui-dropdown-package-') as directory:
            source = Path(directory) / 'source'
            package.copy_source(source)
            self.assertFalse((source / 'docs').exists())
            result = subprocess.run(['cmake', '-S', str(source), '-B', str(Path(directory) / 'build'),
                '-DCUI_BUILD_NATIVE=OFF', '-DCMAKE_BUILD_TYPE=Release'], capture_output=True,
                text=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_standalone_definitions_and_cpp(self):
        cc = shlex.split(os.environ.get('CC', 'cc'))
        if not cc or not shutil.which(cc[0]):
            self.skipTest('C compiler unavailable')
        spec = importlib.util.spec_from_file_location('dropdown_amalgamate', ROOT / 'scripts/amalgamate.py')
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        # Exercise Windows' cp1252 default even on a UTF-8 development host.
        # Generated C includes localized strings and must select UTF-8 itself.
        with mock.patch('pathlib.io.text_encoding',
                        side_effect=lambda encoding, *args: 'cp1252' if encoding in (None, 'locale') else encoding), \
                tempfile.TemporaryDirectory(prefix='cui-dropdown-dist-') as directory:
            out = Path(directory)
            for name, sources in generator.MODULES.items():
                if (ROOT / 'include/cui' / (name + '.h')).exists():
                    (out / (name + '.h')).write_text(generator.generate(name, sources), encoding='utf-8')
            for name in ('cui_platform.h', 'cui_renderer.h'):
                (out / name).write_bytes((ROOT / 'include/cui' / name).read_bytes())
            for entry in ('cui_choice', 'cui_dropdown_button_metadata'):
                with self.subTest(entry=entry):
                    source = out / 'entry.c'
                    source.write_text(f'#define {entry.upper()}_IMPLEMENTATION\n#include "{entry}.h"\n', encoding='utf-8')
                    obj = out / 'entry.o'
                    result = subprocess.run(cc + ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                        '-I' + directory, '-c', str(source), '-o', str(obj)], capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    if shutil.which('nm'):
                        result = subprocess.run(['nm', '-g', str(obj)], capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        defined = {parts[-1].lstrip('_') for line in result.stdout.splitlines()
                                   if len(parts := line.split()) >= 3 and parts[-2].upper() == 'T'}
                        self.assertIn('cui_dropdown_button_with_metadata_ex', defined)
                        self.assertIn('cui_dropdown_button_with_metadata_events', defined)
            cxx = shlex.split(os.environ.get('CXX', 'c++'))
            if cxx and shutil.which(cxx[0]):
                source = out / 'entry.cpp'
                source.write_text('#include "cui_dropdown_button_metadata.h"\n', encoding='utf-8')
                result = subprocess.run(cxx + ['-std=c++11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-I' + directory, '-fsyntax-only', str(source)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
