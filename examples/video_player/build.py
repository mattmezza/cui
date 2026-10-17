#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build the optional next-batch example in an external CMake harness."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', required=True, type=Path)
    parser.add_argument('--font', type=Path, default=Path('/usr/share/fonts/noto/NotoSans-Regular.ttf'))
    parser.add_argument('--test', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    build = args.build_dir.resolve()
    source = build / 'harness'
    source.mkdir(parents=True, exist_ok=True)
    # CMake quoted arguments preserve spaces; reject metacharacters instead of
    # interpreting arbitrary CMake fragments from command-line paths.
    def quote(path):
        value = str(path)
        if any(char in value for char in '\n\r";$'):
            raise ValueError('unsupported CMake path characters')
        return f'"{value}"'
    versions = subprocess.check_output(['pkg-config', '--modversion', 'libavformat',
                                        'libavcodec', 'libavutil', 'libswscale'], text=True)
    audited = ['63.1.102', '63.1.102', '61.1.102', '10.1.102']
    if versions.split() != audited:
        raise RuntimeError(f'Example audit pins FFmpeg library versions {audited}; found {versions.split()}')
    metadata = 'pkg-config library versions:\n' + versions
    for arguments in (['ffmpeg', '-version'], ['ffmpeg', '-buildconf'], ['ffmpeg', '-L']):
        metadata += '\n' + subprocess.check_output(arguments, text=True, stderr=subprocess.STDOUT)
    (build / 'ffmpeg-build-audit.txt').write_text(metadata)
    qroot, qexample, qfont = quote(root), quote(root / 'examples/video_player'), quote(args.font.resolve())
    cmake = f'''cmake_minimum_required(VERSION 3.20)
project(cui_optional_video C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} -Wall -Wextra -Wpedantic -Werror")
set(CUI_BUILD_NATIVE ON CACHE BOOL "" FORCE)
set(CUI_REQUIRE_NATIVE ON CACHE BOOL "" FORCE)
set(CUI_BUILD_CPP_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory({qroot} cui)
enable_testing()
find_package(PkgConfig REQUIRED)
pkg_check_modules(VIDEO_FFMPEG REQUIRED IMPORTED_TARGET libavformat=63.1.102 libavcodec=63.1.102 libavutil=61.1.102 libswscale=10.1.102)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(EXAMPLE {qexample})
add_library(video_controls STATIC "${{EXAMPLE}}/../../src/components/cui_video_controls.c")
target_link_libraries(video_controls PUBLIC cui)
add_library(video_decoder STATIC "${{EXAMPLE}}/decoder.c")
target_link_libraries(video_decoder PUBLIC video_controls PkgConfig::VIDEO_FFMPEG Threads::Threads)
add_executable(video_player "${{EXAMPLE}}/main.c")
target_link_libraries(video_player PRIVATE video_decoder cui_sdl3)
add_executable(test_video_decoder "${{EXAMPLE}}/test_decoder.c")
target_link_libraries(test_video_decoder PRIVATE video_decoder cui_software)
foreach(name slider_controlled video_controls)
 add_executable(test_${{name}} "${{EXAMPLE}}/../../tests/unit/test_${{name}}.c")
 target_link_libraries(test_${{name}} PRIVATE video_controls cui_software)
 add_test(NAME next_${{name}} COMMAND test_${{name}})
endforeach()
add_test(NAME next_video_fixture COMMAND ${{Python3_EXECUTABLE}} "${{EXAMPLE}}/make_fixture.py" "${{CMAKE_BINARY_DIR}}/fixture.mkv")
set_tests_properties(next_video_fixture PROPERTIES FIXTURES_SETUP video_clip)
add_test(NAME next_video_decode COMMAND test_video_decoder "${{CMAKE_BINARY_DIR}}/fixture.mkv")
set_tests_properties(next_video_decode PROPERTIES FIXTURES_REQUIRED video_clip TIMEOUT 30)
add_test(NAME next_video_native COMMAND ${{Python3_EXECUTABLE}} "${{EXAMPLE}}/test_native.py" $<TARGET_FILE:video_player> {qfont} "${{CMAKE_BINARY_DIR}}/fixture.mkv")
set_tests_properties(next_video_native PROPERTIES FIXTURES_REQUIRED video_clip TIMEOUT 40 SKIP_RETURN_CODE 77)
'''
    (source / 'CMakeLists.txt').write_text(cmake)
    subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=Debug'], check=True)
    subprocess.run(['cmake', '--build', str(build), '--target', 'video_player',
                    'test_video_decoder', 'test_slider_controlled', 'test_video_controls',
                    'test_base', '-j', '4'], check=True)
    if args.test:
        return subprocess.run([sys.executable, str(root / 'scripts/test_local.py'),
                               '--build-dir', str(build), '--', '-R', '^next_|^base$', '-V']).returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
