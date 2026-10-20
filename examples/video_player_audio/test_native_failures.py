#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Failed startup must exit normally, with no desktop or physical audio."""
import os
import subprocess
import sys
import tempfile


def main():
    if (not sys.platform.startswith('linux') or
            os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1' or
            not os.environ.get('DISPLAY') or
            os.environ['DISPLAY'].split('.')[0] == ':0' or os.environ.get('WAYLAND_DISPLAY')):
        return 77
    player, font, media = sys.argv[1:]
    env = dict(os.environ, SDL_VIDEODRIVER='x11', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='cui-video-failures-') as folder:
        cases = [
            # Synchronous path validation rejects protocols before session creation.
            ([player, font, 'rejected:local-file', '--smoke', folder], env, True),
            # No resources acquired; quit still runs after failed app init.
            ([player, folder + '/missing-font', media, '--smoke', folder], env, False),
            # Font acquired, window setup fails without touching any real display.
            ([player, font, media, '--smoke', folder],
             dict(env, SDL_VIDEODRIVER='cui-unavailable-test-driver'), False),
        ]
        for command, case_env, session_failure in cases:
            result = subprocess.run(command, env=case_env, capture_output=True, text=True, timeout=15)
            assert result.returncode == 1, (command, result.returncode, result.stderr)
            assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr, result.stderr
            if session_failure:
                assert 'Cannot open supported fixed48k SDL output/media' in result.stderr, result.stderr
                assert 'frame 0 generation 0 UI error 0' in result.stderr, result.stderr
        print('PASS: initial open and two partial-init failures exit normally and cleanly')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
