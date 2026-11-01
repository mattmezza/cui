"""Optional offline partial-symbol provider; compiles only the bridge TU.

Pass an already validated matching immutable engine archive. No engine build occurs.
"""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--engine-archive', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
    '-fPIC', '-shared', '-DCUI_BRIDGE_BUILD=1',
    '-Dcui_bridge_time_field_change_size_v1=cui_test_hidden_time_size',
    '-I', str(root / 'include'), str(root / 'src/bridge/cui_bridge.c'),
    str(args.engine_archive.resolve()), '-lm', '-o', str(args.output.resolve())], check=True)
