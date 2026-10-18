#!/usr/bin/env python3
"""Headless configuration checks; no macOS SDK or target execution needed."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class MacOSBundleTests(unittest.TestCase):
    def configure(self, *, darwin=True, deploy=False, providers=False):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        source = pathlib.Path(temp.name)
        provider_setup = ''
        if providers:
            provider_setup = r'''# Model native deployment only; this fixture never builds Mach-O.
set(CMAKE_HOST_APPLE TRUE)
set(CMAKE_CROSSCOMPILING FALSE)
set(TEST_PROVIDER_DIR "${CMAKE_BINARY_DIR}/pinned deps \${cash} ]=]" CACHE PATH "fixture")
add_library(PkgConfig::SDL3 UNKNOWN IMPORTED)
set_target_properties(PkgConfig::SDL3 PROPERTIES
  IMPORTED_LOCATION_RELEASE "${TEST_PROVIDER_DIR}/sdl/libSDL3.0.dylib"
  IMPORTED_CONFIGURATIONS RELEASE)
add_library(PkgConfig::TEXT INTERFACE IMPORTED)
set_target_properties(PkgConfig::TEXT PROPERTIES INTERFACE_LINK_LIBRARIES
  "${TEST_PROVIDER_DIR}/text/libfreetype.6.dylib;${TEST_PROVIDER_DIR}/text/libharfbuzz.0.dylib")
add_library(provider INTERFACE)
set_target_properties(provider PROPERTIES INTERFACE_LINK_LIBRARIES
  "$<LINK_ONLY:PkgConfig::SDL3>;$<BUILD_INTERFACE:PkgConfig::TEXT>;$<INSTALL_INTERFACE:ignored-provider>")
set_target_properties(provider PROPERTIES INTERFACE_LINK_DIRECTORIES "${TEST_PROVIDER_DIR}/runtime")
add_library(provider_cycle INTERFACE)
target_link_libraries(provider INTERFACE provider_cycle)
target_link_libraries(provider_cycle INTERFACE provider)
add_library(bridge STATIC main.c)
set_target_properties(bridge PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(bridge PRIVATE provider)
target_link_libraries(probe PRIVATE bridge)
'''
        (source / 'main.c').write_text('int main(void) { return 0; }\n')
        (source / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(bundle_probe NONE)
# Configuration-only fixture: do not select or invoke a host/target compiler.
set(CMAKE_C_LINK_EXECUTABLE "unused-configuration-only-linker")
set(CMAKE_C_ARCHIVE_CREATE "unused-configuration-only-archiver")
set(CMAKE_C_ARCHIVE_FINISH "unused-configuration-only-archiver")
include("{(ROOT / 'cmake/CUIMacOSBundle.cmake').as_posix()}")
add_executable(probe main.c)
set_target_properties(probe PROPERTIES LINKER_LANGUAGE C)
{provider_setup}
cui_macos_bundle(probe FONT_DIR "{(ROOT / 'third_party/inter').as_posix()}")
get_target_property(bundle probe MACOSX_BUNDLE)
get_target_property(sources probe SOURCES)
file(WRITE "${{CMAKE_BINARY_DIR}}/properties.txt" "${{bundle}}\\n${{sources}}")
''')
        command = ['cmake', '-G', 'Ninja', '-S', str(source), '-B', str(source / 'build'),
                   '-DCUI_MACOS_BUNDLE=ON', '-DCMAKE_BUILD_TYPE=Release',
                   '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
                   f'-DCUI_MACOS_DEPLOY_DEPENDENCIES={"ON" if deploy else "OFF"}']
        if darwin:
            command += ['-DCMAKE_SYSTEM_NAME=Darwin']
        result = subprocess.run(command, capture_output=True, text=True)
        return source / 'build', result

    def test_cross_bundle_has_pinned_fonts_and_license(self):
        build, result = self.configure()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        # This is a CMake resource/guard contract, not a cross-compiler test.
        self.assertNotIn('CMAKE_C_COMPILER:FILEPATH=',
                         (build / 'CMakeCache.txt').read_text())
        properties = (build / 'properties.txt').read_text()
        self.assertTrue(properties.startswith('TRUE\n'))
        for asset in ('Inter-Regular.otf', 'Inter-Bold.otf', 'LICENSE.txt', 'manifest.json'):
            self.assertIn(asset, properties)
        self.assertIn('runtime dylib deployment and signing are not performed', result.stdout)
        install = (build / 'cmake_install.cmake').read_text()
        self.assertIn('probe.app', install)
        self.assertNotIn('fixup_bundle(', install)

    def test_native_install_searches_resolved_providers_and_revalidates(self):
        build, result = self.configure(deploy=True, providers=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        install_path = build / 'cmake_install.cmake'
        install = install_path.read_text()
        initial = build / 'pinned deps ${cash} ]=]'
        search_file = build / 'cui-bundle-probe-Release-directories.txt'
        search = search_file.read_text()
        for suffix in ('sdl', 'text', 'runtime'):
            self.assertIn((initial / suffix).as_posix(), search)
        self.assertNotIn('ignored-provider', search)
        self.assertIn(search_file.as_posix(), install)
        self.assertNotIn('$<LINK_ONLY:', search)
        self.assertIn('$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/probe.app', install)
        self.assertIn('cui_sign_macos_development_bundle', install)
        # Execute the actual generated fixup fragment with a recording module.
        # This checks CMake argument boundaries/quoting, not Mach-O resolution.
        module = build / 'modules'
        module.mkdir()
        (module / 'BundleUtilities.cmake').write_text(r'''function(fixup_bundle app libraries directories)
  if(NOT ARGC EQUAL 3 OR NOT "${libraries}" STREQUAL "")
    message(FATAL_ERROR "bad argument boundaries")
  endif()
  file(WRITE "${CMAKE_CURRENT_LIST_DIR}/observed.txt" "${app}\n${directories}")
endfunction()
''')
        fragment = re.search(r'include\(BundleUtilities\).*?fixup_bundle\([^\n]+\)', install,
                             re.DOTALL).group(0)
        script = build / 'fixup-probe.cmake'
        prefix = '/install prefix with spaces'
        stage = build / 'DESTDIR with spaces'
        script.write_text(f'set(CMAKE_MODULE_PATH [==[{module.as_posix()}]==])\n'
                          f'set(CMAKE_INSTALL_PREFIX [==[{prefix}]==])\n' + fragment + '\n')
        # DESTDIR is an environment value, not a CMake-generated path literal.
        # Preserve its native spelling in both the input and observed prefix.
        env = dict(os.environ, DESTDIR=str(stage))
        run = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True, env=env)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        observed = (build / 'observed.txt').read_text().splitlines()
        self.assertEqual(observed[0], str(stage) + prefix + '/probe.app')
        directories = observed[1].split(';')
        self.assertEqual(directories.count((initial / 'text').as_posix()), 1)
        for suffix in ('sdl', 'text', 'runtime'):
            self.assertIn((initial / suffix).as_posix(), directories)
        # No cached search list may retain a previous resolved dependency prefix.
        replacement = build / 'replacement providers'
        run = subprocess.run(['cmake', '-S', str(build.parent), '-B', str(build),
                              '-DTEST_PROVIDER_DIR=' + str(replacement)],
                             capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        regenerated = install_path.read_text()
        self.assertIn(search_file.as_posix(), regenerated)
        self.assertNotIn(initial.as_posix(), search_file.read_text())
        self.assertIn((replacement / 'sdl').as_posix(), search_file.read_text())

    def test_windows_module_path_literal_has_no_backslash_escapes(self):
        # Exercise Windows spelling on every host, including characters used by
        # the provider fixture. This is a CMake parser test, not filesystem I/O
        # against the synthetic Windows path.
        module = pathlib.PureWindowsPath(r'C:\Users\fixture\build\pinned deps ${cash} ]=]\modules')
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            observed = root / 'observed.txt'
            script = root / 'windows-path.cmake'
            script.write_text(
                f'set(CMAKE_MODULE_PATH [==[{module.as_posix()}]==])\n'
                f'file(WRITE [==[{observed.as_posix()}]==] "${{CMAKE_MODULE_PATH}}")\n')
            result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(observed.read_text(),
                             'C:/Users/fixture/build/pinned deps ${cash} ]=]/modules')
            self.assertNotIn('\\', observed.read_text())

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
            result = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                     '-D__APPLE__', '-ffunction-sections',
                                     '-I' + str(root), '-I' + str(ROOT / 'include'),
                                     str(probe), '-Wl,--gc-sections', '-o', str(root / 'probe')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(root / 'probe')], cwd='/', check=True)
            font.unlink()
            self.assertNotEqual(subprocess.run([str(root / 'probe')], cwd='/').returncode, 0)

    @unittest.skipIf(sys.platform == 'win32', 'uses a POSIX codesign test double')
    def test_development_signing_orders_libraries_before_bundle_and_verifies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            app = root / 'Probe.app'
            library = app / 'Contents/MacOS/libprobe.dylib'
            library.parent.mkdir(parents=True)
            actual_library = library.with_name('actual.dylib')
            actual_library.touch()
            library.symlink_to(actual_library)
            log = root / 'calls.txt'
            signer = root / 'codesign'
            signer.write_text(f'''#!/bin/sh
printf '%s\\n' "$*" >> '{log}'
exit 0
''')
            signer.chmod(0o755)
            script = root / 'sign.cmake'
            script.write_text(f'''set(CMAKE_HOST_APPLE TRUE)
set(CUI_CODESIGN "{signer}")
include("{ROOT / 'cmake/CUISignMacOS.cmake'}")
cui_sign_macos_development_bundle("{app}")
''')
            result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(log.read_text().splitlines(), [
                f'--force --sign - {library.resolve()}', f'--force --sign - {app}',
                f'--verify --deep --strict {app}'])
            signer.write_text('#!/bin/sh\nexit 1\n')
            result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('Development signing failed', result.stderr)

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
