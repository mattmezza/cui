#!/usr/bin/env python3
"""Compile a headless native dependency consumer against a private prefix."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--compile-only', action='store_true',
                        help='Link the target binary without executing it (cross builds)')
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    env = dict(os.environ, PKG_CONFIG_LIBDIR=str(prefix / 'lib/pkgconfig'),
               PKG_CONFIG_PATH='')
    # FreeType 2.13.3 exposes its libtool version in freetype2.pc.
    packages = {'sdl3': '3.2.28', 'freetype2': '26.2.20', 'harfbuzz': '11.0.1'}
    for package, version in packages.items():
        actual = subprocess.check_output(['pkg-config', '--modversion', package],
                                         env=env, text=True).strip()
        if actual != version:
            raise RuntimeError(f'{package}: expected {version}, got {actual}')
    for license_path in ('sdl3/LICENSE.txt', 'freetype/FTL.TXT',
                         'freetype/GPLv2.TXT', 'freetype/LICENSE.TXT', 'harfbuzz/COPYING'):
        if not (prefix / 'share/licenses' / license_path).is_file():
            raise RuntimeError(f'missing license: {license_path}')
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', *packages], env=env, text=True))
    source = '''#include <SDL3/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#if FREETYPE_MAJOR != 2 || FREETYPE_MINOR != 13 || FREETYPE_PATCH != 3
#error Unexpected FreeType headers
#endif
int main(void) {
    FT_Library library = 0;
    if (SDL_GetVersion() != SDL_VERSION || FT_Init_FreeType(&library)) return 1;
    hb_buffer_t *buffer = hb_buffer_create();
    if (!buffer || hb_ft_font_get_ft_face(hb_font_get_empty()) != 0) return 2;
    hb_buffer_destroy(buffer);
    return FT_Done_FreeType(library);
}
'''
    with tempfile.TemporaryDirectory(prefix='cui-native-deps-') as directory:
        path = Path(directory)
        (path / 'probe.c').write_text(source)
        subprocess.run([*shlex.split(args.cc), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(path / 'probe.c'), '-o', str(path / 'probe'), *flags], check=True)
        if not args.compile_only:
            # No window or SDL video initialization: safe without a display.
            run_env = dict(os.environ)
            for key in ('LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH'):
                run_env[key] = str(prefix / 'lib') + os.pathsep + run_env.get(key, '')
            subprocess.run([str(path / 'probe')], env=run_env, check=True)
    print('Pinned native dependencies, licenses, and hb-ft consumer passed')


if __name__ == '__main__':
    main()
