#!/usr/bin/env python3
import argparse,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=r).decode().split('\0')
roots={'include','src','backends','gallery','examples','tests','web'}
files=sorted(str(r/f) for value in paths if value for f in [Path(value)] if f.parts[0] in roots and f.suffix in {'.c','.h','.cpp'} and f.name!='generated.h')
subprocess.run(['clang-format',*(['--dry-run','--Werror'] if a.check else ['-i']),*files],check=True)
