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

    def test_root_configures_with_cpp_disabled(self):
        if not shutil.which('cmake'):
            self.skipTest('CMake unavailable')
        generator = ['-G', 'Ninja'] if os.name != 'nt' and shutil.which('ninja') else []
        with tempfile.TemporaryDirectory(prefix='cui-questionnaire-c-only-') as directory:
            result = subprocess.run(
                ['cmake', *generator, '-S', str(ROOT), '-B', directory,
                 '-DCUI_BUILD_NATIVE=OFF', '-DCUI_BUILD_BIDI=OFF',
                 '-DCUI_BUILD_REGEX=OFF', '-DCUI_BUILD_CPP_EXAMPLE=OFF'],
                text=True, capture_output=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn('CXX compiler identification', result.stdout)

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
            for entry in ('cui_questionnaire', 'cui_questionnaire_keyboard', 'cui_questionnaire_snapshots', 'cui_questionnaire_observer', 'cui_questionnaire_progress', 'cui_questionnaire_presentation', 'cui_questionnaire_geometry'):
                with self.subTest(entry=entry):
                    source = output / 'questionnaire.c'
                    source.write_text(f'#define {entry.upper()}_IMPLEMENTATION\n#include "{entry}.h"\n', encoding='utf-8')
                    result = subprocess.run(compiler + ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                                       '-fsyntax-only', '-I' + directory, str(source)],
                                            text=True, capture_output=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    # Linking against a full archive could hide a missing new
                    # dependency. Prove both standalone entries define it in
                    # their own object, before any archive participates.
                    if entry in ('cui_questionnaire', 'cui_questionnaire_presentation', 'cui_questionnaire_geometry') and shutil.which('nm'):
                        obj = output / 'standalone.o'
                        result = subprocess.run(compiler + ['-std=c11', '-I' + directory,
                                                           '-c', str(source), '-o', str(obj)],
                                                text=True, capture_output=True, timeout=60)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        symbols = subprocess.run(['nm', '-g', str(obj)], text=True,
                                                 capture_output=True, timeout=30)
                        self.assertEqual(symbols.returncode, 0, symbols.stdout + symbols.stderr)
                        definitions = {parts[-1].lstrip('_') for line in symbols.stdout.splitlines()
                                       if len(parts := line.split()) >= 3 and parts[-2].upper() == 'T'}
                        self.assertIn('cui_questionnaire_error_ex', definitions)
                        self.assertIn('cui_questionnaire_error_control', definitions)
                        self.assertIn('cui_questionnaire_geometry_resolve', definitions)
                        self.assertIn('cui_questionnaire_choice_control_ex', definitions)
            cxx = shlex.split(os.environ.get('CXX', 'c++'))
            if cxx and shutil.which(cxx[0]):
                source = output / 'questionnaire.cpp'
                source.write_text('#include "cui_questionnaire_keyboard.h"\n#include "cui_questionnaire_snapshots.h"\n#include "cui_questionnaire_observer.h"\n#include "cui_questionnaire_progress.h"\n#include "cui_questionnaire_presentation.h"\n#include "cui_questionnaire_geometry.h"\n', encoding='utf-8')
                result = subprocess.run(cxx + ['-std=c++11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                              '-fsyntax-only', '-I' + directory, str(source)],
                                        text=True, capture_output=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
