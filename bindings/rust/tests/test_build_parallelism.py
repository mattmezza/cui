#!/usr/bin/env python3
"""Offline actual build.rs/fake-CMake contract; never builds an SDK or opens UI."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(sys.platform == 'linux' and shutil.which('rustc'), 'requires an existing Linux Rust compiler; no installation')
class RustBuildParallelism(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='cui-rust-build-contract-')
        cls.root = Path(cls.temporary.name)
        cls.script = cls.root / 'build-script'
        crate = Path(__file__).resolve().parents[1]
        subprocess.run(['rustc', '--edition=2021', '-Dwarnings', crate / 'build.rs', '-o', cls.script], check=True)
        cls.fake = cls.root / 'bin'
        cls.fake.mkdir()
        cmake = cls.fake / 'cmake'
        cmake.write_text('#!' + sys.executable + '\n'
                         'import json,os,sys\n'
                         'with open(os.environ["CUI_FAKE_LOG"],"a") as f:\n'
                         ' f.write(json.dumps({"argv":sys.argv[1:],"num_jobs":os.environ.get("NUM_JOBS")})+"\\n")\n')
        cmake.chmod(0o755)
        pkg = cls.fake / 'pkg-config'
        pkg.write_text('#!' + sys.executable + '\nprint("-L/fixture/lib -lSDL3 -lfreetype -lharfbuzz")\n')
        pkg.chmod(0o755)
        cls.manifest = cls.root / 'crate'
        (cls.manifest / 'sdk/source').mkdir(parents=True)
        (cls.manifest / 'sdk/source/CMakeLists.txt').write_text('# fixture; fake CMake must never execute a build\n')

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def invoke(self, jobs, native=False):
        log = self.root / 'calls.jsonl'
        if log.exists():
            log.unlink()
        env = dict(os.environ, PATH=str(self.fake) + os.pathsep + os.environ.get('PATH', ''),
                   CUI_FAKE_LOG=str(log), CARGO_CFG_TARGET_OS='linux', HOST='fixture-linux',
                   TARGET='fixture-linux', CARGO_MANIFEST_DIR=str(self.manifest), OUT_DIR=str(self.root / 'out'))
        for name in ['CUI_BRIDGE_LIB_DIR', 'CARGO_FEATURE_NATIVE', 'CARGO_FEATURE_SDL', 'NUM_JOBS', 'CUI_PACKAGE_BUILD_TYPE']:
            env.pop(name, None)
        if jobs is not None:
            env['NUM_JOBS'] = jobs
        if native:
            env['CARGO_FEATURE_NATIVE'] = '1'
        result = subprocess.run([self.script], env=env, text=True, capture_output=True, check=True)
        calls = [json.loads(line) for line in log.read_text().splitlines()]
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[0]['argv'][:2], ['-S', str(self.manifest / 'packaging')])
        self.assertIn('-DCUI_PACKAGE_ROOT=' + str(self.manifest / 'sdk/source'), calls[0]['argv'])
        self.assertIn('-DCUI_BUILD_NATIVE=' + ('ON' if native else 'OFF'), calls[0]['argv'])
        self.assertIn('-DCMAKE_BUILD_TYPE=Release', calls[0]['argv'])
        self.assertEqual(calls[1]['argv'][:4], ['--build', str(self.root / 'out/cui-native'), '--target', 'cui_rust_bridge'])
        self.assertEqual(calls[1]['num_jobs'], jobs)
        self.assertIn('cargo:rustc-cfg=cui_bridge_static', result.stdout)
        self.assertIn('cargo:rustc-link-lib=static=cui_rust_bridge', result.stdout)
        self.assertIn('cargo:rustc-link-lib=static=cui', result.stdout)
        self.assertIn('cargo:rustc-link-lib=m', result.stdout)
        return calls[1]['argv'][4:], result.stdout

    def test_cargo_one_job(self):
        arguments, _ = self.invoke('1')
        self.assertEqual(arguments, ['-j1'])

    def test_cargo_multiple_jobs_and_native_linkage(self):
        arguments, output = self.invoke('3', native=True)
        self.assertEqual(arguments, ['-j3'])
        for library in ['cui_sdl3', 'cui_text', 'cui_bidi']:
            self.assertIn('cargo:rustc-link-lib=static=' + library, output)
        for library in ['SDL3', 'freetype', 'harfbuzz']:
            self.assertIn('cargo:rustc-link-lib=' + library, output)

    def test_invalid_or_missing_falls_back_to_one(self):
        for jobs in [None, '', '0', '-2', 'invalid', ' 2', str(1 << 256)]:
            with self.subTest(jobs=jobs):
                arguments, _ = self.invoke(jobs)
                self.assertEqual(arguments, ['-j1'])


if __name__ == '__main__':
    unittest.main()
