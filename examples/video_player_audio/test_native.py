#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Actual decoded native pixels, only on wrapper-owned Linux X11."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def main():
    if (not sys.platform.startswith('linux') or
            os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1' or
            not os.environ.get('DISPLAY') or
            os.environ['DISPLAY'].split('.')[0] == ':0' or os.environ.get('WAYLAND_DISPLAY')):
        return 77
    with tempfile.TemporaryDirectory(prefix='cui-video-smoke-') as folder:
        env = dict(os.environ, SDL_VIDEODRIVER='x11', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
        subprocess.run([*sys.argv[1:], '--smoke', folder], env=env, check=True, timeout=30)
        for name, rgb in [('red.bmp', (255, 0, 0)), ('pause.bmp', (255, 0, 0)),
                          ('seek.bmp', (0, 0, 255))]:
            data = (Path(folder) / name).read_bytes()
            offset = struct.unpack_from('<I', data, 10)[0]
            width, height = struct.unpack_from('<ii', data, 18)
            depth = struct.unpack_from('<H', data, 28)[0]
            assert (width, abs(height)) == (640, 440) and depth in (24, 32)
            stride = ((width * depth + 31) // 32) * 4
            x, y = 320, 150
            row = abs(height) - 1 - y if height > 0 else y
            start = offset + row * stride + x * (depth // 8)
            actual = tuple(reversed(data[start:start + 3]))
            assert actual == rgb, (name, actual, rgb)
        print('PASS: native captured decoded red/pause/blue seek pixels with actual dummy PCM drain')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
