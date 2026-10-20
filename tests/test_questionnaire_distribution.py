#!/usr/bin/env python3
"""Compile legacy and additive Questionnaire implementation entry points."""
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class QuestionnaireDistributionTests(unittest.TestCase):
    def test_staged_binding_configures_without_optional_guides(self):
        if not shutil.which('cmake'):
            self.skipTest('CMake unavailable')
        spec = importlib.util.spec_from_file_location(
            'questionnaire_package_stage', ROOT / 'scripts/package_bindings.py')
        package = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(package)
        with tempfile.TemporaryDirectory(prefix='cui-questionnaire-package-') as directory:
            root = Path(directory)
            package.copy_source(root / 'source')
            self.assertFalse((root / 'source/docs').exists())
            result = subprocess.run(
                ['cmake', '-S', str(root / 'source/bindings/bridge'),
                 '-B', str(root / 'build'), '-DCUI_BUILD_NATIVE=OFF',
                 '-DCMAKE_BUILD_TYPE=Release'],
                text=True, capture_output=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_direct_implementation_and_cpp_declarations(self):
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest('C compiler unavailable')
        spec = importlib.util.spec_from_file_location('questionnaire_amalgamate', ROOT / 'scripts/amalgamate.py')
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        with tempfile.TemporaryDirectory(prefix='cui-questionnaire-dist-') as directory:
            output = Path(directory)
            for name, sources in generator.MODULES.items():
                if (ROOT / 'include/cui' / (name + '.h')).exists():
                    (output / (name + '.h')).write_text(generator.generate(name, sources), encoding='utf-8')
            for name in ('cui_platform.h', 'cui_renderer.h'):
                (output / name).write_bytes((ROOT / 'include/cui' / name).read_bytes())
            for entry in ('cui_questionnaire', 'cui_questionnaire_keyboard', 'cui_questionnaire_snapshots', 'cui_questionnaire_observer', 'cui_questionnaire_progress'):
                with self.subTest(entry=entry):
                    source = output / 'questionnaire.c'
                    source.write_text(f'#define {entry.upper()}_IMPLEMENTATION\n#include "{entry}.h"\n', encoding='utf-8')
                    result = subprocess.run(compiler + ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                                       '-fsyntax-only', '-I' + directory, str(source)],
                                            text=True, capture_output=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            cxx = shlex.split(os.environ.get('CXX', 'c++'))
            if cxx and shutil.which(cxx[0]):
                source = output / 'questionnaire.cpp'
                source.write_text('#include "cui_questionnaire_keyboard.h"\n#include "cui_questionnaire_snapshots.h"\n#include "cui_questionnaire_observer.h"\n#include "cui_questionnaire_progress.h"\n', encoding='utf-8')
                result = subprocess.run(cxx + ['-std=c++11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                              '-fsyntax-only', '-I' + directory, str(source)],
                                        text=True, capture_output=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
