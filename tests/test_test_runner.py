"""The local runner must never inherit or fall back to a user's desktop."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('local_runner', Path(__file__).resolve().parents[1] / 'scripts/test_local.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class IsolatedRunner(unittest.TestCase):
    def test_linux_replaces_desktop_and_preserves_ctest_failure_arguments(self):
        original = {'DISPLAY': ':0', 'WAYLAND_DISPLAY': 'wayland-0', 'MIR_SOCKET': 'desktop', 'SDL_VIDEODRIVER': 'wayland', 'XDG_RUNTIME_DIR': '/user/session', 'KEEP': 'value'}
        command, env = runner.test_invocation(Path('build'), ['-R', 'meaningful'], 'linux', original, lambda _: '/usr/bin/xvfb-run', '/private/runtime')
        self.assertEqual(command, ['/usr/bin/xvfb-run', '-a', '-s', '-screen 0 1920x1080x24', 'ctest', '--test-dir', 'build', '--output-on-failure', '-R', 'meaningful'])
        self.assertNotIn('DISPLAY', env)
        self.assertNotIn('WAYLAND_DISPLAY', env)
        self.assertNotIn('MIR_SOCKET', env)
        self.assertNotIn('SDL_VIDEODRIVER', env)
        self.assertEqual(env['XDG_RUNTIME_DIR'], '/private/runtime')
        self.assertEqual(env['KEEP'], 'value')
        self.assertEqual(original['DISPLAY'], ':0')

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


if __name__ == '__main__':
    unittest.main()
