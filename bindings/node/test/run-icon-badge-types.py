#!/usr/bin/env python3
"""Typecheck the retained Icon and Badge API and run the authored inbox."""
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
tsc = [args.tsc, '--strict', '--target', 'ES2020', '--module', 'commonjs', '--skipLibCheck']
subprocess.run([*tsc, '--noEmit', str(bindings / 'node/test/icon-badge.types.ts')], check=True)
subprocess.run([*tsc, '--rootDir', str(bindings), '--outDir', str(args.output),
                str(bindings / 'typescript/examples/icon_badge_inbox.ts')], check=True)
module_dir = args.output / 'node'
module_dir.mkdir(parents=True, exist_ok=True)
for module in (bindings / 'node').glob('*.cjs'):
    shutil.copyfile(module, module_dir / module.name)
subprocess.run([args.node, str(args.output / 'typescript/examples/icon_badge_inbox.js')], check=True)
