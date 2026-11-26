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
subprocess.run([*base, '--noEmit', str(bindings / 'node/test/disclosure.types.ts')], check=True)
subprocess.run([*base, '--rootDir', str(bindings), '--outDir', str(args.output),
                str(bindings / 'typescript/examples/workspace_preferences.ts')], check=True)
node_dir = args.output / 'node'
node_dir.mkdir(parents=True, exist_ok=True)
for module in (bindings / 'node').glob('*.cjs'):
    shutil.copyfile(module, node_dir / module.name)
subprocess.run([args.node, str(args.output / 'typescript/examples/workspace_preferences.js')], check=True)
