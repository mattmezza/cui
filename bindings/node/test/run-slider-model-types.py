#!/usr/bin/env python3
"""Use an existing TS compiler; no installation/network and no desktop windows."""
import argparse
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--tsc', required=True)
p.add_argument('--node', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[3]
base = [a.tsc, '--strict', '--target', 'ES2020', '--module', 'commonjs', '--skipLibCheck']
subprocess.run([*base, '--noEmit', str(root / 'bindings/node/test/slider-model.types.ts')], check=True)
subprocess.run([*base, '--rootDir', str(root / 'bindings'), '--outDir', str(a.output),
                str(root / 'bindings/typescript/examples/slider_model.ts')], check=True)
node_dir = a.output / 'node'
node_dir.mkdir(parents=True, exist_ok=True)
for name in ('index.cjs', 'shadows.cjs', 'slider-model.cjs'):
    shutil.copyfile(root / 'bindings/node' / name, node_dir / name)
subprocess.run([a.node, str(a.output / 'typescript/examples/slider_model.js')], check=True)
