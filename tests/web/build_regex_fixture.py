#!/usr/bin/env python3
"""Build isolated actual CUI Input+optional regex WASI reactor; no gallery edits."""
import argparse
import ast
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=True)
assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
module=ast.parse((ROOT/'scripts/build_web.py').read_text())
sources=next(ast.literal_eval(n.value) for n in module.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
sources=[s for s in sources if not s.startswith('web/') and not s.startswith('examples/')]
exports=['init','event','key','text','length','flags','predicates','copies','painted','readonly','masked','seed','pattern','select_all','generation','config','alloc','free','destroy','run']
subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-DCUI_REGEX_BROWSER_PROBE','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s) for s in sources],str(ROOT/'tests/integration/test_input_regex_control.c'),str(a.library.resolve()),'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export=regex_probe_{s}' for s in exports],'-o',str(a.output/'regex.wasm')],check=True)
shutil.copyfile(ROOT/'web/secure_input.js',a.output/'secure_input.js')
for path in (ROOT/'tests/web/fixtures/regex').iterdir():shutil.copyfile(path,a.output/path.name)
print(a.output)
