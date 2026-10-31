"""The local runner must never inherit or fall back to a user's desktop."""
import importlib.util
from pathlib import Path
import tempfile
import shutil
import subprocess
import unittest

spec = importlib.util.spec_from_file_location('local_runner', Path(__file__).resolve().parents[1] / 'scripts/test_local.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class IsolatedRunner(unittest.TestCase):
    def test_linux_replaces_desktop_and_preserves_ctest_failure_arguments(self):
        original = {'DISPLAY': ':0', 'WAYLAND_DISPLAY': 'wayland-0', 'MIR_SOCKET': 'desktop', 'SDL_VIDEODRIVER': 'wayland', 'XDG_RUNTIME_DIR': '/user/session', 'KEEP': 'value'}
        command, env = runner.test_invocation(Path('build'), ['-R', 'meaningful'], 'linux', original, lambda _: '/usr/bin/xvfb-run', '/private/runtime')
        self.assertEqual(command, ['/usr/bin/xvfb-run', '-a', '-s', '-screen 0 1920x1080x24 -noreset', 'ctest', '--test-dir', 'build', '--output-on-failure', '-R', 'meaningful'])
        self.assertNotIn('DISPLAY', env)
        self.assertNotIn('WAYLAND_DISPLAY', env)
        self.assertNotIn('MIR_SOCKET', env)
        self.assertNotIn('SDL_VIDEODRIVER', env)
        self.assertEqual(env['XDG_RUNTIME_DIR'], '/private/runtime')
        self.assertEqual(env['KEEP'], 'value')
        self.assertEqual(original['DISPLAY'], ':0')

    def test_cmake_default_preserves_explicit_timeout_limits(self):
        cmake = shutil.which('cmake')
        if not cmake:
            self.skipTest('CMake is unavailable')
        source = (Path(__file__).resolve().parents[1] / 'CMakeLists.txt').read_text()
        start = source.index('get_property(CUI_REGISTERED_TESTS DIRECTORY PROPERTY TESTS)')
        end = source.index('# This offline integration check', start)
        defaults = source[start:end]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.16)\nproject(timeout_check NONE)\n'
                'enable_testing()\n'
                'add_test(NAME default_limit COMMAND unused)\n'
                'add_test(NAME explicit_limit COMMAND unused)\n'
                'add_test(NAME unlimited COMMAND unused)\n'
                'set_tests_properties(explicit_limit PROPERTIES TIMEOUT 240)\n'
                'set_tests_properties(unlimited PROPERTIES TIMEOUT 0)\n' + defaults +
                'get_test_property(default_limit TIMEOUT default_value)\n'
                'get_test_property(explicit_limit TIMEOUT explicit_value)\n'
                'get_test_property(unlimited TIMEOUT unlimited_value)\n'
                'if(NOT default_value EQUAL 60 OR NOT explicit_value EQUAL 240 '
                'OR NOT unlimited_value EQUAL 0)\n'
                ' message(FATAL_ERROR "Incorrect timeout defaults")\nendif()\n')
            result = subprocess.run([cmake, '-S', str(root), '-B', str(root / 'build')],
                                    capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_missing_virtual_display_fails_closed(self):
        with self.assertRaisesRegex(RuntimeError, 'no desktop fallback'):
            runner.test_invocation(Path('build'), [], 'linux', {'DISPLAY': ':0'}, lambda _: None)

    def test_other_platforms_require_proven_portable_build_or_hosted_session(self):
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            for platform in ('darwin', 'win32'):
                with self.assertRaisesRegex(RuntimeError, 'local desktop testing is disabled'):
                    runner.test_invocation(build, [], platform, {}, lambda _: None)
                (build / 'CMakeCache.txt').write_text('CUI_BUILD_NATIVE:BOOL=OFF\n')
                command, _ = runner.test_invocation(build, [], platform, {}, lambda _: None)
                self.assertEqual(command[0], 'ctest')
                (build / 'CMakeCache.txt').write_text('CUI_BUILD_NATIVE:BOOL=ON\n')
                with self.assertRaises(RuntimeError):
                    runner.test_invocation(build, [], platform, {}, lambda _: None)
                (build / 'CMakeCache.txt').write_text('//CUI_BUILD_NATIVE:BOOL=OFF\nCUI_BUILD_NATIVE:BOOL=ON\n')
                with self.assertRaises(RuntimeError):
                    runner.test_invocation(build, [], platform, {}, lambda _: None)
                command, _ = runner.test_invocation(build, [], platform, {'GITHUB_ACTIONS': 'true'}, lambda _: None)
                self.assertEqual(command[0], 'ctest')
                (build / 'CMakeCache.txt').unlink()

    def test_macos_native_headless_build_never_uses_cocoa(self):
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            (build / 'CMakeCache.txt').write_text(
                'CUI_BUILD_NATIVE:BOOL=ON\nCUI_HEADLESS_NATIVE_TESTS:BOOL=ON\n')
            command, env = runner.test_invocation(
                build, [], 'darwin', {'SDL_VIDEODRIVER': 'cocoa', 'SDL_RENDER_DRIVER': 'metal'}, lambda _: None)
            self.assertEqual(command[0], 'ctest')
            self.assertEqual(env['SDL_VIDEODRIVER'], 'dummy')
            self.assertEqual(env['SDL_RENDER_DRIVER'], 'software')
            with self.assertRaises(RuntimeError):
                runner.test_invocation(build, [], 'win32', {}, lambda _: None)


if __name__ == '__main__':
    unittest.main()
