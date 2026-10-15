"""Native requests must fail explicitly when adapters cannot be built."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRequirements(unittest.TestCase):
    def configure(self, *options):
        with tempfile.TemporaryDirectory(prefix='cui-native-requirements-') as directory:
            return subprocess.run(
                ['cmake', '-S', str(ROOT), '-B', directory,
                 '-DCUI_BUILD_NATIVE=OFF', '-DCUI_BUILD_CPP_EXAMPLE=OFF', *options],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_required_native_fails_without_adapters(self):
        result = self.configure('-DCUI_REQUIRE_NATIVE=ON')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('CUI_REQUIRE_NATIVE requires', result.stdout)

    def test_requested_starter_fails_without_adapters(self):
        result = self.configure('-DCUI_BUILD_NATIVE_STARTER=ON')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('CUI_BUILD_NATIVE_STARTER requires', result.stdout)

    def test_headless_macos_forces_dummy_and_disables_cross_execution(self):
        with tempfile.TemporaryDirectory(prefix='cui-macos-headless-') as directory:
            result = subprocess.run(
                ['cmake', '-S', str(ROOT), '-B', directory,
                 '-DCUI_BUILD_NATIVE=OFF', '-DCUI_BUILD_CPP_EXAMPLE=OFF',
                 '-DCMAKE_SYSTEM_NAME=Darwin', '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
                 '-DCUI_HEADLESS_NATIVE_TESTS=ON'],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 0, result.stdout)
            tests = (Path(directory) / 'CTestTestfile.cmake').read_text()
            self.assertIn('SDL_VIDEODRIVER=dummy', tests)
            self.assertIn('SDL_RENDER_DRIVER=software', tests)
            self.assertIn('DISABLED "TRUE"', tests)

    def test_headless_macos_rejects_gpu_tests(self):
        result = self.configure('-DCMAKE_SYSTEM_NAME=Darwin',
                                '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
                                '-DCUI_HEADLESS_NATIVE_TESTS=ON', '-DCUI_GPU_TESTS=ON')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('Headless native checks cannot enable GPU', result.stdout)

    def test_portable_build_remains_available(self):
        result = self.configure()
        self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == '__main__':
    unittest.main()
