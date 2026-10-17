#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Resize a running callback app on a wrapper-owned private Linux X11 display."""
import argparse
import ctypes
import ctypes.util
import os
from pathlib import Path
import selectors
import struct
import subprocess
import sys
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', required=True, type=Path)
    parser.add_argument('--font', required=True, type=Path)
    args = parser.parse_args()
    if not sys.platform.startswith('linux') or os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1':
        print('SKIP: requires scripts/test_local.py private Linux Xvfb')
        return 77
    if not os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY'):
        print('SKIP: requires private X11 display without Wayland')
        return 77
    library = ctypes.util.find_library('X11')
    if not library:
        return 77
    x = ctypes.CDLL(library)
    window = ctypes.c_ulong
    display = ctypes.c_void_p
    x.XOpenDisplay.argtypes = [ctypes.c_char_p]; x.XOpenDisplay.restype = display
    x.XDefaultRootWindow.argtypes = [display]; x.XDefaultRootWindow.restype = window
    x.XQueryTree.argtypes = [display, window, ctypes.POINTER(window), ctypes.POINTER(window),
                            ctypes.POINTER(ctypes.POINTER(window)), ctypes.POINTER(ctypes.c_uint)]
    x.XFetchName.argtypes = [display, window, ctypes.POINTER(ctypes.c_void_p)]
    x.XFree.argtypes = [ctypes.c_void_p]
    x.XResizeWindow.argtypes = [display, window, ctypes.c_uint, ctypes.c_uint]
    x.XFlush.argtypes = [display]
    x.XCloseDisplay.argtypes = [display]
    connection = x.XOpenDisplay(None)
    if not connection:
        return 77
    try:
        with tempfile.TemporaryDirectory(prefix='cui-resize-') as temporary:
            folder = Path(temporary)
            title = folder.name
            quit_file = folder / 'quit'
            env = dict(os.environ, SDL_VIDEODRIVER='x11', SDL_RENDER_DRIVER='software')
            child = subprocess.Popen([str(args.executable.resolve()), str(args.font.resolve()),
                str(folder), str(quit_file), title], env=env, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, bufsize=1)
            records = []
            selector = selectors.DefaultSelector()
            selector.register(child.stdout, selectors.EVENT_READ)

            def drain(timeout):
                deadline = time.monotonic() + timeout
                while time.monotonic() < deadline:
                    events = selector.select(max(0, deadline - time.monotonic()))
                    if not events:
                        break
                    line = child.stdout.readline()
                    if not line:
                        break
                    if line.startswith('FRAME '):
                        records.append((time.monotonic(), line.split()))

            def find(parent):
                root, owner = window(), window()
                children = ctypes.POINTER(window)()
                count = ctypes.c_uint()
                if not x.XQueryTree(connection, parent, ctypes.byref(root), ctypes.byref(owner),
                                    ctypes.byref(children), ctypes.byref(count)):
                    return None
                found = None
                try:
                    for i in range(count.value):
                        name = ctypes.c_void_p()
                        if x.XFetchName(connection, children[i], ctypes.byref(name)) and name.value:
                            try:
                                if ctypes.string_at(name).decode(errors='replace') == title:
                                    found = int(children[i]); break
                            finally:
                                x.XFree(name)
                        found = find(children[i])
                        if found:
                            break
                finally:
                    if children:
                        x.XFree(children)
                return found

            try:
                deadline = time.monotonic() + 15
                target = None
                while time.monotonic() < deadline and child.poll() is None:
                    drain(.05)
                    target = find(x.XDefaultRootWindow(connection))
                    if target and records:
                        break
                if child.poll() == 77:
                    return 77
                assert target and records, 'runner never presented its initial frame'
                # Every resize occurs after an actual initial presentation: the
                # application is already running, not being resized before run().
                started = time.monotonic()
                widths = [360, 800, 1400, 480, 1100, 640] * 4
                for width in widths:
                    assert child.poll() is None, 'child exited during resize burst'
                    x.XResizeWindow(connection, target, width, 760)
                    x.XFlush(connection)
                    drain(.06)
                ended = time.monotonic()
                during = [record for stamp, record in records if started < stamp < ended]
                seen = {int(record[2]) for record in during}
                assert {360, 800, 1400} <= seen, f'missing intermediate widths during burst: {seen}'
                assert len(during) >= 6, f'only {len(during)} presentations during active burst'
                for record in during:
                    values = list(map(float, record[1:]))
                    seq, width, height, rw, rh, mx, mw, aw, bw, wide, row, page, signed = values[:13]
                    ax, ay, ah, bx, by, bh = values[13:19]
                    expected_rgb = tuple(map(int, values[19:22]))
                    assert rw == width and rh == height == 760
                    assert mx >= 0 and mw > 0 and mx + mw <= width + 1
                    assert page == 1 and signed == 1 and wide == (width >= 1024)
                    if width == 360:
                        assert row == 0
                    if width in (800, 1400):
                        assert row == 1
                    data = (folder / f'frame-{int(seq)}.bmp').read_bytes()
                    assert data[:2] == b'BM'
                    offset = struct.unpack_from('<I', data, 10)[0]
                    pw, ph = struct.unpack_from('<ii', data, 18)
                    depth = struct.unpack_from('<H', data, 28)[0]
                    assert (pw, abs(ph)) == (int(width), int(height)), (pw, ph, width, height)
                    assert depth in (24, 32)
                    pixels = data[offset:]
                    assert len(set(pixels)) > 16, 'presented frame has no rendered UI variation'
                    stride = ((pw * depth + 31) // 32) * 4
                    for cx, cy, cw, ch in ((ax, ay, aw, ah), (bx, by, bw, bh)):
                        px, py = int(cx + cw - 8), int(cy + ch - 8)
                        assert 0 <= px < pw and 0 <= py < abs(ph)
                        bitmap_y = abs(ph) - 1 - py if ph > 0 else py
                        start = offset + bitmap_y * stride + px * (depth // 8)
                        actual_rgb = tuple(reversed(data[start:start + 3]))
                        assert actual_rgb == expected_rgb, (width, px, py, actual_rgb, expected_rgb)
                quit_file.touch()
                x.XResizeWindow(connection, target, 801, 760); x.XFlush(connection)
                child.wait(timeout=10)
                assert child.returncode == 0, child.stderr.read()
                print(f'PASS: {len(during)} actual presentations at {sorted(seen)} during {ended-started:.2f}s X11 resize burst')
            finally:
                selector.close()
                if child.poll() is None:
                    child.terminate()
                    try:
                        child.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        child.kill(); child.wait(timeout=5)
                child.stdout.close(); child.stderr.close()
    finally:
        x.XCloseDisplay(connection)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
