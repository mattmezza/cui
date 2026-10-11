#!/usr/bin/env python3
import argparse,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
files=sorted(str(f) for d in ['include','src','backends','gallery','examples','tests'] for f in (r/d).rglob('*') if f.suffix in {'.c','.h','.cpp'} and f.name!='generated.h')
subprocess.run(['clang-format',*(['--dry-run','--Werror'] if a.check else ['-i']),*files],check=True)
