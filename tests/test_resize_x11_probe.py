#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Real destroyed-window regression; run only through test_local.py's Xvfb."""
import ctypes
import ctypes.util
import importlib.util
import os
from pathlib import Path
import sys


def main():
    if (not sys.platform.startswith('linux') or
            os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1' or
            not os.environ.get('DISPLAY') or
            os.environ['DISPLAY'].split('.')[0] == ':0' or
            os.environ.get('WAYLAND_DISPLAY')):
        return 77
    spec = importlib.util.spec_from_file_location('resize', Path(__file__).parent /
                                                'packaging/test_continuous_resize.py')
    resize = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(resize)
    library = ctypes.util.find_library('X11')
    if not library:
        return 77
    x = ctypes.CDLL(library)
    d, w = ctypes.c_void_p, ctypes.c_ulong
    x.XOpenDisplay.argtypes = [ctypes.c_char_p]; x.XOpenDisplay.restype = d
    x.XDefaultRootWindow.argtypes = [d]; x.XDefaultRootWindow.restype = w
    x.XCreateSimpleWindow.argtypes = [d, w, ctypes.c_int, ctypes.c_int, ctypes.c_uint,
                                     ctypes.c_uint, ctypes.c_uint, w, w]
    x.XCreateSimpleWindow.restype = w
    x.XDestroyWindow.argtypes = [d, w]
    x.XStoreName.argtypes = [d, w, ctypes.c_char_p]
    x.XQueryTree.argtypes = [d, w, ctypes.POINTER(w), ctypes.POINTER(w),
                            ctypes.POINTER(ctypes.POINTER(w)), ctypes.POINTER(ctypes.c_uint)]
    x.XFetchName.argtypes = [d, w, ctypes.POINTER(ctypes.c_void_p)]
    x.XResizeWindow.argtypes = [d, w, ctypes.c_uint, ctypes.c_uint]
    x.XFree.argtypes = [ctypes.c_void_p]
    x.XCloseDisplay.argtypes = [d]
    connection = x.XOpenDisplay(None)
    if not connection:
        return 77
    probe = resize.WindowProbe(x, connection)
    try:
        root = x.XDefaultRootWindow(connection)
        dead = x.XCreateSimpleWindow(connection, root, 0, 0, 10, 10, 0, 0, 0)
        live = x.XCreateSimpleWindow(connection, root, 0, 0, 10, 10, 0, 0, 0)
        x.XStoreName(connection, live, b'cui-probe-survivor')
        x.XDestroyWindow(connection, dead)
        x.XSync(connection, 0)
        # Deterministically query the stale XID returned by an earlier traversal.
        assert probe.find(dead, 'absent') is None
        name = ctypes.c_void_p()
        assert not probe._request(20, dead, x.XFetchName, ctypes.byref(name))
        assert probe.find(root, 'cui-probe-survivor') == live
        # BadWindow from ConfigureWindow must NOT be mistaken for lookup churn.
        try:
            probe._request(15, dead, x.XResizeWindow, 20, 20)
        except RuntimeError as error:
            assert 'request 12' in str(error), str(error)
        else:
            raise AssertionError('target resize failure was masked')
        # Nor may the trap suppress a lookup for an unexpected resource.
        try:
            probe._request(20, live, lambda conn, target: x.XFetchName(conn, dead, ctypes.byref(name)))
        except RuntimeError:
            pass
        else:
            raise AssertionError('unrelated resource error was masked')
        assert probe.find(root, 'cui-probe-survivor') == live
        x.XDestroyWindow(connection, live)
        x.XSync(connection, 0)
        print('PASS: stale QueryTree/FetchName tolerated; resize and unrelated errors rejected')
    finally:
        x.XCloseDisplay(connection)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
