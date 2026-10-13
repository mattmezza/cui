#!/usr/bin/env python3
"""Run the small native CUI host through Python's standard-library ctypes.

This demonstrates the example host ABI; it is not a complete Python binding
for every CUI component. Callbacks are synchronous on this calling/UI thread.
"""
import argparse
import ctypes
from pathlib import Path
import sys


def run(on_changed, font=None, frames=0, library=None):
    if not callable(on_changed):
        raise TypeError('on_changed must be callable')
    if not isinstance(frames, int) or frames < 0 or frames > 0xFFFFFFFF:
        raise ValueError('frames must be an unsigned 32-bit integer')
    suffix = '.dll' if sys.platform == 'win32' else '.dylib' if sys.platform == 'darwin' else '.so'
    name = 'cui_example_host' if sys.platform == 'win32' else 'libcui_example_host'
    library = Path(library) if library else Path(__file__).resolve().parents[1] / 'build' / (name + suffix)
    host = ctypes.CDLL(str(library.resolve()))
    callback_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int)
    errors = []

    def changed(user, count):
        # Exceptions cannot unwind through C. Preserve the first exception and
        # re-raise after the synchronous host call has returned.
        if errors:
            return
        try:
            on_changed(count)
        except BaseException as error:
            errors.append(error)

    callback = callback_type(changed)  # Keep alive through the entire host call.
    host.cui_example_run.argtypes = [ctypes.c_char_p, ctypes.c_uint, callback_type, ctypes.c_void_p]
    host.cui_example_run.restype = ctypes.c_int
    result = host.cui_example_run(str(font).encode('utf-8') if font else None, frames, callback, None)
    if errors:
        raise errors[0]
    if result:
        raise RuntimeError(f'CUI example host returned {result}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font', nargs='?', help='Font path: enables a native SDL3 window')
    parser.add_argument('frames', nargs='?', type=int, default=0, help='0 runs until window close')
    parser.add_argument('--library', type=Path, help='Override compiled example-host library path')
    args = parser.parse_args()
    observed = []

    def changed(count):
        observed.append(count)
        print(f'Python count: {count}', flush=True)

    run(changed, args.font, args.frames, args.library)
    if not args.font:
        assert observed == [1, 2], observed
        sentinel = RuntimeError('callback sentinel')
        def fail(count):
            raise sentinel
        try:
            run(fail, library=args.library)
        except RuntimeError as error:
            assert error is sentinel
        else:
            raise AssertionError('Callback exception was lost')
        replay = []
        run(replay.append, library=args.library)
        assert replay == [1, 2], replay
        print('PASS: Python callbacks, exception propagation and fresh rerun')


if __name__ == '__main__':
    main()
