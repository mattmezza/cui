#!/usr/bin/env python3
"""Headless configuration checks; no macOS SDK or target execution needed."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class MacOSBundleTests(unittest.TestCase):
    def configure(self, *, darwin=True, deploy=False):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        source = pathlib.Path(temp.name)
        (source / 'main.c').write_text('int main(void) { return 0; }\n')
        (source / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(bundle_probe C)
include("{ROOT / 'cmake/CUIMacOSBundle.cmake'}")
add_executable(probe main.c)
cui_macos_bundle(probe FONT_DIR "{ROOT / 'third_party/inter'}")
get_target_property(bundle probe MACOSX_BUNDLE)
get_target_property(sources probe SOURCES)
file(WRITE "${{CMAKE_BINARY_DIR}}/properties.txt" "${{bundle}}\\n${{sources}}")
''')
        command = ['cmake', '-S', str(source), '-B', str(source / 'build'),
                   '-DCUI_MACOS_BUNDLE=ON', '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
                   f'-DCUI_MACOS_DEPLOY_DEPENDENCIES={"ON" if deploy else "OFF"}']
        if darwin:
            command += ['-DCMAKE_SYSTEM_NAME=Darwin']
        result = subprocess.run(command, capture_output=True, text=True)
        return source / 'build', result

    def test_cross_bundle_has_pinned_fonts_and_license(self):
        build, result = self.configure()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        properties = (build / 'properties.txt').read_text()
        self.assertTrue(properties.startswith('TRUE\n'))
        for asset in ('Inter-Regular.otf', 'Inter-Bold.otf', 'LICENSE.txt', 'manifest.json'):
            self.assertIn(asset, properties)
        self.assertIn('runtime dylib deployment and signing are not performed', result.stdout)
        install = (build / 'cmake_install.cmake').read_text()
        self.assertIn('probe.app', install)
        self.assertNotIn('fixup_bundle(', install)

    @unittest.skipUnless(sys.platform.startswith('linux'),
                         'uses the Linux linker to test the isolated resolver')
    def test_font_discovery_ignores_working_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            (root / 'mach-o').mkdir()
            (root / 'mach-o/dyld.h').write_text(
                '#include <stdint.h>\nint _NSGetExecutablePath(char *, uint32_t *);\n')
            font = root / 'Starter.app/Contents/Resources/inter/Inter-Regular.otf'
            font.parent.mkdir(parents=True)
            font.touch()
            executable = root / 'Starter.app/Contents/MacOS/starter'
            executable.parent.mkdir()
            probe = root / 'probe.c'
            probe.write_text(f'''#define main starter_main
#include "{ROOT / 'examples/native_starter/main.c'}"
#undef main
int _NSGetExecutablePath(char *path, uint32_t *size) {{
    const char *value = "{executable}";
    if (strlen(value) + 1 > *size) return -1;
    strcpy(path, value);
    return 0;
}}
int main(void) {{
    const char *path = bundle_font();
    return path && strstr(path, "Resources/inter/Inter-Regular.otf") ? 0 : 1;
}}
''')
            result = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic',
                                     '-D__APPLE__', '-ffunction-sections',
                                     '-I' + str(root), '-I' + str(ROOT / 'include'),
                                     str(probe), '-Wl,--gc-sections', '-o', str(root / 'probe')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(root / 'probe')], cwd='/', check=True)
            font.unlink()
            self.assertNotEqual(subprocess.run([str(root / 'probe')], cwd='/').returncode, 0)

    def test_cross_deployment_is_rejected(self):
        _, result = self.configure(deploy=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('requires a native macOS build', result.stderr)

    def test_non_darwin_bundle_is_rejected(self):
        # On macOS use an explicit Linux target to exercise this guard.
        if sys.platform == 'darwin':
            self.skipTest('host defaults to Darwin')
        _, result = self.configure(darwin=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('requires a Darwin target toolchain', result.stderr)


if __name__ == '__main__':
    unittest.main()
