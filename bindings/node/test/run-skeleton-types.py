#!/usr/bin/env python3
"""Typecheck and run the composed TypeScript example with an existing compiler."""
import argparse
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--tsc', required=True)
parser.add_argument('--node', required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
bindings = Path(__file__).resolve().parents[2]
base = [args.tsc, '--strict', '--target', 'ES2020', '--module', 'commonjs', '--skipLibCheck']
subprocess.run([*base, '--noEmit', str(bindings / 'node/test/skeleton-presentation.types.ts')], check=True)
subprocess.run([*base, '--rootDir', str(bindings), '--outDir', str(args.output),
                str(bindings / 'typescript/examples/skeleton_profile.ts')], check=True)
node_dir = args.output / 'node'
node_dir.mkdir(parents=True, exist_ok=True)
for name in ('index.cjs', 'shadows.cjs', 'slider-model.cjs', 'time-field-localized.cjs',
             'skeleton-presentation.cjs'):
    shutil.copyfile(bindings / 'node' / name, node_dir / name)
subprocess.run([args.node, str(args.output / 'typescript/examples/skeleton_profile.js')], check=True)
