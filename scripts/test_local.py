#!/usr/bin/env python3
"""Run local CTest without placing native test windows on a user's desktop."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def test_invocation(build, extra, platform, environment, locate):
    env = dict(environment)
    command = ['ctest', '--test-dir', str(build), '--output-on-failure', *extra]
    if platform.startswith('linux'):
        virtual = locate('xvfb-run')
        if not virtual:
            raise RuntimeError('xvfb-run is required for isolated local testing; no desktop fallback.')
        env.pop('DISPLAY', None)
        env.pop('WAYLAND_DISPLAY', None)
        env.pop('MIR_SOCKET', None)
        env['SDL_VIDEODRIVER'] = 'x11'
        command = [virtual, '-a', '-s', '-screen 0 1920x1080x24', *command]
    else:
        cache = build / 'CMakeCache.txt'
        native = [line.removeprefix('CUI_BUILD_NATIVE:BOOL=') for line in cache.read_text().splitlines()
                  if line.startswith('CUI_BUILD_NATIVE:BOOL=')] if cache.is_file() else []
        portable = native == ['OFF']
        if not portable and env.get('GITHUB_ACTIONS') != 'true':
            raise RuntimeError('Native testing requires an isolated session; local desktop testing is disabled.')
    return command, env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('build'))
    args, extra = parser.parse_known_args()
    if extra[:1] == ['--']:
        extra = extra[1:]
    try:
        command, env = test_invocation(args.build_dir, extra, sys.platform, os.environ, shutil.which)
    except RuntimeError as error:
        parser.exit(2, str(error) + '\n')
    print('Running CTest in an isolated display or portable-only build.', flush=True)
    return subprocess.run(command, env=env).returncode


if __name__ == '__main__':
    raise SystemExit(main())
