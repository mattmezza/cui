#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Install, relocate and execute optional regex CMake/pkg-config Input consumers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
def run(command, **kwargs):
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
    assert result.returncode == 0, f'{command}:\n{result.stdout}'
    return result.stdout

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    build = args.build.resolve()
    sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
    with tempfile.TemporaryDirectory(prefix='regex-package-', dir=build) as directory:
        work = Path(directory)
        stage = work / 'stage'
        run(['cmake', '--install', str(build), '--prefix', str(stage)])
        prefix = work / 'relocated'
        stage.rename(prefix)
        notices = prefix / 'share/cui/licenses/regex'
        for source in (ROOT / 'backends/regex/licenses').iterdir():
            assert (notices / source.name).read_bytes() == source.read_bytes()
        assert (notices / 'pin.json').read_bytes() == (ROOT / 'backends/regex/pin.json').read_bytes()
        receipt = json.loads((notices / 'regex-build.json').read_text())
        assert receipt['library_sha256'] == hashlib.sha256((prefix / 'lib/libcui_regex.a').read_bytes()).hexdigest()
        consumer = work / 'consumer'
        consumer.mkdir()
        source = ROOT / 'tests/integration/test_input_regex_control.c'
        (consumer / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.20)\nproject(installed C)\n'
            'find_package(CUIRegex CONFIG REQUIRED)\n'
            f'add_executable(consumer "{source}")\n'
            'target_compile_options(consumer PRIVATE -std=c11 -Wall -Wextra -Wpedantic -Werror -UNDEBUG)\n'
            'target_link_libraries(consumer PRIVATE CUI::cui_regex)\n'
            + ('target_compile_options(consumer PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(consumer PRIVATE -fsanitize=address,undefined)\n' if args.sanitize else ''))
        run(['cmake', '-S', str(consumer), '-B', str(consumer / 'build'), '-G', 'Ninja', f'-DCMAKE_PREFIX_PATH={prefix}'])
        run(['cmake', '--build', str(consumer / 'build'), '--parallel', '2'])
        run([str(consumer / 'build/consumer')])
        env = dict(os.environ, PKG_CONFIG_LIBDIR=str(prefix / 'lib/pkgconfig'))
        flags = shlex.split(run(['pkg-config', '--static', '--cflags', '--libs', 'cui-regex'], env=env))
        assert '-lcui_regex' in flags and '-lcui' in flags and '-pthread' in flags
        run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG', *sanitize, str(source), '-o', str(work / 'pc-consumer'), *flags])
        run([str(work / 'pc-consumer')])
        options = ['cmake', '-S', str(ROOT), '-G', 'Ninja', '-DCUI_BUILD_NATIVE=OFF', '-DCUI_BUILD_BIDI=OFF', '-DCUI_BUILD_CPP_EXAMPLE=OFF', '-DCUI_REGEX_LIBRARY=/nonexistent/cui-regex.a']
        run([*options, '-B', str(work / 'off'), '-DCUI_BUILD_REGEX=OFF'])
        graph = (work / 'off/build.ninja').read_text()
        assert 'libcui_regex.a' not in graph and 'cargo' not in graph
        run(['cmake', '--build', str(work / 'off'), '--target', 'cui', '--parallel', '2'])
        bad = subprocess.run([*options, '-B', str(work / 'bad'), '-DCUI_BUILD_REGEX=ON'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        assert bad.returncode and 'Missing optional regex archive' in bad.stdout
    print('PASS relocated CMake/pkg-config actual Input consumers, notices/hash, OFF Rust-free, missing archive guard')
if __name__ == '__main__': main()
