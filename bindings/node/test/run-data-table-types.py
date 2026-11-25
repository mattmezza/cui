"""Typecheck a DataTable API and run its compiled TypeScript inbox."""
import argparse
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--tsc", required=True)
parser.add_argument("--node", required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--case", choices=("keyed", "paging"), required=True)
args = parser.parse_args()
bindings = Path(__file__).resolve().parents[2]
tsc = [args.tsc, "--strict", "--target", "ES2020", "--module", "commonjs", "--skipLibCheck"]
fixture, example = {"keyed": ("keyed-data-table", "data_table_live"),
                    "paging": ("data-table-paging", "data_table_paging")}[args.case]
subprocess.run([*tsc, "--noEmit", str(bindings / f"node/test/{fixture}.types.ts")], check=True)
subprocess.run([*tsc, "--rootDir", str(bindings), "--outDir", str(args.output),
                str(bindings / f"typescript/examples/{example}.ts")], check=True)
node_dir = args.output / "node"
node_dir.mkdir(parents=True, exist_ok=True)
for module in (bindings / "node").glob("*.cjs"):
    shutil.copyfile(module, node_dir / module.name)
subprocess.run([args.node, str(args.output / f"typescript/examples/{example}.js")], check=True)
