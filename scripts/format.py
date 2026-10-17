#!/usr/bin/env python3
import argparse,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=r).decode().split('\0')
roots={'include','src','backends','gallery','examples','tests','web','bindings'}
sdk_roots=(Path('bindings/go/sdk/source'),Path('bindings/rust/sdk/source'))
# Generated Go translation units contain go:build directives; clang-format would
# turn them into ordinary comments. The SDK freshness gate verifies exact bytes.
files=sorted(str(r/f) for value in paths if value for f in [Path(value)] if f.parts[0] in roots and f.suffix in {'.c','.h','.cpp'} and f.name!='generated.h' and not any(f.is_relative_to(sdk) for sdk in sdk_roots) and not (f.parent == Path('bindings/go/cui') and f.name.startswith('z_cui_')))
subprocess.run(['clang-format',*(['--dry-run','--Werror'] if a.check else ['-i']),*files],check=True)
