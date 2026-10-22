#!/usr/bin/env python3
"""Compile Calendar/DatePicker declarations and every additive implementation entry."""
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CalendarDistributionTests(unittest.TestCase):
    def test_direct_implementation_and_cpp_declarations(self):
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest('C compiler unavailable')
        spec = importlib.util.spec_from_file_location('calendar_amalgamate', ROOT / 'scripts/amalgamate.py')
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        with tempfile.TemporaryDirectory(prefix='cui-calendar-dist-') as directory:
            output = Path(directory)
            for name, sources in generator.MODULES.items():
                if (ROOT / 'include/cui' / (name + '.h')).exists():
                    (output / (name + '.h')).write_text(generator.generate(name, sources), encoding='utf-8')
            for name in ('cui_platform.h', 'cui_renderer.h'):
                (output / name).write_bytes((ROOT / 'include/cui' / name).read_bytes())
            for entry in ('cui_picker', 'cui_calendar_presentation', 'cui_calendar_behavior', 'cui_date_picker_calendar', 'cui_date_picker_actions', 'cui_date_time_picker'):
                with self.subTest(entry=entry):
                    source = output / 'calendar.c'
                    source.write_text(f'#define {entry.upper()}_IMPLEMENTATION\n#include "{entry}.h"\n', encoding='utf-8')
                    result = subprocess.run(compiler + ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                                       '-fsyntax-only', '-I' + directory, str(source)],
                                            text=True, capture_output=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            cxx = shlex.split(os.environ.get('CXX', 'c++'))
            if cxx and shutil.which(cxx[0]):
                source = output / 'calendar.cpp'
                source.write_text('#include "cui_date_time_picker.h"\n', encoding='utf-8')
                result = subprocess.run(cxx + ['-std=c++11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                              '-fsyntax-only', '-I' + directory, str(source)],
                                        text=True, capture_output=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
