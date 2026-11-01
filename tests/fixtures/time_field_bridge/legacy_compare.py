"""Compare actual old/new bridge trace processes inside the private display.

No historical binary download or candidate-derived expected output. Supply a
frozen pre-feature executable/library built against the same engine as current.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--old-executable', type=Path, required=True)
    parser.add_argument('--new-executable', type=Path, required=True)
    parser.add_argument('--old-library-dir', type=Path, required=True)
    parser.add_argument('--new-library-dir', type=Path, required=True)
    args = parser.parse_args()
    if os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1':
        raise SystemExit('Run through scripts/test_local.py; no desktop fallback')
    traces = {}
    for name, executable, library in (
            ('old', args.old_executable, args.old_library_dir),
            ('new', args.new_executable, args.new_library_dir)):
        env = dict(os.environ)
        env.pop('LD_PRELOAD', None)
        env['LD_LIBRARY_PATH'] = str(library.resolve())
        traces[name] = subprocess.check_output([str(executable.resolve())], env=env)
    if traces['old'] != traces['new']:
        raise AssertionError('Legacy trace mismatch\nOLD:\n' + traces['old'].decode() + '\nNEW:\n' + traces['new'].decode())
    print(json.dumps({'trace_sha256': hashlib.sha256(traces['old']).hexdigest(),
                      'scope': 'Separate loaded images, exact trace bytes, private display'}, indent=2))


if __name__ == '__main__':
    main()
